#include <cstdint>
#include <iostream>
#include <random>

#include <lambdifier/binary_operator.hpp>
#include <lambdifier/function_call.hpp>
#include <lambdifier/llvm_state.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

using namespace lambdifier;

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<unsigned> random_all(0, 3);
static std::uniform_int_distribution<unsigned> random_terminal(0, 1);
static std::uniform_int_distribution<unsigned> random_kernel(2, 3);
static std::uniform_real_distribution<double> rng01(0., 1.);

enum kernel_types { number_t, variable_t, unary_t, binary_t };

lambdifier::expression random_expression(unsigned min_depth, unsigned max_depth, unsigned depth)
{
    unsigned type;
    if (depth < min_depth) {
        type = random_kernel(gen);
    } else if (depth >= max_depth) {
        type = random_terminal(gen);
    } else {
        type = random_all(gen);
    }
    if (type == kernel_types::number_t) {
        return expression{number{3.14}};
    } else if (type == kernel_types::variable_t) {
        return expression{variable{"x"}};
    } else if (type == kernel_types::unary_t) {
        return expression{function_call("llvm.exp", std::vector{random_expression(min_depth, max_depth, depth + 1)})};
    } else if (type == kernel_types::binary_t) {
        return expression{binary_operator{'+', random_expression(min_depth, max_depth, depth + 1),
                                          random_expression(min_depth, max_depth, depth + 1)}};
    }
}

void mutate(lambdifier::expression &ex, double mut_p, unsigned depth)
{
    if (rng01(gen) < mut_p) {
        ex = random_expression(2, 4, depth);
    } else if (auto bo_ptr = ex.extract_unsafe<binary_operator>()) {
        mutate(bo_ptr->access_lhs(), mut_p, depth + 1);
        mutate(bo_ptr->access_rhs(), mut_p, depth + 1);
    } else if (auto call_ptr = ex.extract_unsafe<function_call>()) {
        for (auto &ex : call_ptr->access_args()) {
            mutate(ex, mut_p, depth + 1);
        }
    }
}

int main()
{
    auto ex = random_expression(4, 7, 0);
    std::cout << ex << "\n";
    // Init the LLVM machinery.
    lambdifier::llvm_state s{"my llvm module"};
    s.add_expression("f", ex);
    mutate(ex, 0.3, 0);
    std::cout << ex << "\n";
    s.add_expression("f_mutated", ex);
    std::cout << s.dump() << '\n';

    return 0;
}
