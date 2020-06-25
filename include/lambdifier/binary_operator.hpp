#ifndef LAMBDIFIER_BINARY_OPERATOR_HPP
#define LAMBDIFIER_BINARY_OPERATOR_HPP

#include <string>
#include <unordered_map>

#include <llvm/IR/Value.h>

#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

class LAMBDIFIER_DLL_PUBLIC binary_operator
{
    char op;
    expression lhs, rhs;

public:
    explicit binary_operator(char, expression, expression);
    binary_operator(const binary_operator &);
    binary_operator(binary_operator &&) noexcept;
    ~binary_operator();

    // Getters/setters.
    const expression &get_lhs() const;
    const expression &get_rhs() const;
    expression &access_lhs();
    expression &access_rhs();
    void set_lhs(expression);
    void set_rhs(expression);
    char get_op() const {return op;};

    llvm::Value *codegen(llvm_state &) const;
    std::string to_string() const;
    double evaluate(std::unordered_map<std::string, double> &) const;
    void evaluate(std::unordered_map<std::string, std::vector<double>> &, std::vector<double> &) const;
    expression diff(const std::string &) const;
};

} // namespace lambdifier

#endif
