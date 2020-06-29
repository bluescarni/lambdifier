#ifndef LAMBDIFIER_EXPRESSION_HPP
#define LAMBDIFIER_EXPRESSION_HPP

#include <cassert>
#include <memory>
#include <ostream>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <unordered_map>
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
    virtual void evaluate(std::unordered_map<std::string, std::vector<double>> &, std::vector<double> &) const = 0;
    virtual expression diff(const std::string &) const = 0;
    // Auxiliary data structures computations
    virtual void compute_connections(std::vector<std::vector<unsigned>> &, unsigned &) = 0;
    virtual void compute_node_values(std::unordered_map<std::string, double> &, std::vector<double> &,
                                     const std::vector<std::vector<unsigned>> &, unsigned &)
        = 0;
    // Auxiliary algorithms on the expression tree
    virtual void gradient(std::unordered_map<std::string, double> &, std::unordered_map<std::string, double> &,
                          const std::vector<double> &, const std::vector<std::vector<unsigned>> &, unsigned &, double) = 0;
    virtual llvm::Value *taylor_init(llvm_state &, llvm::Value *) const = 0;
};

template <typename T>
concept taylor_expr = requires(const detail::remove_cvref_t<T> &t, llvm_state &s, llvm::Value *arr)
{
    {
        t.taylor_init(s, arr)
    }
    ->detail::same_as<llvm::Value *>;
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

    double evaluate(std::unordered_map<std::string, double> &in) const final
    {
        return m_value.evaluate(in);
    }

    void evaluate(std::unordered_map<std::string, std::vector<double>> &in, std::vector<double> &out) const final
    {
        m_value.evaluate(in, out);
    }

    void compute_connections(std::vector<std::vector<unsigned>> &connections, unsigned &node_counter)
    {
        m_value.compute_connections(connections, node_counter);
    }

    void compute_node_values(std::unordered_map<std::string, double> &in, std::vector<double> &node_values,
                             const std::vector<std::vector<unsigned>> &node_connections, unsigned &node_counter)
    {
        m_value.compute_node_values(in, node_values, node_connections, node_counter);
    }

    void gradient(std::unordered_map<std::string, double> &in, std::unordered_map<std::string, double> &grad,
                  const std::vector<double> &node_values, const std::vector<std::vector<unsigned>> &node_connections,
                  unsigned &node_counter, double acc)
    {
        m_value.gradient(in, grad, node_values, node_connections, node_counter, acc);
    }

    expression diff(const std::string &) const final;

    llvm::Value *taylor_init(llvm_state &s, llvm::Value *arr) const final
    {
        if constexpr (taylor_expr<T>) {
            return m_value.taylor_init(s, arr);
        } else {
            throw std::invalid_argument("The expression '" + to_string()
                                        + "' is not suitable for use in Taylor integration");
        }
    }
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
    // Call operators on double. Normal and batch version
    double operator()(std::unordered_map<std::string, double> &) const;
    void operator()(std::unordered_map<std::string, std::vector<double>> &, std::vector<double> &) const;

    // Computation of auxiliary structures needed for ad_hoc tree algorithms (e.g. AD)
    // Computes the input nodes for each node of the tree
    std::vector<std::vector<unsigned>> compute_connections() const;
    // Computes the input nodes for each node of the tree (version for recursion)
    void compute_connections(std::vector<std::vector<unsigned>> &, unsigned &) const;
    // Computes the output value of each node of the tree
    std::vector<double> compute_node_values(std::unordered_map<std::string, double> &,
                                            const std::vector<std::vector<unsigned>> &) const;
    // Computes the output value of each node of the tree (version for recursion)
    void compute_node_values(std::unordered_map<std::string, double> &, std::vector<double> &,
                             const std::vector<std::vector<unsigned>> &, unsigned &) const;
    // Computes the gradient of the expression w.r.t. all its variables using reverse mode autodiff
    std::unordered_map<std::string, double> gradient(std::unordered_map<std::string, double> &,
                                                     const std::vector<std::vector<unsigned>> &) const;
    // Computes the gradient of the expression w.r.t. all its variables using reverse mode autodiff (version for
    // recursion)
    void gradient(std::unordered_map<std::string, double> &, std::unordered_map<std::string, double> &,
                  const std::vector<double> &, const std::vector<std::vector<unsigned>> &, unsigned &, double = 1.) const;

    expression &operator=(const expression &);
    expression &operator=(expression &&) noexcept;

    ~expression();

    llvm::Value *codegen(llvm_state &) const;

    std::string to_string() const;

    expression diff(const std::string &) const;

    llvm::Value *taylor_init(llvm_state &, llvm::Value *) const;

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
    T *extract() noexcept
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
        throw std::runtime_error("The derivative has not been implemented for the expression '" + to_string() + "'");
    }
}

} // namespace detail

LAMBDIFIER_DLL_PUBLIC std::vector<expression> taylor_decompose(std::vector<expression> v);

} // namespace lambdifier

#endif
