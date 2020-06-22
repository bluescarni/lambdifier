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

static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_int_distribution<unsigned> random_all(0, 3);
static std::uniform_real_distribution<double> rng01(0., 1.);

enum kernel_types { number_t, variable_t, unary_t, binary_t };

static std::vector<char> allowed_bo = {'+', '-', '*', '/'};
static std::vector<expression (*)(expression)> allowed_func
    = {lambdifier::exp, lambdifier::sin, lambdifier::cos, lambdifier::log};
static std::vector<double> allowed_numbers = {3.14, -2.34};
static std::vector<std::string> allowed_variables = {"x", "y"};

template <typename Iter, typename rng>
Iter random_element(Iter start, Iter end, rng &g)
{
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

lambdifier::expression random_expression(unsigned min_depth, unsigned max_depth, unsigned depth = 0u)
{
    unsigned type;
    if (depth < min_depth) {
        // We get a kernel
        // probability to get any function or a bo is equal
        type = (rng01(gen) < allowed_func.size() / (allowed_func.size() + allowed_bo.size())) ? kernel_types::unary_t
                                                                                              : kernel_types::binary_t;
    } else if (depth >= max_depth) {
        // We get a terminal
        // probability to get a terminal with an input variable or a constant is equal
        type = (rng01(gen) < allowed_numbers.size() / (allowed_numbers.size() + allowed_variables.size()))
                   ? kernel_types::number_t
                   : kernel_types::variable_t;
    } else {
        // We get whatever
        type = random_all(gen);
    }
    switch (type) {
        case kernel_types::number_t: {
            auto value = *random_element(allowed_numbers.begin(), allowed_numbers.end(), gen);
            return expression{number{value}};
            break;
        }
        case kernel_types::variable_t: {
            auto value = *random_element(allowed_variables.begin(), allowed_variables.end(), gen);
            return expression{variable{value}};
            break;
        }
        case kernel_types::unary_t: {
            auto value = *random_element(allowed_func.begin(), allowed_func.end(), gen);
            return value(random_expression(min_depth, max_depth, depth + 1));
            break;
        }
        case kernel_types::binary_t: {
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

lambdifier::expression extract_random_subtree(const lambdifier::expression &ex, double cr_p)
{
    if (rng01(gen) < cr_p) {
        return ex;
    } else if (auto bo_ptr = ex.extract<binary_operator>()) {
        return (rng01(gen) < 0.5) ? extract_random_subtree(bo_ptr->get_lhs(), cr_p)
                                  : extract_random_subtree(bo_ptr->get_rhs(), cr_p);
    } else if (auto call_ptr = ex.extract<function_call>()) {
        auto exs = call_ptr->get_args();
        return extract_random_subtree(*random_element(exs.begin(), exs.end(), gen), cr_p);
    }
    return ex;
}

void inject_subtree(lambdifier::expression &ex, const lambdifier::expression &sub_ex, double cr_p)
{
    if (rng01(gen) < cr_p) {
        ex = sub_ex;
    } else if (auto bo_ptr = ex.extract_unsafe<binary_operator>()) {
        (rng01(gen) < 0.5) ? inject_subtree(bo_ptr->access_lhs(), sub_ex, cr_p)
                           : inject_subtree(bo_ptr->access_rhs(), sub_ex, cr_p);
    } else if (auto call_ptr = ex.extract_unsafe<function_call>()) {
        auto exs = call_ptr->access_args();
        inject_subtree(*random_element(exs.begin(), exs.end(), gen), sub_ex, cr_p);
    } else {
        ex = sub_ex;
    }
}

void crossover(lambdifier::expression &ex1, lambdifier::expression &ex2, double cr_p)
{
    auto sub_ex1 = extract_random_subtree(ex1, cr_p);
    auto sub_ex2 = extract_random_subtree(ex2, cr_p);
    inject_subtree(ex1, sub_ex2, cr_p);
    inject_subtree(ex2, sub_ex1, cr_p);
}

std::vector<std::vector<double>> random_args_v(unsigned N, unsigned n)
{
    std::uniform_real_distribution<double> rngm11(-1, 1.);
    std::vector<std::vector<double>> retval(N, std::vector<double>(n, 0u));
    for (auto &vec : retval) {
        for (auto &el : vec) {
            el = rngm11(gen);
        }
    }
    return retval;
}

std::vector<std::unordered_map<std::string, double>> v_to_d(const std::vector<std::vector<double>> &in)
{
    std::vector<std::unordered_map<std::string, double>> retval(in.size());
    for (auto i = 0u; i < in.size(); ++i) {
        retval[i]["x"] = in[i][0];
        retval[i]["y"] = in[i][1];
    }
    return retval;
}

using namespace std::chrono;

int main()
{
    // auto ex1 = random_expression(2, 4);
    // auto ex2 = random_expression(2, 4);
    //
    // std::cout << "ex1: " << ex1 << "\n";
    // std::cout << "ex2: " << ex2 << "\n";
    // crossover(ex1, ex2, 0.3);
    // std::cout << "crossover"
    //          << "\n";
    // std::cout << "ex1: " << ex1 << "\n";
    // std::cout << "ex2: " << ex2 << "\n";
    // std::cout << "numbers:"
    //          << "\n";
    // std::unordered_map<std::string, double> values{{"x", 1.234}, {"y", 0.123}};
    // std::cout << "ex1: " << ex1(values) << "\n";
    // std::cout << "ex2: " << ex2(values) << "\n";

    // Init the LLVM machinery.
    lambdifier::llvm_state s{"optimized"};
    auto ex = random_expression(3, 10);
    // We force both variables in.
    while (ex.get_variables().size() < 2u) {
        ex = random_expression(3, 10);
    };
    std::cout << "ex: " << ex << "\n";
    s.add_expression("f", ex, true);

    // 1 - We time the compilation into llvm
    auto start = high_resolution_clock::now();
    s.compile();
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    std::cout << "Time to compile the expression (microseconds): " << duration.count() << "\n";
    // 2 - we time the function call from llvm
    unsigned N = 10000u;
    auto func = s.fetch("f");
    auto args_v = random_args_v(N, 2u);
    start = high_resolution_clock::now();
    for (auto &args : args_v) {
        func(args.data());
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "Time to 10000 call of compiled expression (microseconds): " << duration.count() << "\n";
    // 3 - we time the function call from evaluate
    auto args_d = v_to_d(args_v);
    start = high_resolution_clock::now();
    for (auto &args : args_d) {
        ex(args);
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "Time to 10000 evaluation of the tree (microseconds): " << duration.count() << "\n";
    return 0;
}