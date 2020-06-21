#ifndef LAMBDIFIER_FUNCTION_CALL_HPP
#define LAMBDIFIER_FUNCTION_CALL_HPP

#include <functional>
#include <string>
#include <vector>

#include <llvm/IR/Attributes.h>
#include <llvm/IR/Value.h>

#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

class LAMBDIFIER_DLL_PUBLIC function_call
{
public:
    enum class type { internal, external, builtin };

    using diff_t = std::function<expression(const std::vector<expression> &, const std::string &)>;

private:
    std::string name, display_name;
    std::vector<expression> args;
    std::vector<llvm::Attribute::AttrKind> attributes;
    type ty = type::internal;
    diff_t diff_f;

public:
    explicit function_call(std::string, std::vector<expression>);
    function_call(const function_call &);
    function_call(function_call &&) noexcept;
    ~function_call();

    // Getters.
    const std::string &get_name() const;
    const std::string &get_display_name() const;
    const std::vector<expression> &get_args() const;
    std::vector<expression> &access_args();

    const std::vector<llvm::Attribute::AttrKind> &get_attributes() const;
    type get_type() const;
    diff_t get_diff_f() const;

    // Setters.
    void set_name(std::string);
    void set_display_name(std::string);
    void set_args(std::vector<expression>);
    void set_attributes(std::vector<llvm::Attribute::AttrKind>);
    void set_type(type);
    void set_diff_f(diff_t);

    // Expression interface.
    llvm::Value *codegen(llvm_state &) const;
    std::string to_string() const;
    double evaluate(std::unordered_map<std::string, double> &) const;

    expression diff(const std::string &) const;
};

} // namespace lambdifier

#endif
