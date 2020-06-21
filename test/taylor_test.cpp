#include <cstdint>
#include <iostream>

#include <lambdifier/llvm_state.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/variable.hpp>
#include <lambdifier/number.hpp>


using namespace lambdifier;

int main()
{
    // Init the LLVM machinery.
    lambdifier::llvm_state s{"my llvm module"};

    // Build the rhs of the Van der Pol equation.
    auto x = "x"_var, y = "y"_var;
    auto dxdt = y;
    auto dydt = (1_num-x*x)*y - x;
    // Precomputing the derivatives to order 9
    // Second order
    auto d2xdt2 = dxdt.diff("x")*dxdt + dxdt.diff("y")*dydt;
    auto d2ydt2 = dydt.diff("x")*dxdt + dydt.diff("y")*dydt;
    // Third order
    auto d3xdt3 = d2xdt2.diff("x")*dxdt + d2xdt2.diff("y")*dydt;
    auto d3ydt3 = d2ydt2.diff("x")*dxdt + d2ydt2.diff("y")*dydt;
    // Fourth order
    auto d4xdt4 = d3xdt3.diff("x")*dxdt + d3xdt3.diff("y")*dydt;
    auto d4ydt4 = d3ydt3.diff("x")*dxdt + d3ydt3.diff("y")*dydt;
    // Fifth order
    auto d5xdt5 = d4xdt4.diff("x")*dxdt + d4xdt4.diff("y")*dydt;
    auto d5ydt5 = d4ydt4.diff("x")*dxdt + d4ydt4.diff("y")*dydt;
    // Sixth order
    auto d6xdt6 = d5xdt5.diff("x")*dxdt + d5xdt5.diff("y")*dydt;
    auto d6ydt6 = d5ydt5.diff("x")*dxdt + d5ydt5.diff("y")*dydt;
    // Seventh order
    auto d7xdt7 = d6xdt6.diff("x")*dxdt + d6xdt6.diff("y")*dydt;
    auto d7ydt7 = d6ydt6.diff("x")*dxdt + d6ydt6.diff("y")*dydt;
    // Eigth order
    auto d8xdt8 = d7xdt7.diff("x")*dxdt + d7xdt7.diff("y")*dydt;
    auto d8ydt8 = d7ydt7.diff("x")*dxdt + d7ydt7.diff("y")*dydt;

    std::cout << d2ydt2 << std::endl;

    s.add_expression("d2ydt2", d2ydt2);

    std::cout << s.dump() << '\n';

    return 0;
}
