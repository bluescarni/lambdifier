#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <tuple>
#include <unordered_map>
#include <utility>
#include <vector>

#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <lambdifier/binary_operator.hpp>
#include <lambdifier/detail/string_conv.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

namespace lambdifier
{

binary_operator::binary_operator(char op, expression lhs, expression rhs)
    : op(op), lhs(std::move(lhs)), rhs(std::move(rhs))
{
    if (op != '+' && op != '-' && op != '*' && op != '/') {
        throw std::invalid_argument("Invalid binary operator: " + std::string(1, op));
    }
}

binary_operator::binary_operator(const binary_operator &) = default;
binary_operator::binary_operator(binary_operator &&) noexcept = default;

binary_operator::~binary_operator() = default;

const expression &binary_operator::get_lhs() const
{
    return lhs;
}

const expression &binary_operator::get_rhs() const
{
    return rhs;
}

expression &binary_operator::access_lhs()
{
    return lhs;
}

expression &binary_operator::access_rhs()
{
    return rhs;
}

void binary_operator::set_lhs(expression e)
{
    lhs = std::move(e);
}

void binary_operator::set_rhs(expression e)
{
    rhs = std::move(e);
}

llvm::Value *binary_operator::codegen(llvm_state &s) const
{
    auto *l = lhs.codegen(s);
    auto *r = rhs.codegen(s);
    if (!l || !r) {
        return nullptr;
    }

    switch (op) {
        case '+':
            return s.get_builder().CreateFAdd(l, r, "addtmp");
        case '-':
            return s.get_builder().CreateFSub(l, r, "subtmp");
        case '*':
            return s.get_builder().CreateFMul(l, r, "multmp");
        default:
            assert(op == '/');
            return s.get_builder().CreateFDiv(l, r, "divtmp");
    }
}

std::string binary_operator::to_string() const
{
    return "(" + lhs.to_string() + " " + op + " " + rhs.to_string() + ")";
}

double binary_operator::evaluate(std::unordered_map<std::string, double> &in) const
{
    switch (op) {
        case '+':
            return lhs(in) + rhs(in);
        case '-':
            return lhs(in) - rhs(in);
        case '*':
            return lhs(in) * rhs(in);
        default:
            assert(op == '/');
            return lhs(in) / rhs(in);
    }
}

void binary_operator::evaluate(std::unordered_map<std::string, std::vector<double>> &in, std::vector<double> &out) const
{
    switch (op) {
        case '+': {
            auto tmp = out;
            lhs(in, out);
            rhs(in, tmp);
            std::transform(out.begin(), out.end(), tmp.begin(), out.begin(), std::plus<double>());
            break;
        }
        case '-': {
            auto tmp = out;
            lhs(in, out);
            rhs(in, tmp);
            std::transform(out.begin(), out.end(), tmp.begin(), out.begin(), std::minus<double>());
            break;
        }
        case '*': {
            auto tmp = out;
            lhs(in, out);
            rhs(in, tmp);
            std::transform(out.begin(), out.end(), tmp.begin(), out.begin(), std::multiplies<double>());
            break;
        }
        default: {
            assert(op == '/');
            auto tmp = out;
            lhs(in, out);
            rhs(in, tmp);
            std::transform(out.begin(), out.end(), tmp.begin(), out.begin(), std::divides<double>());
            break;
        }
    }
}

void binary_operator::compute_connections(std::vector<std::vector<unsigned>> &node_connections, unsigned &node_counter) const
{
    const unsigned node_id = node_counter;
    node_counter++;
    node_connections.push_back(std::vector<unsigned>(2));
    node_connections[node_id][0] = node_counter;
    get_lhs().compute_connections(node_connections, node_counter);
    node_connections[node_id][1] = node_counter;
    get_rhs().compute_connections(node_connections, node_counter);
}

expression binary_operator::diff(const std::string &s) const
{
    switch (op) {
        case '+':
            return lhs.diff(s) + rhs.diff(s);
        case '-':
            return lhs.diff(s) - rhs.diff(s);
        case '*':
            return lhs.diff(s) * rhs + lhs * rhs.diff(s);
        default:
            assert(op == '/');
            return (lhs.diff(s) * rhs - lhs * rhs.diff(s)) / (rhs * rhs);
    }
}

llvm::Value *binary_operator::taylor_init(llvm_state &s, llvm::Value *arr) const
{
    auto &builder = s.get_builder();

    // Helper to create the LLVM operands.
    auto create_op = [&s, &builder, arr](const expression &e) -> llvm::Value * {
        if (auto num_ptr = e.extract<number>()) {
            // The expression is a number, run its codegen.
            return num_ptr->codegen(s);
        } else if (auto var_ptr = e.extract<variable>()) {
            // The expression is a variable. Check that it
            // is a u variable and extract its index.
            const auto var_name = var_ptr->get_name();
            if (var_name.rfind("u_", 0) != 0) {
                throw std::invalid_argument(
                    "Invalid variable name '" + var_name
                    + "' encountered in the Taylor initialization phase for a binary operator expression (the name "
                      "must be in the form 'u_n', where n is a non-negative integer)");
            }
            const auto idx = detail::uname_to_index(var_name);

            // Index into the array of derivatives.
            auto ptr = builder.CreateInBoundsGEP(arr, {builder.getInt32(0), builder.getInt32(idx)}, "diff_ptr");

            // Return a load instruction from the array of derivatives.
            return builder.CreateLoad(ptr, "diff_load");
        }
        throw std::invalid_argument("The invalid expression '" + e.to_string()
                                    + "' was passed to the Taylor initialization phase of a binary operator (the "
                                      "expression must be either a variable or a number, but it is neither)");
    };

    // Emit the codegen for the binary operation.
    switch (op) {
        case '+':
            return builder.CreateFAdd(create_op(lhs), create_op(rhs), "taylor_init_add");
        case '-':
            return builder.CreateFSub(create_op(lhs), create_op(rhs), "taylor_init_sub");
        case '*':
            return builder.CreateFMul(create_op(lhs), create_op(rhs), "taylor_init_mul");
        default:
            assert(op == '/');
            return builder.CreateFDiv(create_op(lhs), create_op(rhs), "taylor_init_div");
    }
}

namespace detail
{

namespace
{

// Common boilerplate for the implementation of
// the Taylor derivatives of binary operators.
auto bo_taylor_diff_common(llvm_state &s, const std::string &name)
{
    auto &builder = s.get_builder();

    // Check the function name.
    if (s.get_module().getFunction(name) != nullptr) {
        throw std::invalid_argument("Cannot add the function '" + name
                                    + "' when building the Taylor derivative of a binary operator expression: the "
                                      "function already exists in the LLVM module");
    }

    // Prepare the function prototype. The arguments are:
    // - const double pointer to the derivatives array,
    // - 32-bit integer (order of the derivative).
    std::vector<llvm::Type *> fargs{llvm::PointerType::getUnqual(builder.getDoubleTy()), builder.getInt32Ty()};

    // The function will return the n-th derivative as a double.
    auto *ft = llvm::FunctionType::get(builder.getDoubleTy(), fargs, false);
    assert(ft != nullptr);

    // Now create the function. Don't need to call it from outside,
    // thus internal linkage.
    auto *f = llvm::Function::Create(ft, llvm::Function::InternalLinkage, name, s.get_module());
    assert(f != nullptr);

    // Setup the function arugments.
    auto arg_it = f->args().begin();
    arg_it->setName("diff_ptr");
    arg_it->addAttr(llvm::Attribute::ReadOnly);
    arg_it->addAttr(llvm::Attribute::NoCapture);
    auto diff_ptr = arg_it;

    (++arg_it)->setName("order");
    auto order = arg_it;

    // Create a new basic block to start insertion into.
    auto *bb = llvm::BasicBlock::Create(s.get_context(), "entry", f);
    assert(bb != nullptr);
    builder.SetInsertPoint(bb);

    return std::tuple{f, diff_ptr, order};
}

// Derivative of number +- u_{idx}.
template <bool AddOrSub>
llvm::Function *bo_taylor_diff_addsub_numvar(llvm_state &s, const std::string &name, std::uint32_t n_uvars,
                                             const std::unordered_map<std::uint32_t, number> &, std::uint32_t idx)
{
    auto &builder = s.get_builder();

    auto [f, diff_ptr, order] = bo_taylor_diff_common(s, name);

    // Compute the index for accessing the derivative. The index is order * n_uvars + idx.
    auto arr_idx = builder.CreateAdd(builder.CreateMul(builder.getInt32(n_uvars), order), builder.getInt32(idx));
    // Convert into a pointer.
    auto arr_ptr = builder.CreateInBoundsGEP(diff_ptr, arr_idx, "diff_ptr");
    // Load the value.
    auto ret = builder.CreateLoad(arr_ptr, "diff_load");

    if constexpr (AddOrSub) {
        builder.CreateRet(ret);
    } else {
        builder.CreateRet(builder.CreateFNeg(ret));
    }

    s.verify_function(f);

    return f;
}

// Derivative of u_{idx} +- number.
template <bool AddOrSub>
llvm::Function *bo_taylor_diff_addsub_varnum(llvm_state &s, const std::string &name, std::uint32_t n_uvars,
                                             const std::unordered_map<std::uint32_t, number> &, std::uint32_t idx)
{
    auto &builder = s.get_builder();

    auto [f, diff_ptr, order] = bo_taylor_diff_common(s, name);

    // Compute the index for accessing the derivative. The index is order * n_uvars + idx.
    auto arr_idx = builder.CreateAdd(builder.CreateMul(builder.getInt32(n_uvars), order), builder.getInt32(idx));
    // Convert into a pointer.
    auto arr_ptr = builder.CreateInBoundsGEP(diff_ptr, arr_idx, "diff_ptr");
    // Load the value.
    auto ret = builder.CreateLoad(arr_ptr, "diff_load");

    // NOTE: does not matter here plus or minus.
    builder.CreateRet(ret);

    s.verify_function(f);

    return f;
}

// Derivative of u_{idx0} +- u_{idx1}.
template <bool AddOrSub>
llvm::Function *bo_taylor_diff_addsub_varvar(llvm_state &s, const std::string &name, std::uint32_t n_uvars,
                                             const std::unordered_map<std::uint32_t, number> &, std::uint32_t idx0,
                                             std::uint32_t idx1)
{
    auto &builder = s.get_builder();

    auto [f, diff_ptr, order] = bo_taylor_diff_common(s, name);

    // Compute the indices for accessing the derivatives. The indices are order * n_uvars + (idx0, idx1).
    auto arr_idx0 = builder.CreateAdd(builder.CreateMul(builder.getInt32(n_uvars), order), builder.getInt32(idx0));
    auto arr_idx1 = builder.CreateAdd(builder.CreateMul(builder.getInt32(n_uvars), order), builder.getInt32(idx1));
    // Convert into pointers.
    auto arr_ptr0 = builder.CreateInBoundsGEP(diff_ptr, arr_idx0, "diff_ptr0");
    auto arr_ptr1 = builder.CreateInBoundsGEP(diff_ptr, arr_idx1, "diff_ptr1");
    // Load the values.
    auto v0 = builder.CreateLoad(arr_ptr0, "diff_load0");
    auto v1 = builder.CreateLoad(arr_ptr1, "diff_load1");

    if constexpr (AddOrSub) {
        builder.CreateRet(builder.CreateFAdd(v0, v1));
    } else {
        builder.CreateRet(builder.CreateFSub(v0, v1));
    }

    s.verify_function(f);

    return f;
}

llvm::Function *bo_taylor_diff_add(const binary_operator &bo, llvm_state &s, const std::string &name,
                                   std::uint32_t n_uvars, const std::unordered_map<std::uint32_t, number> &cd_uvars)
{
    if (bo.get_lhs().extract<number>() != nullptr) {
        // lhs is a number, rhs must be a variable.
        return bo_taylor_diff_addsub_numvar<true>(s, name, n_uvars, cd_uvars,
                                                  uname_to_index(bo.get_rhs().extract<variable>()->get_name()));
    } else if (bo.get_rhs().extract<number>() != nullptr) {
        // rhs is a number, lhs must be a variable.
        return bo_taylor_diff_addsub_varnum<true>(s, name, n_uvars, cd_uvars,
                                                  uname_to_index(bo.get_lhs().extract<variable>()->get_name()));
    } else {
        // Both are variables.
        return bo_taylor_diff_addsub_varvar<true>(s, name, n_uvars, cd_uvars,
                                                  uname_to_index(bo.get_lhs().extract<variable>()->get_name()),
                                                  uname_to_index(bo.get_rhs().extract<variable>()->get_name()));
    }
}

llvm::Function *bo_taylor_diff_sub(const binary_operator &bo, llvm_state &s, const std::string &name,
                                   std::uint32_t n_uvars, const std::unordered_map<std::uint32_t, number> &cd_uvars)
{
    if (bo.get_lhs().extract<number>() != nullptr) {
        // lhs is a number, rhs must be a variable.
        return bo_taylor_diff_addsub_numvar<false>(s, name, n_uvars, cd_uvars,
                                                   uname_to_index(bo.get_rhs().extract<variable>()->get_name()));
    } else if (bo.get_rhs().extract<number>() != nullptr) {
        // rhs is a number, lhs must be a variable.
        return bo_taylor_diff_addsub_varnum<false>(s, name, n_uvars, cd_uvars,
                                                   uname_to_index(bo.get_lhs().extract<variable>()->get_name()));
    } else {
        // Both are variables.
        return bo_taylor_diff_addsub_varvar<false>(s, name, n_uvars, cd_uvars,
                                                   uname_to_index(bo.get_lhs().extract<variable>()->get_name()),
                                                   uname_to_index(bo.get_rhs().extract<variable>()->get_name()));
    }
}

// Derivative of number * u_{idx}.
llvm::Function *bo_taylor_diff_mul_numvar(llvm_state &s, const std::string &name, std::uint32_t n_uvars,
                                          const std::unordered_map<std::uint32_t, number> &, const number &num,
                                          std::uint32_t idx)
{
    auto &builder = s.get_builder();

    auto [f, diff_ptr, order] = bo_taylor_diff_common(s, name);

    // Compute the index for accessing the derivative. The index is order * n_uvars + idx.
    auto arr_idx = builder.CreateAdd(builder.CreateMul(builder.getInt32(n_uvars), order), builder.getInt32(idx));
    // Convert into a pointer.
    auto arr_ptr = builder.CreateInBoundsGEP(diff_ptr, arr_idx, "diff_ptr");
    // Load the value.
    auto ret = builder.CreateLoad(arr_ptr, "diff_load");

    builder.CreateRet(builder.CreateFMul(num.codegen(s), ret));

    s.verify_function(f);

    return f;
}

// Derivative of u_{idx0} * u_{idx1}.
llvm::Function *bo_taylor_diff_mul_varvar(llvm_state &s, const std::string &name, std::uint32_t n_uvars,
                                          const std::unordered_map<std::uint32_t, number> &, std::uint32_t idx0,
                                          std::uint32_t idx1)
{
    auto &builder = s.get_builder();

    auto [f, diff_ptr, order] = bo_taylor_diff_common(s, name);

    // Accumulator for the result.
    auto ret_acc = builder.CreateAlloca(builder.getDoubleTy(), 0, "ret_acc");
    builder.CreateStore(llvm::ConstantFP::get(s.get_context(), llvm::APFloat(0.)), ret_acc);

    // Initial value for the for-loop. We will be operating
    // in the range [0, order] (i.e., order inclusive).
    auto start_val = builder.getInt32(0);

    // Make the new basic block for the loop header,
    // inserting after current block.
    auto *preheader_bb = builder.GetInsertBlock();
    auto *loop_bb = llvm::BasicBlock::Create(s.get_context(), "loop", f);

    // Insert an explicit fall through from the current block to the loop_bb.
    builder.CreateBr(loop_bb);

    // Start insertion in loop_bb.
    builder.SetInsertPoint(loop_bb);

    // Start the PHI node with an entry for Start.
    auto *j_var = builder.CreatePHI(builder.getInt32Ty(), 2, "j");
    j_var->addIncoming(start_val, preheader_bb);

    // Loop body.
    // Compute the indices for accessing the derivatives in this loop iteration.
    // The indices are:
    // - (order - j_var) * n_uvars + idx0,
    // - j_var * n_uvars + idx1.
    auto arr_idx0 = builder.CreateAdd(builder.CreateMul(builder.CreateSub(order, j_var), builder.getInt32(n_uvars)),
                                      builder.getInt32(idx0));
    auto arr_idx1 = builder.CreateAdd(builder.CreateMul(j_var, builder.getInt32(n_uvars)), builder.getInt32(idx1));
    // Convert into pointers.
    auto arr_ptr0 = builder.CreateInBoundsGEP(diff_ptr, arr_idx0, "diff_ptr0");
    auto arr_ptr1 = builder.CreateInBoundsGEP(diff_ptr, arr_idx1, "diff_ptr1");
    // Load the values.
    auto v0 = builder.CreateLoad(arr_ptr0, "diff_load0");
    auto v1 = builder.CreateLoad(arr_ptr1, "diff_load1");
    // Update ret_acc.
    builder.CreateStore(builder.CreateFAdd(builder.CreateLoad(ret_acc), builder.CreateFMul(v0, v1)), ret_acc);

    // Compute the next value of the iteration.
    // NOTE: addition works regardless of integral signedness.
    auto *next_j_var = builder.CreateAdd(j_var, builder.getInt32(1), "next_j");

    // Compute the end condition.
    // NOTE: we use the unsigned less-than-or-equal predicate.
    auto *end_cond = builder.CreateICmp(llvm::CmpInst::ICMP_ULE, next_j_var, order, "loopcond");

    // Create the "after loop" block and insert it.
    auto *loop_end_bb = builder.GetInsertBlock();
    auto *after_bb = llvm::BasicBlock::Create(s.get_context(), "afterloop", f);

    // Insert the conditional branch into the end of loop_end_bb.
    builder.CreateCondBr(end_cond, loop_bb, after_bb);

    // Any new code will be inserted in after_bb.
    builder.SetInsertPoint(after_bb);

    // Add a new entry to the PHI node for the backedge.
    j_var->addIncoming(next_j_var, loop_end_bb);

    builder.CreateRet(builder.CreateLoad(ret_acc));

    s.verify_function(f);

    return f;
}

llvm::Function *bo_taylor_diff_mul(const binary_operator &bo, llvm_state &s, const std::string &name,
                                   std::uint32_t n_uvars, const std::unordered_map<std::uint32_t, number> &cd_uvars)
{
    if (auto num_ptr = bo.get_lhs().extract<number>()) {
        // lhs is a number, rhs must be a variable.
        return bo_taylor_diff_mul_numvar(s, name, n_uvars, cd_uvars, *num_ptr,
                                         uname_to_index(bo.get_rhs().extract<variable>()->get_name()));
    } else if (auto num_ptr = bo.get_rhs().extract<number>()) {
        // rhs is a number, lhs must be a variable.
        return bo_taylor_diff_mul_numvar(s, name, n_uvars, cd_uvars, *num_ptr,
                                         uname_to_index(bo.get_lhs().extract<variable>()->get_name()));
    } else {
        // Both are variables.
        return bo_taylor_diff_mul_varvar(s, name, n_uvars, cd_uvars,
                                         uname_to_index(bo.get_lhs().extract<variable>()->get_name()),
                                         uname_to_index(bo.get_rhs().extract<variable>()->get_name()));
    }
}

llvm::Function *bo_taylor_diff_div(const binary_operator &, llvm_state &, const std::string &, std::uint32_t,
                                   const std::unordered_map<std::uint32_t, number> &)
{
    // TODO
    throw;
}

} // namespace

} // namespace detail

llvm::Function *binary_operator::taylor_diff(llvm_state &s, const std::string &name, std::uint32_t n_uvars,
                                             const std::unordered_map<std::uint32_t, number> &cd_uvars) const
{
    // lhs and rhs must be u vars or numbers.
    auto check_arg = [](const expression &e) {
        if (e.extract<number>() != nullptr) {
            // The expression is a number.
        } else if (auto var_ptr = e.extract<variable>()) {
            // The expression is a variable. Check that it
            // is a u variable and extract its index.
            const auto var_name = var_ptr->get_name();
            if (var_name.rfind("u_", 0) != 0) {
                throw std::invalid_argument(
                    "Invalid variable name '" + var_name
                    + "' encountered in the Taylor diff phase for a binary operator expression (the name "
                      "must be in the form 'u_n', where n is a non-negative integer)");
            }
        } else {
            throw std::invalid_argument("The invalid expression '" + e.to_string()
                                        + "' was passed to the Taylor diff phase of a binary operator (the "
                                          "expression must be either a variable or a number, but it is neither)");
        }
    };

    check_arg(lhs);
    check_arg(rhs);

    // They cannot be both numbers.
    if (lhs.extract<number>() != nullptr && rhs.extract<number>() != nullptr) {
        throw std::invalid_argument(
            "Cannot compute the Taylor derivative in a binary operator if both operands are numbers");
    }

    switch (op) {
        case '+':
            return detail::bo_taylor_diff_add(*this, s, name, n_uvars, cd_uvars);
        case '-':
            return detail::bo_taylor_diff_sub(*this, s, name, n_uvars, cd_uvars);
        case '*':
            return detail::bo_taylor_diff_mul(*this, s, name, n_uvars, cd_uvars);
        default:
            assert(op == '/');
            return detail::bo_taylor_diff_div(*this, s, name, n_uvars, cd_uvars);
    }
}

} // namespace lambdifier
