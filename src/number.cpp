#include <string>
#include <unordered_map>

#include <llvm/ADT/APFloat.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Value.h>

#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>
#include <lambdifier/number.hpp>

namespace lambdifier
{

number::number(double value) : value(value) {}

number::number(const number &) = default;
number::number(number &&) noexcept = default;

number::~number() = default;

double number::get_value() const
{
    return value;
}

llvm::Value *number::codegen(llvm_state &s) const
{
    return llvm::ConstantFP::get(s.get_context(), llvm::APFloat(value));
}

std::string number::to_string() const
{
    return std::to_string(value);
}

double number::evaluate(std::unordered_map<std::string, double> &) const
{
    return value;
}

void number::evaluate(std::unordered_map<std::string, std::vector<double>> &, std::vector<double> &out) const
{
    out = std::vector<double>(out.size(), value);
}
expression number::diff(const std::string &) const
{
    return expression{number{0}};
}

void number::set_value(double x)
{
    value = x;
}

inline namespace literals
{

expression operator""_num(long double x)
{
    return expression{number{static_cast<double>(x)}};
}

expression operator""_num(unsigned long long n)
{
    return expression{number{static_cast<double>(n)}};
}

} // namespace literals

} // namespace lambdifier
