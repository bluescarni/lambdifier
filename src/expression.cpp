#include <algorithm>
#include <cmath>
#include <ostream>
#include <string>
#include <utility>
#include <vector>

#include <llvm/IR/Value.h>

#include <lambdifier/binary_operator.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/function_call.hpp>
#include <lambdifier/llvm_state.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

namespace lambdifier
{

expression::expression(const expression &e) : m_ptr(e.m_ptr->clone()) {}

expression::expression(expression &&) noexcept = default;

expression &expression::operator=(expression &&) noexcept = default;

expression &expression::operator=(const expression &other)
{
    if (this != &other) {
        *this = expression(other);
    }
    return *this;
}

expression::~expression() = default;

llvm::Value *expression::codegen(llvm_state &s) const
{
    return m_ptr->codegen(s);
}

std::string expression::to_string() const
{
    return m_ptr->to_string();
}

std::vector<std::string> expression::get_variables() const
{
    std::vector<std::string> retval;

    if (auto bo_ptr = extract<binary_operator>()) {
        const auto l_vars = bo_ptr->get_lhs().get_variables();
        const auto r_vars = bo_ptr->get_rhs().get_variables();

        retval.insert(retval.end(), l_vars.begin(), l_vars.end());
        retval.insert(retval.end(), r_vars.begin(), r_vars.end());
    } else if (auto var_ptr = extract<variable>()) {
        retval.push_back(var_ptr->get_name());
    } else if (auto call_ptr = extract<function_call>()) {
        for (const auto &ex : call_ptr->get_args()) {
            auto vars = ex.get_variables();
            retval.insert(retval.end(), vars.begin(), vars.end());
        }
    }

    std::sort(retval.begin(), retval.end());
    retval.erase(std::unique(retval.begin(), retval.end()), retval.end());

    return retval;
}

expression expression::diff(const std::string &s) const
{
    return m_ptr->diff(s);
}

bool expression::is_zero() const
{
    if (auto num_ptr = extract<number>()) {
        return num_ptr->get_value() == 0.;
    } else {
        return false;
    }
}

bool expression::is_one() const
{
    if (auto num_ptr = extract<number>()) {
        return num_ptr->get_value() == 1.;
    } else {
        return false;
    }
}

bool expression::is_finite_number() const
{
    if (auto num_ptr = extract<number>()) {
        return std::isfinite(num_ptr->get_value());
    } else {
        return false;
    }
}

expression operator+(expression e1, expression e2)
{
    const auto e1_zero = e1.is_zero(), e2_zero = e2.is_zero();
    if (e1_zero) {
        if (e2_zero) {
            // 0 + 0 == 0.
            return expression{number{0}};
        } else {
            // 0 + e2 == e2.
            return e2;
        }
    } else {
        if (e2_zero) {
            // e1 + 0 == e1;
            return e1;
        } else {
            // e1 + e2.
            return expression{binary_operator{'+', std::move(e1), std::move(e2)}};
        }
    }
}

expression operator-(expression e1, expression e2)
{
    const auto e1_zero = e1.is_zero(), e2_zero = e2.is_zero();
    if (e1_zero) {
        if (e2_zero) {
            // 0 - 0 == 0.
            return expression{number{0}};
        } else {
            // 0 - e2 == -e2.
            return -std::move(e2);
        }
    } else {
        if (e2_zero) {
            // e1 - 0 == e1;
            return e1;
        } else {
            // e1 - e2.
            return expression{binary_operator{'-', std::move(e1), std::move(e2)}};
        }
    }
}

expression operator*(expression e1, expression e2)
{
    if ((e1.is_zero() && e2.is_finite_number()) || (e1.is_finite_number() && e2.is_zero())) {
        // 0 * finite == finite * 0 == 0.
        return expression{number{0}};
    } else if (e1.is_one()) {
        // 1 * e2 == e2.
        return e2;
    } else if (e2.is_one()) {
        // e1 * 1 == e1.
        return e1;
    } else {
        return expression{binary_operator{'*', std::move(e1), std::move(e2)}};
    }
}

expression operator/(expression e1, expression e2)
{
    return expression{binary_operator{'/', std::move(e1), std::move(e2)}};
}

expression &operator+=(expression &x, expression e)
{
    return x = std::move(x) + std::move(e);
}

expression &operator-=(expression &x, expression e)
{
    return x = std::move(x) - std::move(e);
}

expression &operator*=(expression &x, expression e)
{
    return x = std::move(x) * std::move(e);
}

expression &operator/=(expression &x, expression e)
{
    return x = std::move(x) / std::move(e);
}

expression operator+(expression e)
{
    return e;
}

expression operator-(expression e)
{
    return expression{number{-1}} * std::move(e);
}

std::ostream &operator<<(std::ostream &os, const expression &e)
{
    return os << e.to_string();
}

} // namespace lambdifier
