#include <algorithm>
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

expression expression::diff(const std::string &s) const
{
    return m_ptr->diff(s);
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

expression operator+(expression e1, expression e2)
{
    if (auto e1_nptr = e1.extract<number>(), e2_nptr = e2.extract<number>(); e1_nptr && e2_nptr) {
        // Both are numbers, add them.
        return expression{number{e1_nptr->get_value() + e2_nptr->get_value()}};
    } else if (e1_nptr && e1_nptr->get_value() == 0) {
        // e1 zero, e2 smybolic.
        return e2;
    } else if (e2_nptr && e2_nptr->get_value() == 0) {
        // e2 zero, e1 smybolic.
        return e1;
    } else {
        // The standard case.
        return expression{binary_operator{'+', std::move(e1), std::move(e2)}};
    }
}

expression operator-(expression e1, expression e2)
{
    if (auto e1_nptr = e1.extract<number>(), e2_nptr = e2.extract<number>(); e1_nptr && e2_nptr) {
        // Both are numbers, subtract them.
        return expression{number{e1_nptr->get_value() - e2_nptr->get_value()}};
    } else if (e1_nptr && e1_nptr->get_value() == 0) {
        // e1 zero, e2 smybolic.
        return -std::move(e2);
    } else if (e2_nptr && e2_nptr->get_value() == 0) {
        // e2 zero, e1 smybolic.
        return e1;
    } else {
        // The standard case.
        return expression{binary_operator{'-', std::move(e1), std::move(e2)}};
    }
}

expression operator*(expression e1, expression e2)
{
    if (auto e1_nptr = e1.extract<number>(), e2_nptr = e2.extract<number>(); e1_nptr && e2_nptr) {
        // Both are numbers, multiply them.
        return expression{number{e1_nptr->get_value() * e2_nptr->get_value()}};
    } else if (e1_nptr) {
        if (e1_nptr->get_value() == 0) {
            // 0 * symbolic = 0.
            return expression{number{0}};
        } else if (e1_nptr->get_value() == 1) {
            // 1 * symbolic = symbolic.
            return e2;
        }
        // NOTE: if we get here, it means that e1 is a
        // number different from 0 or 1. We will fall through
        // the standard case.
    } else if (e2_nptr) {
        if (e2_nptr->get_value() == 0) {
            // symbolic * 0 = 0.
            return expression{number{0}};
        } else if (e2_nptr->get_value() == 1) {
            // symbolic * 1 = symbolic.
            return e1;
        }
        // NOTE: if we get here, it means that e2 is a
        // number different from 0 or 1. We will fall through
        // the standard case.
    }
    // The standard case.
    return expression{binary_operator{'*', std::move(e1), std::move(e2)}};
}

expression operator/(expression e1, expression e2)
{
    // TODO: division simplifications.
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
