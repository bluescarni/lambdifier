#include <chrono>
#include <cstdint>
#include <iostream>
#include <random>

#include <lambdifier/autodiff.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

using namespace lambdifier;

static std::mt19937 gen(12345u);

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

std::vector<std::unordered_map<std::string, double>> vv_to_vd(const std::vector<std::vector<double>> &in)
{
    std::vector<std::unordered_map<std::string, double>> retval(in.size());
    for (auto i = 0u; i < in.size(); ++i) {
        retval[i]["x"] = in[i][0];
        retval[i]["y"] = in[i][1];
        retval[i]["z"] = in[i][2];
    }
    return retval;
}

// ------------------------------------ Main --------------------------------------
using namespace std::chrono;
int main()
{
    // Size of thest data.
    unsigned N = 10000u;
    // expression ex = "x"_var * "y"_var / "y"_var + "y"_var * "x"_var;
    expression ex
        = (sin(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var)
               * sin(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var)
           + pow(cos(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var), expression{number{2}}))
              * (sin(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var)
                     * sin(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var)
                 + pow(cos(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var), expression{number{2}}))
          - (sin(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var)
                 * sin(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var)
             + pow(cos(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var), expression{number{2}}))
                / (sin(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var)
                       * sin(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var)
                   + pow(cos(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var), expression{number{2}}))
          + cos((sin(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var)
                     * sin(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var)
                 + pow(cos(cos("x"_var) * "y"_var / expression{number{2.23}} / "z"_var), expression{number{2}})));
    auto node_connections = compute_connections(ex);
    auto args_vv = random_args_vv(N, 3u);
    auto args_vd = vv_to_vd(args_vv);
std::cout << node_connections.size() << std::endl;
    // 1 - We time the autodiff
    auto start = high_resolution_clock::now();
    for (auto &args : args_vd) {
        auto grad = gradient(ex, args, node_connections);
    }
    auto stop = high_resolution_clock::now();
    auto duration = duration_cast<microseconds>(stop - start);
    std::cout << "Millions of gradients per second (reverse autodiff): " << N / static_cast<double>(duration.count())
              << "M\n";

    // 2 - We time the symboilic differentiation plus evaluate
    start = high_resolution_clock::now();
    auto dx = ex.diff("x");
    auto dy = ex.diff("y");
    auto dz = ex.diff("z");
    for (auto &args : args_vd) {
        auto dx(args);
        auto dy(args);
        auto dz(args);
    }
    stop = high_resolution_clock::now();
    duration = duration_cast<microseconds>(stop - start);
    std::cout << "Millions of gradients per second (symbolic diff and evaluate): "
              << 1. / (static_cast<double>(duration.count()) / N) << "M\n";

    // std::cout << grad["x"] << " " << ex.diff("x")(in) << std::endl;
    // std::cout << grad["y"] << " " << ex.diff("y")(in) << std::endl;
    // std::cout << grad["z"] << " " << ex.diff("z")(in) << std::endl;

    return 0;
}
