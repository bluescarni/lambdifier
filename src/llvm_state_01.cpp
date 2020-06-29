#include <cassert>
#include <cstdint>
#include <limits>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <lambdifier/detail/check_symbol_name.hpp>
#include <lambdifier/detail/string_conv.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/function_call.hpp>
#include <lambdifier/llvm_state.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

// TODO remove
#include <iostream>

namespace lambdifier
{

namespace detail
{

namespace
{

expression llvm_value_to_expression(const llvm::Value *val,
                                    const std::unordered_map<const llvm::Value *, expression> &value_exp_map)
{
    // Helper to extract the representation
    // of val in order to produce better error
    // messages.
    auto get_val_repr = [val]() {
        std::string out;
        llvm::raw_string_ostream ostr(out);
        val->print(ostr);
        return ostr.str();
    };

    if (auto it = value_exp_map.find(val); it == value_exp_map.end()) {
        // The value is not in the map. We can deal with it only
        // if it is a constant.
        if (llvm::isa<llvm::Constant>(val)) {
            if (auto cfp = llvm::dyn_cast<llvm::ConstantFP>(val)) {
                // Double-precision constant.
                return expression{number{cfp->getValueAPF().convertToDouble()}};
            } else if (auto cint = llvm::dyn_cast<llvm::ConstantInt>(val)) {
                // 32-bit signed integer.
                const auto &v = cint->getValue();
                assert(v.isSignedIntN(32));
                return expression{number{static_cast<double>(v.getSExtValue())}};
            }
            throw std::runtime_error("An unknown constant type was encountered while converting IR to expression. The "
                                     "value representation is: "
                                     + get_val_repr());
        } else {
            throw std::runtime_error("A value of unknown type was encountered while converting IR to expression. The "
                                     "value representation is: "
                                     + get_val_repr());
        }
    } else {
        // The value is already in the map. Fetch the corresponding expression.
        return it->second;
    }
}

} // namespace

} // namespace detail

void llvm_state::add_llvm_inst_to_value_exp_map(std::unordered_map<const llvm::Value *, expression> &value_exp_map,
                                                const llvm::Instruction &inst, std::optional<expression> &retval) const
{
    // The current instruction cannot be in the map.
    assert(value_exp_map.find(&inst) == value_exp_map.end());

    // Fetch the operands array (as a pointer to
    // the first operand) and its size.
    auto op_ptr = inst.getOperandList();
    const auto op_n = inst.getNumOperands();

    switch (inst.getOpcode()) {
        case llvm::Instruction::Call: {
            // The instruction is a function call.
            // Get the name of the function.
            // NOTE: in a function call instruction,
            // the arguments come first, while the function name
            // is the last operand.
            assert(op_n > 0u);
            const auto func_name = std::string{op_ptr[op_n - 1u].get()->getName()};

            // NOTE: this if-cascade should become a global unordered_map
            // encoding how to translate llvm function calls into expressions.
            // Entries into this map could be added to handle user-defined functions
            // (i.e., the sin() implementation in math_functions should add the
            // handler for llvm.sin.f64).
            if (func_name == "llvm.sin.f64") {
                // Sine.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, sin(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.cos.f64") {
                // Cosine.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, cos(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.exp.f64") {
                // Exp.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, exp(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.exp2.f64") {
                // Exp2.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, exp2(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.log.f64") {
                // log.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, log(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.log2.f64") {
                // log2.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, log2(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.log10.f64") {
                // log10.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, log10(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.sqrt.f64") {
                // sqrt.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, sqrt(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.fabs.f64") {
                // abs.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, abs(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.pow.f64.f64") {
                // Pow.
                assert(op_n == 3u);
                value_exp_map.emplace(&inst, pow(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map),
                                                 detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map)));
            } else if (func_name == "llvm.powi.f64") {
                // Powi.
                assert(op_n == 3u);
                value_exp_map.emplace(&inst, pow(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map),
                                                 detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map)));
            } else {
                // In the default case, we will try to
                // see if the function call refers to a
                // non-empty function from the module.
                if (auto f = module->getFunction(func_name); f != nullptr && !f->empty()) {
                    // Build the arguments for the function call.
                    std::vector<expression> f_args;
                    for (auto arg_it = f->arg_begin(); arg_it != f->arg_end(); ++arg_it) {
                        f_args.emplace_back(variable{arg_it->getName()});
                    }

                    // Prepare the function call.
                    function_call fc{func_name, std::move(f_args)};
                    fc.set_type(function_call::type::internal);
                    // NOTE: use the same function attributes
                    // as the functions implemented in math_functions.cpp.
                    fc.set_attributes({llvm::Attribute::NoUnwind, llvm::Attribute::Speculatable,
                                       llvm::Attribute::ReadNone, llvm::Attribute::WillReturn});

                    // For the derivative, we fetch the expression representation
                    // of func_name and we derive it.
                    // NOTE: other possible approaches (not clear which one is better):
                    // - look for the derivative of func_name as an LLVM function in the module
                    //   (following some naming convention, e.g., func_name.d.x) and convert that
                    //   into an expression instead. This would have the advantage of letting LLVM
                    //   simplify the derivative;
                    // - as above, but instead of extracting the expression representation of func_name.d.x,
                    //   return a function call to it.
                    fc.set_diff_f([this, func_name](const std::vector<expression> &, const std::string &s) {
                        auto func_ex = this->to_expression(func_name);
                        return func_ex.diff(s);
                    });

                    value_exp_map.emplace(&inst, expression{std::move(fc)});
                } else {
                    throw std::runtime_error("Unable to convert an IR call to the function '" + func_name
                                             + "' into an expression: the function is either not present in the "
                                               "module, or it is an empty function");
                }
            }

            break;
        }
        case llvm::Instruction::FAdd:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             + detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::FMul:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             * detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::FSub:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             - detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::FDiv:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             / detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::FNeg:
            assert(op_n == 1u);
            value_exp_map.emplace(&inst, -detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map));
            break;

        case llvm::Instruction::Ret:
            assert(op_n == 1u);
            // NOTE: we will stop processing instructions
            // the moment we have the first ret statement.
            assert(!retval);
            retval.emplace(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map));
            break;

        default:
            throw std::runtime_error(std::string{"Unknown instruction encountered while converting IR to expression: "}
                                     + inst.getOpcodeName());
    }
}

