#ifndef LAMBDIFIER_NUMBER_HPP
#define LAMBDIFIER_NUMBER_HPP

#include <string>

#include <llvm/IR/Value.h>

#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

class LAMBDIFIER_DLL_PUBLIC number
{
    double value;

public:
    explicit number(double);
    number(const number &);
    number(number &&) noexcept;
    ~number();

    double get_value() const;

    llvm::Value *codegen(llvm_state &) const;
    std::string to_string() const;
    double evaluate(std::unordered_map<std::string, double> &) const;

    void set_value(double);
};

inline namespace literals
{

LAMBDIFIER_DLL_PUBLIC expression operator""_num(long double);
LAMBDIFIER_DLL_PUBLIC expression operator""_num(unsigned long long);

} // namespace literals

} // namespace lambdifier

#endif
