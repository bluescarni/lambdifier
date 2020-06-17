#ifndef LAMBDIFIER_EXPRESSION_HPP
#define LAMBDIFIER_EXPRESSION_HPP

#include <cassert>
#include <memory>
#include <ostream>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

#include <llvm/IR/Value.h>

#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

namespace detail
{

template <typename T>
using remove_cvref_t = std::remove_cv_t<std::remove_reference_t<T>>;

template <class T, class U>
concept same_helper = std::is_same_v<T, U>;

template <class T, class U>
concept same_as = same_helper<T, U> &&same_helper<U, T>;

struct LAMBDIFIER_DLL_PUBLIC_INLINE_CLASS expr_inner_base {
    virtual ~expr_inner_base() {}
    virtual std::unique_ptr<expr_inner_base> clone() const = 0;
    virtual llvm::Value *codegen(llvm_state &) const = 0;
    virtual std::string to_string() const = 0;
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
};

} // namespace detail

class expression;

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

    expression &operator=(const expression &);
    expression &operator=(expression &&) noexcept;

    ~expression();

    llvm::Value *codegen(llvm_state &) const;

    std::string to_string() const;

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

    std::vector<std::string> get_variables() const;
};

LAMBDIFIER_DLL_PUBLIC expression operator+(const expression &, const expression &);
LAMBDIFIER_DLL_PUBLIC expression operator-(const expression &, const expression &);
LAMBDIFIER_DLL_PUBLIC expression operator*(const expression &, const expression &);
LAMBDIFIER_DLL_PUBLIC expression operator/(const expression &, const expression &);

LAMBDIFIER_DLL_PUBLIC expression operator+(const expression &);
LAMBDIFIER_DLL_PUBLIC expression operator-(const expression &);

LAMBDIFIER_DLL_PUBLIC std::ostream &operator<<(std::ostream &, const expression &);

} // namespace lambdifier

#endif