expression llvm_state::to_expression(const std::string &name) const
{
    // Fetch the function.
    auto f = module->getFunction(name);
    if (f == nullptr || f->empty()) {
        throw std::runtime_error("Unable to convert an IR call to the function '" + name
                                 + "' into an expression: the function is either not present in the "
                                   "module, or it is an empty function");
    }

    if (f->getBasicBlockList().size() != 1u) {
        throw std::runtime_error("Only single-block functions can be converted to expressions, but the function '"
                                 + name + "' has " + std::to_string(f->getBasicBlockList().size()) + " blocks");
    }

    // Fetch the function's entry block.
    const auto &eb = f->getEntryBlock();

    // value_exp_map will map LLVM values to corresponding expressions.
    // retval will end up containing the return value of the function.
    std::unordered_map<const llvm::Value *, expression> value_exp_map;
    std::optional<expression> retval;

    // Add the function arguments to value_exp_map.
    for (auto arg_it = f->arg_begin(); arg_it != f->arg_end(); ++arg_it) {
        [[maybe_unused]] const auto res = value_exp_map.emplace(&*arg_it, variable{arg_it->getName()});
        assert(res.second);
    }

    // Iterate over the instructions in the block.
    for (const auto &inst : eb) {
        add_llvm_inst_to_value_exp_map(value_exp_map, inst, retval);
        if (retval) {
            // Forcibily break out the moment we have a return statement.
            break;
        }
    }

    if (!retval) {
        throw std::runtime_error("Unable to convert an IR call to the function '" + name
                                 + "' into an expression: the function has no return statement");
    }

    return std::move(*retval);
}

std::string llvm_state::dump_function(const std::string &name) const
{
    if (auto f = module->getFunction(name)) {
        std::string out;
        llvm::raw_string_ostream ostr(out);
        f->print(ostr);
        return ostr.str();
    } else {
        throw std::invalid_argument("Could not locate the function called '" + name + "'");
    }
}

