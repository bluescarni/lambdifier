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

void forward_pass(const expression &ex, std::vector<double> &node_values,
                  std::vector<std::vector<double>> &node_connections, std::unordered_map<std::string, double> &in,
                  unsigned &node_id)
{
    const unsigned node_id_copy = node_id;
    node_values.push_back(ex(in));
    std::cout << node_id_copy << ": " << ex << " -> " << node_values.back() << std::endl;
    node_id++;

    if (auto bo_ptr = ex.extract<binary_operator>()) {
        node_connections.push_back(std::vector<double>(2));
        node_connections[node_id_copy][0] = node_id;
        forward_pass(bo_ptr->get_lhs(), node_values, node_connections, in, node_id);
        node_connections[node_id_copy][1] = node_id;
        forward_pass(bo_ptr->get_rhs(), node_values, node_connections, in, node_id);
    } else if (auto call_ptr = ex.extract<function_call>()) {
        node_connections.push_back(std::vector<double>(call_ptr->get_args().size()));
        for (auto i = 0u; i < call_ptr->get_args().size(); ++i) {
            node_connections[node_id_copy][i] = node_id;
            forward_pass(call_ptr->get_args()[i], node_values, node_connections, in, node_id);
        }
    } else {
        node_connections.push_back(std::vector<double>());
    }
}

void backward_pass(const expression &ex, const std::vector<double> &node_values,
                   const std::vector<std::vector<double>> &node_connections,
                   std::unordered_map<std::string, double> &in, std::unordered_map<std::string, double> &gradient,
                   unsigned node_id, double acc = 1.)
{
    // Variable
    if (auto var_ptr = ex.extract<variable>()) {
        gradient[var_ptr->get_name()] = gradient[var_ptr->get_name()] + acc;
    }
    // Binary Operator
    if (auto bo_ptr = ex.extract<binary_operator>()) {
        switch (bo_ptr->get_op()) {
            case '+': {

                break;
            }
            case '-': {
                break;
            }
            case '*': {
                // lhs
                backward_pass(bo_ptr->get_lhs(), node_values, node_connections, in, gradient, node_id + 1,
                              acc * node_values[node_connections[node_id][1]]);
                // rhs
                backward_pass(bo_ptr->get_rhs(), node_values, node_connections, in, gradient, node_id + 1,
                              acc * node_values[node_connections[node_id][0]]);
                break;
            }
            default: {
                assert(op == '/');
                // lhs (a/b -> 1/b)
                backward_pass(bo_ptr->get_lhs(), node_values, node_connections, in, gradient, node_id + 1,
                              acc / node_values[node_connections[node_id][1]]);
                // rhs (a/b -> -a/b^2)
                backward_pass(bo_ptr->get_rhs(), node_values, node_connections, in, gradient, node_id + 1,
                              -acc * node_values[node_connections[node_id][0]]
                                  / node_values[node_connections[node_id][1]]
                                  / node_values[node_connections[node_id][1]]);
                break;
            }
        }
    }
    // Function call
    if (auto call_ptr = ex.extract<function_call>()) {
        // In this trivial example we know its sin TODO: make this general
        double d = 

        backward_pass(call_ptr->get_args()[0], node_values, node_connections, in, gradient, node_id + 1,
                      acc * std::cos(node_values[node_connections[node_id][0]]));
    }
}

// ------------------------------------ Main --------------------------------------
using namespace std::chrono;
int main()
{
    expression ex = sin("x"_var * "y"_var) / "y"_var;
    std::vector<double> node_values;
    std::vector<std::vector<double>> node_connections;
    std::unordered_map<std::string, double> in;
    in["x"] = 3.2;
    in["y"] = -0.3;
    unsigned node_id = 0;
    forward_pass(ex, node_values, node_connections, in, node_id);
    unsigned i = 0;
    for (const auto &connections : node_connections) {
        std::cout << i << ": ";
        i++;
        for (const auto &connection : connections) {
            std::cout << connection << ", ";
        }
        std::cout << "\n";
    }
    std::unordered_map<std::string, double> gradient;
    backward_pass(ex, node_values, node_connections, in, gradient, 0);
    std::cout << gradient["x"] << std::endl;
    std::cout << gradient["y"] << std::endl;

    return 0;
}
