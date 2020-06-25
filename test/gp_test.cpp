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

// The seed is fixed as to get always the same expression
static std::mt19937 gen(12345u);
static std::uniform_int_distribution<unsigned> random_all(0, 3);
static std::uniform_real_distribution<double> rng01(0., 1.);

enum kernel_types { number_t, variable_t, binary_t, unary_t };

static std::vector<char> allowed_bo = {'+', '-', '*', '/'};
static std::vector<expression (*)(expression)> allowed_func = {lambdifier::exp, lambdifier::sin, lambdifier::cos};
static std::vector<double> allowed_numbers = {3.14, -2.34};
static std::vector<std::string> allowed_variables = {"x", "y"};

// ------------------------------------SOME HELPER FUNCTIONS --------------------------------------
template <typename Iter, typename rng>
Iter random_element(Iter start, Iter end, rng &g)
{
    std::uniform_int_distribution<> dis(0, std::distance(start, end) - 1);
    std::advance(start, dis(g));
    return start;
}

std::vector<std::vector<double>> random_args_vv(unsigned N, unsigned n)
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

std::vector<double> random_args_batch(unsigned nvars, unsigned batch_size)
{
    std::uniform_real_distribution<double> rngm11(-1, 1.);
    std::vector<double> retval(nvars * batch_size);
    for (auto &x : retval) {
        x = rngm11(gen);
    }
    return retval;
}

std::vector<std::unordered_map<std::string, double>> vv_to_vd(const std::vector<std::vector<double>> &in)
{
    std::vector<std::unordered_map<std::string, double>> retval(in.size());
    for (auto i = 0u; i < in.size(); ++i) {
        retval[i]["x"] = in[i][0];
        retval[i]["y"] = in[i][1];
    }
    return retval;
}

std::unordered_map<std::string, std::vector<double>> vv_to_dv(const std::vector<std::vector<double>> &in)
{
    std::unordered_map<std::string, std::vector<double>> retval;
    std::vector<double> x_vec(in.size()), y_vec(in.size());
    for (auto i = 0u; i < in.size(); ++i) {
        x_vec[i] = in[i][0];
        y_vec[i] = in[i][1];
    }
    retval["x"] = x_vec;
    retval["y"] = y_vec;
    return retval;
}

// ------------------------------------Basic GP classes--------------------------------------
expression random_expression(unsigned min_depth, unsigned max_depth, unsigned depth = 0u)
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

void mutate(expression &ex, double mut_p, unsigned depth)
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

expression extract_random_subtree(const expression &ex, double cr_p)
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

void inject_subtree(expression &ex, const expression &sub_ex, double cr_p)
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

void crossover(expression &ex1, expression &ex2, double cr_p)
{
    auto sub_ex1 = extract_random_subtree(ex1, cr_p);
    auto sub_ex2 = extract_random_subtree(ex2, cr_p);
    inject_subtree(ex1, sub_ex2, cr_p);
    inject_subtree(ex2, sub_ex1, cr_p);
}

// ------------------------------------ Main --------------------------------------
using namespace std::chrono;
int main()
{
    // Init the LLVM machinery.
    lambdifier::llvm_state s{"optimized"};
    auto ex = random_expression(3, 6);
    // We force both variables in.
    while (ex.get_variables().size() < 2u) {
        ex = random_expression(3, 6);
    };
    // Uncomment for simpler expression.
    // ex = "x"_var * "x"_var + "y"_var + "y"_var * "y"_var - "y"_var * "x"_var;
    std::cout << "ex: " << ex << "\n";
    s.add_expression("f", ex, 10000);
    s.add_expression("g", ex, 20);
    std::cout << s.dump() << '\n';

    // 1 - We time the compilation into llvm
    auto start = high_resolution_clock::now();
    s.compile();
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    std::cout << "Time to compile the expression (microseconds): " << duration.count() << "\n";

    // 2 - we time the function call from llvm
    unsigned N = 10000u;
    double res;
    auto func = s.fetch("f");
    auto args_vv = random_args_vv(N, 2u);
    start = high_resolution_clock::now();
    for (auto &args : args_vv) {
        res = func(args.data());
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "Millions of evaluations per second (llvm): " << 1. / (static_cast<double>(duration.count()) / N)
              << "M\n";

    // 3 - we time the function call from evaluate
    auto args_vd = vv_to_vd(args_vv);
    start = high_resolution_clock::now();
    for (auto &args : args_vd) {
        res = ex(args);
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "Millions of evaluations per second (tree): " << 1. / (static_cast<double>(duration.count()) / N)
              << "M\n";

    // 4 - we time the function call from evaluate_batch
    auto args_dv = vv_to_dv(args_vv);
    std::vector<double> out(10000, 0.12345);
    start = high_resolution_clock::now();

    ex(args_dv, out);
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "Millions of evaluations per second (tree in one batch): "
              << 1. / (static_cast<double>(duration.count()) / N) << "M\n";

    // 5 - we time the function call from evaluate_batch (size 200)
    std::vector<std::unordered_map<std::string, std::vector<double>>> args_dv_batches(10000 / 200);
    for (auto i = 0u; i < args_dv_batches.size(); ++i) {
        std::vector<std::vector<double>> tmp(args_vv.begin() + 200 * i, args_vv.begin() + 200 * (i + 1));
        args_dv_batches[i] = vv_to_dv(tmp);
    }
    out = std::vector<double>(200, 0.123);
    start = high_resolution_clock::now();
    for (auto i = 0u; i < args_dv_batches.size(); ++i) {
        ex(args_dv_batches[i], out);
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "Millions of evaluations per second (tree in batches of 200): "
              << 1. / (static_cast<double>(duration.count()) / N) << "M\n";

    // 6 - we time the function call from llvm batch
    auto func_batch = s.fetch_batch("f");
    out = std::vector<double>(10000, 0.12345);
    auto llvm_batch_args = random_args_batch(2, 10000);
    start = high_resolution_clock::now();
    func_batch(out.data(), llvm_batch_args.data());
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "Millions of evaluations per second (llvm batch 10000): "
              << 1. / (static_cast<double>(duration.count()) / N) << "M\n";

    // 7 - we time the function call from llvm batch (20).
    auto g_batch = s.fetch_batch("g");
    start = high_resolution_clock::now();
    for (auto i = 0u; i < 500u; ++i) {
        g_batch(out.data() + i * 20, llvm_batch_args.data() + 2 * i * 20);
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "Millions of evaluations per second (llvm batch 20): "
              << 1. / (static_cast<double>(duration.count()) / N) << "M\n";

    return 0;
}
