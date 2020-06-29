#include <cassert>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include <llvm/IR/Value.h>

#include <lambdifier/binary_operator.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

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

} // namespace lambdifier
