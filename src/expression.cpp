#include <algorithm>
#include <ostream>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include <iostream>


#include <llvm/IR/Value.h>

#include <lambdifier/binary_operator.hpp>
#include <lambdifier/detail/string_conv.hpp>
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

bool expression::operator==(const expression &other) const
{
    if (auto bo_ptr = extract<binary_operator>()) {
        if (auto bo_ptr_other = other.extract<binary_operator>()) {
            switch (bo_ptr->get_op()) {
                case '+':
                    if (bo_ptr_other->get_op() == '+') {
                        return (bo_ptr->get_lhs() == bo_ptr_other->get_lhs())
                               && (bo_ptr->get_rhs() == bo_ptr_other->get_rhs());
                    } else {
                        return false;
                    }
                    break;
                case '-':
                    if (bo_ptr_other->get_op() == '-') {
                        return (bo_ptr->get_lhs() == bo_ptr_other->get_lhs())
                               && (bo_ptr->get_rhs() == bo_ptr_other->get_rhs());
                    } else {
                        return false;
                    }
                    break;
                case '*':
                    if (bo_ptr_other->get_op() == '*') {
                        return (bo_ptr->get_lhs() == bo_ptr_other->get_lhs())
                               && (bo_ptr->get_rhs() == bo_ptr_other->get_rhs());
                    } else {
                        return false;
                    }
                    break;
                default:
                    assert(bo_ptr->get_op() == '/');
                    if (bo_ptr_other->get_op() == '/') {
                        return (bo_ptr->get_lhs() == bo_ptr_other->get_lhs())
                               && (bo_ptr->get_rhs() == bo_ptr_other->get_rhs());
                    } else {
                        return false;
                    }
                    break;
            }
        } else {
            return false;
        }
    } else if (auto fun_ptr = extract<function_call>()) {
        if (auto fun_ptr_other = other.extract<function_call>()) {
            if (fun_ptr->get_name() == fun_ptr_other->get_name()) {
                if (fun_ptr->get_args().size() == fun_ptr_other->get_args().size()) {
                    return std::equal(fun_ptr->get_args().begin(), fun_ptr->get_args().end(),
                                      fun_ptr_other->get_args().begin());
                } else {
                    return false;
                }
            } else {
                return false;
            }
        } else {
            return false;
        }
    } else if (auto num_ptr = extract<number>()) {
        if (auto num_ptr_other = other.extract<number>()) {
            return num_ptr->get_value() == num_ptr_other->get_value();
        } else {
            return false;
        }
    } else if (auto var_ptr = extract<variable>()) {
        if (auto var_ptr_other = other.extract<variable>()) {
            return var_ptr->get_name() == var_ptr_other->get_name();
        } else {
            return false;
        }
    } else {
        // throw?
        return false;
    }
}

bool expression::operator!=(const expression &other) const
{
    return !(*this == other);
}

std::string expression::to_string() const
{
    return m_ptr->to_string();
}

double expression::operator()(std::unordered_map<std::string, double> &in) const
{
    return m_ptr->evaluate(in);
}

void expression::operator()(std::unordered_map<std::string, std::vector<double>> &in, std::vector<double> &out) const
{
    // We first resize out (TODO: a check on the consistency of the in sizes?)
    decltype((*in.begin()).second.size()) n;
    n = (in.empty()) ? 0u : (*in.begin()).second.size();
    out.resize(n);
    return m_ptr->evaluate(in, out);
}

std::vector<std::vector<unsigned>> expression::compute_connections() const
{
    std::vector<std::vector<unsigned>> retval;
    unsigned node_counter = 0u;
    compute_connections(retval, node_counter);
    return retval;
}
void expression::compute_connections(std::vector<std::vector<unsigned>> &node_connections, unsigned &node_counter) const
{
    return m_ptr->compute_connections(node_connections, node_counter);
}
std::vector<double> expression::compute_node_values(std::unordered_map<std::string, double> &in,
                                                    const std::vector<std::vector<unsigned>> &node_connections) const
{
    std::vector<double> node_values(node_connections.size());
    unsigned node_counter = 0u;
    compute_node_values(in, node_values, node_connections, node_counter);
    return node_values;
}
void expression::compute_node_values(std::unordered_map<std::string, double> &in, std::vector<double> &node_values,
                                     const std::vector<std::vector<unsigned>> &node_connections,
                                     unsigned &node_counter) const
{
    return m_ptr->compute_node_values(in, node_values, node_connections, node_counter);
}