// Create the function to implement the n-th order normalised derivative of a
// state variable in a Taylor system. n_uvars is the total number of
// u variables in the decomposition, var is the u variable which is equal to
// the first derivative of the state variable.
llvm::Function *llvm_state::taylor_add_sv_diff(const std::string &fname, std::uint32_t n_uvars, const variable &var)
{
    // TODO check if fname is present already.

    // Extract the index of the u variable.
    const auto u_idx = detail::uname_to_index(var.get_name());

    // Prepare the main function prototype. The arguments are:
    // - const double pointer to the derivatives array,
    // - 32-bit integer (order of the derivative).
    std::vector<llvm::Type *> fargs{llvm::PointerType::getUnqual(builder->getDoubleTy()), builder->getInt32Ty()};

    // The function will return the n-th derivative.
    auto *ft = llvm::FunctionType::get(builder->getDoubleTy(), fargs, false);
    assert(ft != nullptr);

    // Now create the function. Don't need to call it from outside,
    // thus internal linkage.
    auto *f = llvm::Function::Create(ft, llvm::Function::InternalLinkage, fname, module.get());
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
    auto *bb = llvm::BasicBlock::Create(get_context(), "entry", f);
    assert(bb != nullptr);
    builder->SetInsertPoint(bb);

    // Fetch from diff_ptr the pointer to the u variable
    // at u_idx. The index is (order - 1) * n_uvars + u_idx.
    auto in_ptr = builder->CreateInBoundsGEP(
        diff_ptr,
        {builder->CreateAdd(
            builder->CreateMul(builder->getInt32(n_uvars), builder->CreateSub(order, builder->getInt32(1))),
            builder->getInt32(u_idx))},
        "diff_ptr");

    // Load the value from in_ptr.
    auto diff_load = builder->CreateLoad(in_ptr, "diff_load");

    // We have to divide the derivative by order
    // to get the normalised derivative of the state variable.
    // TODO precompute in the main function the 1/n factors?
    auto ret = builder->CreateFDiv(diff_load, builder->CreateUIToFP(order, builder->getDoubleTy()));

    builder->CreateRet(ret);

    // Verify it.
    verify_function(f);

    return f;
}

llvm::Function *llvm_state::taylor_add_sv_diff(const std::string &, std::uint32_t, const number &)
{
    // TODO
    throw std::runtime_error("No support for state variables with constant derivatives yet!");
}

