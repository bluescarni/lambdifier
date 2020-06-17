#include <cstddef>
#include <stdexcept>
#include <string>

#include <llvm/IR/Value.h>

#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>
#include <lambdifier/variable.hpp>

namespace lambdifier
{

variable::variable(const std::string &name) : name(name) {}

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

inline namespace literals
{

expression operator""_var(const char *s, std::size_t n)
{
    return expression{variable{std::string{s, n}}};
}

} // namespace literals

} // namespace lambdifier
