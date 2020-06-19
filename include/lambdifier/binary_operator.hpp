#ifndef LAMBDIFIER_BINARY_OPERATOR_HPP
#define LAMBDIFIER_BINARY_OPERATOR_HPP

#include <string>

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
    explicit binary_operator(char, const expression &, const expression &);
    binary_operator(const binary_operator &);
    binary_operator(binary_operator &&) noexcept;
    ~binary_operator();

    const expression &get_lhs() const;
    const expression &get_rhs() const;
    expression &access_lhs();
    expression &access_rhs();
    void set_lhs(expression);
    void set_rhs(expression);

    llvm::Value *codegen(llvm_state &) const;
    std::string to_string() const;
};

} // namespace lambdifier

#endif
