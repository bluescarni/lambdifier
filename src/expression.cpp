#include <algorithm>

#include <ostream>
#include <string>
#include <vector>
#include <unordered_map>


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

double expression::operator()(std::unordered_map<std::string, double> &values) const
{
    auto names = get_variables();
    return m_ptr->evaluate(values);
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

expression operator+(const expression &e1, const expression &e2)
{
    return expression{binary_operator{'+', e1, e2}};
}

expression operator-(const expression &e1, const expression &e2)
{
    return expression{binary_operator{'-', e1, e2}};
}

expression operator*(const expression &e1, const expression &e2)
{
    return expression{binary_operator{'*', e1, e2}};
}

expression operator/(const expression &e1, const expression &e2)
{
    return expression{binary_operator{'/', e1, e2}};
}

expression operator+(const expression &e)
{
    return e;
}

expression operator-(const expression &e)
{
    return expression{number{-1}} * e;
}

std::ostream &operator<<(std::ostream &os, const expression &e)
{
    return os << e.to_string();
}

} // namespace lambdifier
