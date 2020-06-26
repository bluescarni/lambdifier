#include <cstdint>
#include <iostream>
#include <random>

#include <chrono>
#include <lambdifier/binary_operator.hpp>
#include <lambdifier/function_call.hpp>
#include <lambdifier/llvm_state.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

using namespace lambdifier;

namespace details
{
// This traverses the tree once building the node_connection data structure. The structure contains
// for each node the ids of its input nodes. TODO (preallocate? reserve? right now we push_back)
// The node ids get assigned on the fly by the order of visit (left->right .. args[0]->args[n]), hence node_id is passed
// by ref. The root call has to be done with a node_id = 0.
void _compute_connections(const expression &ex, std::vector<std::vector<unsigned>> &node_connections, unsigned &node_id)
{
    const unsigned node_id_copy = node_id;
    node_id++;
    if (auto bo_ptr = ex.extract<binary_operator>()) {
        node_connections.push_back(std::vector<unsigned>(2));
        node_connections[node_id_copy][0] = node_id;
        _compute_connections(bo_ptr->get_lhs(), node_connections, node_id);
        node_connections[node_id_copy][1] = node_id;
        _compute_connections(bo_ptr->get_rhs(), node_connections, node_id);
    } else if (auto call_ptr = ex.extract<function_call>()) {
        node_connections.push_back(std::vector<unsigned>(call_ptr->get_args().size()));
        for (auto i = 0u; i < call_ptr->get_args().size(); ++i) {
            node_connections[node_id_copy][i] = node_id;
            _compute_connections(call_ptr->get_args()[i], node_connections, node_id);
        }
    } else {
        node_connections.push_back(std::vector<unsigned>());
    }
}
void _forward_pass(const expression &ex, std::vector<double> &node_values,
                  const std::vector<std::vector<unsigned>> &node_connections,
                  std::unordered_map<std::string, double> &in, unsigned &node_id)
{
    // In the forward pass we fill in the output value of each tree node.
    // The node ids get assigned on the fly by the order of visit (left->right .. args[0]->args[n]).
    const unsigned node_id_curr = node_id;
    node_id++;

    // Binary operator
    if (auto bo_ptr = ex.extract<binary_operator>()) {
        // We have to recurse first as to make sure node_values is filled before being accessed later.
        _forward_pass(bo_ptr->get_lhs(), node_values, node_connections, in, node_id);
        _forward_pass(bo_ptr->get_rhs(), node_values, node_connections, in, node_id);
        switch (bo_ptr->get_op()) {
            case '+':
                node_values[node_id_curr]
                    = node_values[node_connections[node_id_curr][0]] + node_values[node_connections[node_id_curr][1]];
                break;

            case '-':
                node_values[node_id_curr]
                    = node_values[node_connections[node_id_curr][0]] - node_values[node_connections[node_id_curr][1]];
                break;
            case '*':
                node_values[node_id_curr]
                    = node_values[node_connections[node_id_curr][0]] * node_values[node_connections[node_id_curr][1]];
                break;

            default:
                assert(bo_ptr->get_op() == '/');
                node_values[node_id_curr]
                    = node_values[node_connections[node_id_curr][0]] / node_values[node_connections[node_id_curr][1]];
                break;
        }
        // Function call
    } else if (auto call_ptr = ex.extract<function_call>()) {
        // We have to recurse first as to make sure node_values is filled before being accessed later.
        for (auto i = 0u; i < call_ptr->get_args().size(); ++i) {
            _forward_pass(call_ptr->get_args()[i], node_values, node_connections, in, node_id);
        }
        std::vector<double> in_values(call_ptr->get_args().size());
        for (auto i = 0u; i < call_ptr->get_args().size(); ++i) {
            in_values[i] = node_values[node_connections[node_id_curr][i]];
        }
        node_values[node_id_curr] = call_ptr->evaluate_num(in_values);
        // Variable
    } else if (auto var_ptr = ex.extract<variable>()) {
        node_values[node_id_curr] = in[var_ptr->get_name()];
        // Number
    } else if (auto num_ptr = ex.extract<number>()) {
        node_values[node_id_curr] = num_ptr->get_value();
    }
    // std::cout << node_id_curr << ": " << ex << " -> " << node_values[node_id_curr] << std::endl; //REMOVE
}

void _backward_pass(const expression &ex, const std::vector<double> &node_values,
                   const std::vector<std::vector<unsigned>> &node_connections,
                   std::unordered_map<std::string, double> &in, std::unordered_map<std::string, double> &gradient,
                   unsigned &node_id, double acc = 1.)
{
    // In the backward pass we simply apply the derivative composition rule from
    // the root node, accumulating the value up to the leaves which will then contain (say) dN0/dx = dN0/dN4 * dN4/dN7 *
    // dN7/dx. Each variable leaf is treated as an independent variable and then the derivative accumulated in the
    // correct gradient position.
    unsigned node_id_curr = node_id;
    // std::cout << node_id << ": " << acc << std::endl; // REMOVE
    node_id++;
    // Variable
    if (auto var_ptr = ex.extract<variable>()) {
        gradient[var_ptr->get_name()] = gradient[var_ptr->get_name()] + acc;
    }
    // Binary Operator
    if (auto bo_ptr = ex.extract<binary_operator>()) {
        switch (bo_ptr->get_op()) {
            case '+': {
                // lhs (a + b -> 1)
                _backward_pass(bo_ptr->get_lhs(), node_values, node_connections, in, gradient, node_id, acc);
                // rhs (a + b -> 1)
                _backward_pass(bo_ptr->get_rhs(), node_values, node_connections, in, gradient, node_id, acc);
                break;
            }
            case '-': {
                // lhs (a - b -> 1)
                _backward_pass(bo_ptr->get_lhs(), node_values, node_connections, in, gradient, node_id, acc);
                // rhs (a - b -> -1)
                _backward_pass(bo_ptr->get_rhs(), node_values, node_connections, in, gradient, node_id, -acc);
                break;
            }
            case '*': {
                // lhs (a*b -> b)
                _backward_pass(bo_ptr->get_lhs(), node_values, node_connections, in, gradient, node_id,
                              acc * node_values[node_connections[node_id_curr][1]]);
                // rhs (a*b -> a)
                _backward_pass(bo_ptr->get_rhs(), node_values, node_connections, in, gradient, node_id,
                              acc * node_values[node_connections[node_id_curr][0]]);
                break;
            }
            default: {
                // lhs (a/b -> 1/b)
                _backward_pass(bo_ptr->get_lhs(), node_values, node_connections, in, gradient, node_id,
                              acc / node_values[node_connections[node_id_curr][1]]);
                // rhs (a/b -> -a/b^2)
                _backward_pass(bo_ptr->get_rhs(), node_values, node_connections, in, gradient, node_id,
                              -acc * node_values[node_connections[node_id_curr][0]]
                                  / node_values[node_connections[node_id_curr][1]]
                                  / node_values[node_connections[node_id_curr][1]]);
                break;
            }
        }
    }
    // Function Call
    if (auto call_ptr = ex.extract<function_call>()) {
        std::vector<double> in_values(call_ptr->get_args().size());
        for (auto i = 0u; i < call_ptr->get_args().size(); ++i) {
            in_values[i] = node_values[node_connections[node_id_curr][i]];
        }
        for (auto i = 0u; i < call_ptr->get_args().size(); ++i) {
            auto value = call_ptr->devaluate_num(in_values, i);
            _backward_pass(call_ptr->get_args()[0], node_values, node_connections, in, gradient, node_id, acc * value);
        }
    }
}
} // namespace details

