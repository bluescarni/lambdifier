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

static std::vector<char> allowed_bo = {'+', '-', '*', '/'};
static std::vector<std::string> allowed_func = {"llvm.exp", "llvm.sin"};
static std::vector<double> allowed_numbers = {3.14, -2.34};
static std::vector<std::string> allowed_variables = {"x", "y"};

template <typename Iter, typename rng>
Iter random_element(Iter start, Iter end, rng &g)
{
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

lambdifier::expression random_expression(unsigned min_depth, unsigned max_depth, unsigned depth)
{
    unsigned type;
    if (depth < min_depth) {
        // We get a kernel
        // probability to get any function or a bo is equal
        type = (rng01(gen) < allowed_func.size() / (allowed_func.size() + allowed_bo.size())) ? 2 : 3;
    } else if (depth >= max_depth) {
        // We get a terminal
        // probability to get a terminal with an input variable or a constant is equal
        type = (rng01(gen) < allowed_numbers.size() / (allowed_numbers.size() + allowed_variables.size())) ? 0 : 1;
    } else {
        // We get whatever
        type = random_all(gen);
    }
    switch (type) {
        case kernel_types::number_t:
        {
            auto value = *random_element(allowed_numbers.begin(), allowed_numbers.end(), gen);
            return expression{number{value}};
            break;
        }
        case kernel_types::variable_t:
        {
            auto value = *random_element(allowed_variables.begin(), allowed_variables.end(), gen);
            return expression{variable{value}};
            break;
        }
        case kernel_types::unary_t:
        {
            auto value = *random_element(allowed_func.begin(), allowed_func.end(), gen);
            return expression{function_call(value, std::vector{random_expression(min_depth, max_depth, depth + 1)})};
            break;
        }
        case kernel_types::binary_t:
        {
            auto value = *random_element(allowed_bo.begin(), allowed_bo.end(), gen);
            return expression{binary_operator{value, random_expression(min_depth, max_depth, depth + 1),
                                              random_expression(min_depth, max_depth, depth + 1)}};
            break;
        }
        default:
            throw;
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
