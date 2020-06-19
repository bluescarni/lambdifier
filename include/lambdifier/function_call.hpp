#ifndef LAMBDIFIER_FUNCTION_CALL_HPP
#define LAMBDIFIER_FUNCTION_CALL_HPP

#include <string>
#include <vector>

#include <llvm/IR/Value.h>

#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

class LAMBDIFIER_DLL_PUBLIC function_call
{
    std::string name;
    std::vector<expression> args;

public:
    explicit function_call(const std::string &, std::vector<expression>);
    function_call(const function_call &);
    function_call(function_call &&) noexcept;
    ~function_call();

    const std::string &get_name() const;
    const std::vector<expression> &get_args() const;

    llvm::Value *codegen(llvm_state &) const;
    std::string to_string() const;
};

namespace detail
{

class LAMBDIFIER_DLL_PUBLIC function_caller
{
    std::string name;

public:
    explicit function_caller(const std::string &);
    template <typename... Args>
    expression operator()(Args... args) const
    {
        return expression{function_call(name, std::vector<expression>{args...})};
    }
};

} // namespace detail

inline namespace literals
{

LAMBDIFIER_DLL_PUBLIC detail::function_caller operator""_func(const char *, std::size_t);

}

} // namespace lambdifier

#endif