std::vector<std::vector<unsigned>> compute_connections(const expression &ex)
{
    std::vector<std::vector<unsigned>> retval;
    unsigned node_id = 0;
    details::_compute_connections(ex, retval, node_id);
    return retval;
}

std::unordered_map<std::string, double> gradient(const expression &ex, std::unordered_map<std::string, double> &in, const std::vector<std::vector<unsigned>> &node_connections) {
    std::unordered_map<std::string, double> retval;
    std::vector<double> node_values(node_connections.size());
    unsigned node_id = 0u;
    details::_forward_pass(ex, node_values, node_connections, in, node_id);
    node_id = 0u;
    details::_backward_pass(ex, node_values, node_connections, in, retval, node_id);
    return retval;
}   


// ------------------------------------ Main --------------------------------------
using namespace std::chrono;
int main()
{
    // expression ex = "x"_var * "y"_var / "y"_var + "y"_var * "x"_var;
    expression ex = sin("x"_var * "y"_var) / "y"_var + cos(sin("x"_var / "y"_var) - "x"_var);
    std::unordered_map<std::string, double> in;
    in["x"] = 3.2;
    in["y"] = -0.3;
    auto node_connections = compute_connections(ex);
    //unsigned i=0u;
    //for (const auto &connections : node_connections) {
    //    std::cout << i << ": ";
    //    i++;
    //    for (const auto &connection : connections) {
    //        std::cout << connection << ", ";
    //    }
    //    std::cout << "\n";
    //}
    auto grad = gradient(ex, in, node_connections);
    std::cout << grad["x"] << " " << ex.diff("x")(in) << std::endl;
    std::cout << grad["y"] << " " << ex.diff("y")(in) << std::endl;
    return 0;
}
