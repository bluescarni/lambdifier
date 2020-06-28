#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

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

} // namespace lambdifier