std::unordered_map<std::string, double>
expression::gradient(std::unordered_map<std::string, double> &in,
                     const std::vector<std::vector<unsigned>> &node_connections) const
{
    std::unordered_map<std::string, double> grad;
    auto node_values = compute_node_values(in, node_connections);
    auto node_counter = 0u;
    gradient(in, grad, node_values, node_connections, node_counter);
    return grad;
}

void expression::gradient(std::unordered_map<std::string, double> &in, std::unordered_map<std::string, double> &grad,
                          const std::vector<double> &node_values,
                          const std::vector<std::vector<unsigned>> &node_connections, unsigned &node_counter,
                          double acc) const
{
    return m_ptr->gradient(in, grad, node_values, node_connections, node_counter, acc);
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
    // TODO: should we error out here?

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
    if (auto e1_nptr = e1.extract<number>(), e2_nptr = e2.extract<number>(); e1_nptr && e2_nptr) {
        // Both are numbers, divide them.
        return expression{number{e1_nptr->get_value() / e2_nptr->get_value()}};
    } else if (e2_nptr) {
        // e1 is symbolic, e2 a number.
        if (e2_nptr->get_value() == 1) {
            // symbolic / 1 = symbolic.
            return e1;
        } else if (e2_nptr->get_value() == -1) {
            // symbolic / -1 = -symbolic.
            return -std::move(e1);
        } else {
            // symbolic / x = symbolic * 1/x.
            return std::move(e1) * expression{number{1 / e2_nptr->get_value()}};
        }
    } else {
        // The standard case.
        return expression{binary_operator{'/', std::move(e1), std::move(e2)}};
    }
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

namespace detail
{

namespace
{

void rename_ex_variables(expression &ex, const std::unordered_map<std::string, std::string> &repl_map)
{
    if (auto bo_ptr = ex.extract<binary_operator>()) {
        rename_ex_variables(bo_ptr->access_lhs(), repl_map);
        rename_ex_variables(bo_ptr->access_rhs(), repl_map);
    } else if (auto var_ptr = ex.extract<variable>()) {
        if (auto it = repl_map.find(var_ptr->get_name()); it != repl_map.end()) {
            var_ptr->set_name(it->second);
        }
    } else if (auto call_ptr = ex.extract<function_call>()) {
        for (auto &arg_ex : call_ptr->access_args()) {
            rename_ex_variables(arg_ex, repl_map);
        }
    }
    // TODO: should we error out here?
}

// Transform in-place ex by decomposition, appending the
// result of the decomposition to u_vars_defs.
// NOTE: this will render ex unusable.
void taylor_decompose_ex(expression &ex, std::vector<expression> &u_vars_defs)
{
    if (ex.extract<variable>() != nullptr || ex.extract<number>() != nullptr) {
        // NOTE: an expression does *not* require decomposition
        // if it is a variable or a number.
        return;
    } else if (auto bo_ptr = ex.extract<binary_operator>()) {
        // Variables to track how the size
        // of u_vars_defs changes after the decomposition
        // of lhs and rhs.
        auto old_size = u_vars_defs.size(), new_size = old_size;

        // We decompose the lhs, and we check if the
        // decomposition added new elements to u_vars_defs.
        // If it did, then it means that the lhs required
        // further decompositions and the creation of new
        // u vars: the new lhs will become the last added
        // u variable. If it did not, it means that the lhs
        // was a variable or a number (see above), and thus
        // we can use it as-is.
        taylor_decompose_ex(bo_ptr->access_lhs(), u_vars_defs);
        new_size = u_vars_defs.size();
        if (new_size > old_size) {
            bo_ptr->access_lhs() = expression{variable{"u_" + detail::li_to_string(new_size - 1u)}};
        }
        old_size = new_size;

        // Same for the rhs.
        taylor_decompose_ex(bo_ptr->access_rhs(), u_vars_defs);
        new_size = u_vars_defs.size();
        if (new_size > old_size) {
            bo_ptr->access_rhs() = expression{variable{"u_" + detail::li_to_string(new_size - 1u)}};
        }

        u_vars_defs.emplace_back(std::move(*bo_ptr));
    } else if (auto func_ptr = ex.extract<function_call>()) {
        // The function call treatment is a generalization
        // of the binary operator.
        auto old_size = u_vars_defs.size(), new_size = old_size;

        for (auto &arg : func_ptr->access_args()) {
            taylor_decompose_ex(arg, u_vars_defs);
            new_size = u_vars_defs.size();
            if (new_size > old_size) {
                arg = expression{variable{"u_" + detail::li_to_string(new_size - 1u)}};
            }
            old_size = new_size;
        }

        u_vars_defs.emplace_back(std::move(*func_ptr));
    }
    // TODO: should we error out here?
}

} // namespace

} // namespace detail

std::vector<expression> taylor_decompose(std::vector<expression> v_ex)
{
    // Determine the variables in the system of equations.
    std::vector<std::string> vars;
    for (const auto &ex : v_ex) {
        const auto ex_vars = ex.get_variables();
        vars.insert(vars.end(), ex_vars.begin(), ex_vars.end());
        std::sort(vars.begin(), vars.end());
        vars.erase(std::unique(vars.begin(), vars.end()), vars.end());
    }

    if (vars.size() != v_ex.size()) {
        throw std::invalid_argument("The number of variables (" + std::to_string(vars.size())
                                    + ") differs from the number of equations (" + std::to_string(v_ex.size()) + ")");
    }

    // Create the map for renaming the variables to u_i.
    std::unordered_map<std::string, std::string> repl_map;
    for (decltype(vars.size()) i = 0; i < vars.size(); ++i) {
        repl_map.emplace(vars[i], "u_" + detail::li_to_string(i));
    }

    // Rename the variables in the original equations..
    for (auto &ex : v_ex) {
        detail::rename_ex_variables(ex, repl_map);
    }

    // Init the vector containing the definitions
    // of the u variables. It initially contains
    // just the variables of the system.
    std::vector<expression> u_vars_defs;
    for (const auto &var : vars) {
        u_vars_defs.emplace_back(variable{var});
    }

    // Create a copy of the original equations in terms of u variables.
    // We will be reusing this below.
    auto v_ex_copy = v_ex;

    // Decompose the equations.
    for (decltype(v_ex.size()) i = 0; i < v_ex.size(); ++i) {
        const auto orig_size = u_vars_defs.size();
        detail::taylor_decompose_ex(v_ex[i], u_vars_defs);
        if (u_vars_defs.size() != orig_size) {
            // NOTE: if the size of u_vars_defs changes,
            // it means we had to decompose v_ex[i]. In such
            // case, we replace the original definition of the
            // expression with its definition in terms of the
            // last u variable added in the decomposition.
            // In the other case, v_ex_copy will keep on containing
            // the original definition of v_ex[i] in terms
            // of u variables.
            v_ex_copy[i] = expression{variable{"u_" + detail::li_to_string(u_vars_defs.size() - 1u)}};
        }
    }

    // Append the (possibly new) definitions of the diff equations
    // in terms of u variables.
    for (auto &ex : v_ex_copy) {
        u_vars_defs.emplace_back(std::move(ex));
    }

    return u_vars_defs;
}

llvm::Value *expression::taylor_init(llvm_state &s, llvm::Value *arr) const
{
    return m_ptr->taylor_init(s, arr);
}

} // namespace lambdifier
