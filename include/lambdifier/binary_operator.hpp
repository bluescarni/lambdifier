#ifndef LAMBDIFIER_BINARY_OPERATOR_HPP
#define LAMBDIFIER_BINARY_OPERATOR_HPP

#include <algorithm>
#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

#include <llvm/IR/Value.h>

#include <lambdifier/detail/visibility.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

class LAMBDIFIER_DLL_PUBLIC binary_operator
{
    char op;
    expression lhs, rhs;

public:
    explicit binary_operator(char, expression, expression);
    binary_operator(const binary_operator &);
    binary_operator(binary_operator &&) noexcept;
    ~binary_operator();

    // Getters/setters.
    const expression &get_lhs() const;
    const expression &get_rhs() const;
    expression &access_lhs();
    expression &access_rhs();
    void set_lhs(expression);
    void set_rhs(expression);
    char get_op() const
    {
        return op;
    };

    llvm::Value *codegen(llvm_state &) const;
    std::string to_string() const;
    double evaluate(std::unordered_map<std::string, double> &) const;
    void evaluate(std::unordered_map<std::string, std::vector<double>> &, std::vector<double> &) const;
    void compute_connections(std::vector<std::vector<unsigned>> &, unsigned &) const;
    void compute_node_values(std::unordered_map<std::string, double> &in, std::vector<double> &node_values,
                             const std::vector<std::vector<unsigned>> &node_connections, unsigned &node_counter) const
    {
        const unsigned node_id = node_counter;
        node_counter++;
        // We have to recurse first as to make sure out is filled before being accessed later.
        get_lhs().compute_node_values(in, node_values, node_connections, node_counter);
        get_rhs().compute_node_values(in, node_values, node_connections, node_counter);
        switch (get_op()) {
            case '+':
                node_values[node_id]
                    = node_values[node_connections[node_id][0]] + node_values[node_connections[node_id][1]];
                break;
            case '-':
                node_values[node_id]
                    = node_values[node_connections[node_id][0]] - node_values[node_connections[node_id][1]];
                break;
            case '*':
                node_values[node_id]
                    = node_values[node_connections[node_id][0]] * node_values[node_connections[node_id][1]];
                break;
            default:
                assert(bo_ptr->get_op() == '/');
                node_values[node_id]
                    = node_values[node_connections[node_id][0]] / node_values[node_connections[node_id][1]];
                break;
        }
    }
    void gradient(std::unordered_map<std::string, double> &in, std::unordered_map<std::string, double> &grad,
                  const std::vector<double> &node_values, const std::vector<std::vector<unsigned>> &node_connections,
                  unsigned &node_counter, double acc)
    {
        const unsigned node_id = node_counter;
        node_counter++;
        switch (get_op()) {
            case '+': {
                // lhs (a + b -> 1)
                get_lhs().gradient(in, grad, node_values, node_connections, node_counter, acc);
                // rhs (a + b -> 1)
                get_rhs().gradient(in, grad, node_values, node_connections, node_counter, acc);
                break;
            }
            case '-': {
                // lhs (a + b -> 1)
                get_lhs().gradient(in, grad, node_values, node_connections, node_counter, acc);
                // rhs (a + b -> 1)
                get_rhs().gradient(in, grad, node_values, node_connections, node_counter, -acc);
                break;
            }
            case '*': {
                // lhs (a*b -> b)
                get_lhs().gradient(in, grad, node_values, node_connections, node_counter,
                                   acc * node_values[node_connections[node_id][1]]);
                // rhs (a*b -> a)
                get_rhs().gradient(in, grad, node_values, node_connections, node_counter,
                                   acc * node_values[node_connections[node_id][0]]);
                break;
            }
            default: {
                // lhs (a/b -> 1/b)
                get_lhs().gradient(in, grad, node_values, node_connections, node_counter,
                                   acc / node_values[node_connections[node_id][1]]);
                // rhs (a/b -> -a/b^2)
                get_rhs().gradient(in, grad, node_values, node_connections, node_counter,
                                   -acc * node_values[node_connections[node_id][0]]
                                       / node_values[node_connections[node_id][1]]
                                       / node_values[node_connections[node_id][1]]);
                break;
            }
        }
    }
    expression diff(const std::string &) const;
};

} // namespace lambdifier

#endif
