#ifndef LAMBDIFIER_EXPRESSION_HPP
#define LAMBDIFIER_EXPRESSION_HPP

#include <cassert>
#include <unordered_map>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <llvm/IR/Value.h>

#include <lambdifier/detail/fwd_decl.hpp>
#include <lambdifier/detail/type_traits.hpp>
#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

namespace detail
{

struct LAMBDIFIER_DLL_PUBLIC_INLINE_CLASS expr_inner_base {
    virtual ~expr_inner_base() {}
    virtual std::unique_ptr<expr_inner_base> clone() const = 0;
    virtual llvm::Value *codegen(llvm_state &) const = 0;
    virtual std::string to_string() const = 0;
    virtual double evaluate(std::unordered_map<std::string, double> &) const = 0;
    virtual expression diff(const std::string &) const = 0;
};

template <typename T>
struct LAMBDIFIER_DLL_PUBLIC_INLINE_CLASS expr_inner final : expr_inner_base {
    T m_value;

    // Constructors from T (copy and move variants).
    explicit expr_inner(const T &x) : m_value(x) {}
    explicit expr_inner(T &&x) : m_value(std::move(x)) {}

    expr_inner(const expr_inner &) = delete;
    expr_inner(expr_inner &&) = delete;
    expr_inner &operator=(const expr_inner &) = delete;
    expr_inner &operator=(expr_inner &&) = delete;

    std::unique_ptr<expr_inner_base> clone() const final
    {
        return std::make_unique<expr_inner>(m_value);
    }

    llvm::Value *codegen(llvm_state &s) const final
    {
        return m_value.codegen(s);
    }

    std::string to_string() const final
    {
        return m_value.to_string();
    }

    double evaluate(std::unordered_map<std::string, double> &values) const final
    {
        return m_value.evaluate(values);
    }
    expression diff(const std::string &) const final;
};

} // namespace detail

template <typename T>
concept ud_expr
    = !std::is_same_v<expression,
                      detail::remove_cvref_t<T>> && requires(const detail::remove_cvref_t<T> &t, llvm_state &s)
{
    {
        t.codegen(s)
    }
    ->detail::same_as<llvm::Value *>;
    {
        t.to_string()
    }
    ->detail::same_as<std::string>;
};

class LAMBDIFIER_DLL_PUBLIC expression
{
    std::unique_ptr<detail::expr_inner_base> m_ptr;

public:
    template <ud_expr T>
    explicit expression(T &&x)
        : m_ptr(std::make_unique<detail::expr_inner<detail::remove_cvref_t<T>>>(std::forward<T>(x)))
    {
    }

    expression(const expression &);
    expression(expression &&) noexcept;
    double operator()(std::unordered_map<std::string, double> &) const;

    expression &operator=(const expression &);
    expression &operator=(expression &&) noexcept;

    ~expression();

    llvm::Value *codegen(llvm_state &) const;

    std::string to_string() const;

    expression diff(const std::string &) const;

    bool is_zero() const;
    bool is_one() const;
    bool is_finite_number() const;

private:
    detail::expr_inner_base const *ptr() const
    {
        assert(m_ptr.get() != nullptr);
        return m_ptr.get();
    }
    detail::expr_inner_base *ptr()
    {
        assert(m_ptr.get() != nullptr);
        return m_ptr.get();
    }

public:
    template <typename T>
    const T *extract() const noexcept
    {
        auto p = dynamic_cast<const detail::expr_inner<T> *>(ptr());
        return p == nullptr ? nullptr : &(p->m_value);
    }

    template <typename T>
    T *extract_unsafe() noexcept
    {
        auto p = dynamic_cast<detail::expr_inner<T> *>(ptr());
        return p == nullptr ? nullptr : &(p->m_value);
    }

    std::vector<std::string> get_variables() const;
};

LAMBDIFIER_DLL_PUBLIC expression operator+(expression, expression);
LAMBDIFIER_DLL_PUBLIC expression operator-(expression, expression);
LAMBDIFIER_DLL_PUBLIC expression operator*(expression, expression);
LAMBDIFIER_DLL_PUBLIC expression operator/(expression, expression);

LAMBDIFIER_DLL_PUBLIC expression &operator+=(expression &, expression);
LAMBDIFIER_DLL_PUBLIC expression &operator-=(expression &, expression);
LAMBDIFIER_DLL_PUBLIC expression &operator*=(expression &, expression);
LAMBDIFIER_DLL_PUBLIC expression &operator/=(expression &, expression);

LAMBDIFIER_DLL_PUBLIC expression operator+(expression);
LAMBDIFIER_DLL_PUBLIC expression operator-(expression);

LAMBDIFIER_DLL_PUBLIC std::ostream &operator<<(std::ostream &, const expression &);

namespace detail
{

template <typename T>
concept differentiable_expr = requires(const T &e, const std::string &s)
{
    {
        e.diff(s)
    }
    ->same_as<expression>;
};

template <typename T>
inline expression expr_inner<T>::diff(const std::string &s) const
{
    if constexpr (differentiable_expr<T>) {
        return m_value.diff(s);
    } else {
        // TODO.
        throw std::runtime_error("diff() not implemented");
    }
}

} // namespace detail

} // namespace lambdifier

#endif
