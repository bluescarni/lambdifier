#include <cstddef>
#include <stdexcept>
#include <string>
#include <utility>
#include <unordered_map>


#include <llvm/IR/Value.h>

#include <lambdifier/detail/check_symbol_name.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

namespace lambdifier
{

variable::variable(std::string s) : name(std::move(s))
{
    detail::check_symbol_name(name);
}

variable::variable(const variable &) = default;
variable::variable(variable &&) noexcept = default;

variable::~variable() = default;

llvm::Value *variable::codegen(llvm_state &s) const
{
    auto *v = s.get_named_values()[name];
    if (!v) {
        throw std::invalid_argument("Unknown variable name: " + name);
    }
    return v;
}

std::string variable::get_name() const
{
    return name;
}

std::string variable::to_string() const
{
    return name;
}

double variable::evaluate(std::unordered_map<std::string, double> &values) const {
    return values[name];
}


void variable::set_name(std::string s)
{
    detail::check_symbol_name(s);
    name = std::move(s);
}

expression variable::diff(const std::string &s) const
{
    if (s == name) {
        return expression{number{1}};
    } else {
        return expression{number{0}};
    }
}

inline namespace literals
{

expression operator""_var(const char *s, std::size_t n)
{
    return expression{variable{std::string{s, n}}};
}

} // namespace literals

} // namespace lambdifier