void llvm_state::add_taylor(const std::string &name, std::vector<expression> sys, unsigned max_order)
{
    // TODO taylor function naming.
    detail::check_symbol_name(name);

    if (module->getNamedValue(name) != nullptr) {
        throw std::invalid_argument("The name '" + name + "' already exists in the module");
    }

    if (max_order == 0u) {
        throw std::invalid_argument("The maximum order cannot be zero");
    }

    // Record the number of equations/variables.
    const auto n_eq = sys.size();

    // Decompose the system of equations.
    const auto dc = taylor_decompose(std::move(sys));

    std::cout << "Decomposition:\n";
    for (const auto &ex : dc) {
        std::cout << ex << '\n';
    }

    // Compute the number of u variables.
    assert(dc.size() > n_eq);
    const auto n_uvars = dc.size() - n_eq;

    std::cout << "n vars/eq: " << n_eq << '\n';
    std::cout << "n uvars: " << n_uvars << '\n';

    // Overflow checking. We want to make sure we can do all computations
    // using uint32_t.
    if (n_eq > std::numeric_limits<std::uint32_t>::max() || n_uvars > std::numeric_limits<std::uint32_t>::max()
        || n_uvars > std::numeric_limits<std::uint32_t>::max() / max_order) {
        throw std::overflow_error("An overflow condition was detected in the number of variables");
    }

    // Create the functions for the computation of the derivatives
    // of the u variables. We begin with the state variables.
    // We will also identify the state variables whose derivatives
    // are constants and record them.
    std::unordered_map<std::uint32_t, number> cd_uvars;
    // We will store pointers to the created functions
    // for later use.
    std::vector<llvm::Function *> u_diff_funcs;
    // NOTE: the derivatives of the state variables
    // are at the end of the decomposition vector.
    for (decltype(dc.size()) i = n_uvars; i < dc.size(); ++i) {
        const auto &ex = dc[i];
        const auto u_idx = static_cast<std::uint32_t>(i - n_uvars);

        if (auto var_ptr = ex.extract<variable>()) {
            // ex is a variable.
            u_diff_funcs.emplace_back(taylor_add_sv_diff(name, static_cast<std::uint32_t>(n_uvars), u_idx, *var_ptr));
        } else if (auto num_ptr = ex.extract<number>()) {
            // ex is a number. Add its index to the list
            // of constant-derivative state variables.
            cd_uvars.emplace(u_idx, *num_ptr);
            u_diff_funcs.emplace_back(taylor_add_sv_diff(name, static_cast<std::uint32_t>(n_uvars), u_idx, *num_ptr));
        } else {
            // NOTE: ex can be only a variable or a number.
            assert(false);
        }
    }

    // Prepare the main function prototype. The arguments are:
    // - double pointer to in/out array,
    // - double (timestep),
    // - 32-bit integer (order of the derivative).
    std::vector<llvm::Type *> fargs{llvm::PointerType::getUnqual(builder->getDoubleTy()), builder->getDoubleTy(),
                                    builder->getInt32Ty()};
    // The function does not return anything.
    auto *ft = llvm::FunctionType::get(builder->getVoidTy(), fargs, false);
    assert(ft != nullptr);
    // Now create the function.
    auto *f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, module.get());
    assert(f != nullptr);
    // Set the name of the function arguments.
    auto arg_it = f->args().begin();
    auto in_out_arg = arg_it;
    (arg_it++)->setName("in_out");
    auto h_arg = arg_it;
    (arg_it++)->setName("h");
    auto order_arg = arg_it;
    arg_it->setName("order");

    // Create a new basic block to start insertion into.
    auto *bb = llvm::BasicBlock::Create(get_context(), "entry", f);
    assert(bb != nullptr);
    builder->SetInsertPoint(bb);

    // Create the array of derivatives for the u variables.
    // NOTE: the static cast is fine, as we checked above that n_uvars * max_order
    // fits in 32 bits.
    auto array_type = llvm::ArrayType::get(builder->getDoubleTy(), static_cast<std::uint64_t>(n_uvars * max_order));
    assert(array_type != nullptr);
    auto diff_arr = builder->CreateAlloca(array_type, 0, "diff");
    assert(diff_arr != nullptr);
    // Store a pointer to the beginning of the
    // derivatives array.
    auto base_diff_ptr
        = builder->CreateInBoundsGEP(diff_arr, {builder->getInt32(0), builder->getInt32(0)}, "base_diff_ptr");

    // Create also the accumulators for the state variables.
    std::vector<llvm::Value *> sv_acc;
    for (std::uint32_t i = 0; i < n_eq; ++i) {
        sv_acc.emplace_back(builder->CreateAlloca(builder->getDoubleTy(), 0, "sv_acc_" + detail::li_to_string(i)));
    }

    // Create the accumulator for the timestep.
    auto h_acc = builder->CreateAlloca(builder->getDoubleTy(), 0, "h_acc");
    builder->CreateStore(h_arg, h_acc);

    // Fill-in the order-0 row of the derivatives array.
    // Use a separate block for clarity.
    auto *init_bb = llvm::BasicBlock::Create(get_context(), "order_0_init", f);
    builder->CreateBr(init_bb);
    builder->SetInsertPoint(init_bb);

    // Load the initial values for the state variables from in_out.
    for (std::uint32_t i = 0; i < n_eq; ++i) {
        // Fetch the input pointer from in_out.
        auto in_ptr = builder->CreateInBoundsGEP(in_out_arg, {builder->getInt32(i)}, "in_out_ptr");

        // Create the load instruction from in_out.
        auto load_inst = builder->CreateLoad(in_ptr, "in_out_load");

        // Fetch the target pointer in diff_arr.
        auto diff_ptr = builder->CreateInBoundsGEP(diff_arr,
                                                   // The offsets. The first is fixed because
                                                   // diff_arr somehow becomes a pointer
                                                   // to itself in the generation of the instruction,
                                                   // and thus we need to deref it. The second
                                                   // offset is the index into the array.
                                                   {builder->getInt32(0), builder->getInt32(i)},
                                                   // Name for the pointer variable.
                                                   "diff_ptr");

        // Do the copy, both into the diff array and into the accumulators.
        builder->CreateStore(load_inst, diff_ptr);
        builder->CreateStore(load_inst, sv_acc[i]);
    }

    // Fill in the initial values for the other u vars in the diff array.
    // These are not loaded directly from in_out, rather they are computed
    // via the taylor_init machinery.
    for (auto i = static_cast<std::uint32_t>(n_eq); i < n_uvars; ++i) {
        const auto &u_ex = dc[i];

        // Fetch the target pointer in diff_arr.
        auto diff_ptr = builder->CreateInBoundsGEP(diff_arr, {builder->getInt32(0), builder->getInt32(i)}, "diff_ptr");

        // Run the initialization and store the result.
        builder->CreateStore(u_ex.taylor_init(*this, diff_arr), diff_ptr);
    }

    // The for loop to fill the derivatives array. We will iterate
    // in the [1, order) range.
    // Init the variable for the start of the loop (i = 1).
    auto start_val = builder->getInt32(1);

    // Make the new basic block for the loop header,
    // inserting after current block.
    auto *preheader_bb = builder->GetInsertBlock();
    auto *loop_bb = llvm::BasicBlock::Create(get_context(), "loop", f);

    // Insert an explicit fall through from the current block to the loop_bb.
    builder->CreateBr(loop_bb);

    // Start insertion in loop_bb.
    builder->SetInsertPoint(loop_bb);

    // Start the PHI node with an entry for Start.
    auto *variable = builder->CreatePHI(builder->getInt32Ty(), 2, "i");
    variable->addIncoming(start_val, preheader_bb);

    // TODO loop body.

    // Compute the next value of the iteration.
    // NOTE: addition works regardless of integral signedness.
    auto *next_var = builder->CreateAdd(variable, builder->getInt32(1), "nextvar");

    // Compute the end condition.
    // NOTE: we use the unsigned less-than predicate.
    auto *end_cond = builder->CreateICmp(llvm::CmpInst::ICMP_ULT, next_var, order_arg, "loopcond");

    // Create the "after loop" block and insert it.
    auto *loop_end_bb = builder->GetInsertBlock();
    auto *after_bb = llvm::BasicBlock::Create(get_context(), "afterloop", f);

    // Insert the conditional branch into the end of loop_end_bb.
    builder->CreateCondBr(end_cond, loop_bb, after_bb);

    // Any new code will be inserted in after_bb.
    builder->SetInsertPoint(after_bb);

    // Add a new entry to the PHI node for the backedge.
    variable->addIncoming(next_var, loop_end_bb);

    // The last step is to finalise the accumulators
    // and to write them into in_out.
    for (std::uint32_t i = 0; i < n_eq; ++i) {
        auto sv = sv_acc[i];

        // Compute h_acc * (derivative of order "order_arg" of the
        // current state variable).
        auto sv_diff_f_call = builder->CreateCall(u_diff_funcs[i], {base_diff_ptr, order_arg},
                                                  "final_sv_diff_" + detail::li_to_string(i));
        sv_diff_f_call->setTailCall(true);
        auto final_sv = builder->CreateFAdd(builder->CreateLoad(sv),
                                            builder->CreateFMul(builder->CreateLoad(h_acc), sv_diff_f_call),
                                            "final_sv_" + detail::li_to_string(i));

        // Store the result into in_out.
        auto out_ptr = builder->CreateInBoundsGEP(in_out_arg, {builder->getInt32(i)});
        builder->CreateStore(final_sv, out_ptr);
    }

    // Finish off the function.
    builder->CreateRetVoid();

    // Verify it.
    verify_function(f);

    // Run the optimization pass.
    // if (opt_level > 0u) {
    //     pm->run(*module);
    // }
}

} // namespace lambdifier
