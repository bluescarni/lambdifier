#ifndef LAMBDIFIER_VARIABLE_HPP
#define LAMBDIFIER_VARIABLE_HPP

#include <cstddef>
#include <string>

#include <llvm/IR/Value.h>

#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

class LAMBDIFIER_DLL_PUBLIC variable
{
    std::string name;

public:
    explicit variable(std::string);
    variable(const variable &);
    variable(variable &&) noexcept;
    ~variable();

    // Getter/setter for the name.
    std::string get_name() const;
    void set_name(std::string);

    llvm::Value *codegen(llvm_state &) const;
    std::string to_string() const;
    double evaluate(std::unordered_map<std::string, double> &) const;
};

inline namespace literals
{

LAMBDIFIER_DLL_PUBLIC expression operator""_var(const char *, std::size_t);

}

} // namespace lambdifier

#endif
