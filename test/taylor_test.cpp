#include <iostream>

#include <lambdifier/llvm_state.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

using namespace lambdifier;

int main()
{
    // Init the LLVM machinery.
    lambdifier::llvm_state s{"my llvm module"};

    // Build the rhs of the Van der Pol equation.
    auto x = "x"_var, y = "y"_var;
    auto dxdt = y;
    auto dydt = (1_num - x * x) * y - x;
    s.add_expression("dxdt", dxdt);
    s.add_expression("dydt", dydt);
    // Precomputing the derivatives to order 9

    // Second order
    auto d2xdt2 = dxdt.diff("x") * dxdt + dxdt.diff("y") * dydt;
    auto d2ydt2 = dydt.diff("x") * dxdt + dydt.diff("y") * dydt;
    s.add_expression("d2xdt2", d2xdt2);
    s.add_expression("d2ydt2", d2ydt2);
    d2xdt2 = s.to_expression("d2xdt2");
    d2ydt2 = s.to_expression("d2ydt2");
    std::cout << "Size of d2xdt2: " << s.dump_function("d2xdt2").size() << '\n';
    std::cout << "Size of d2ydt2: " << s.dump_function("d2ydt2").size() << '\n';

    // Third order
    auto d3xdt3 = d2xdt2.diff("x") * dxdt + d2xdt2.diff("y") * dydt;
    auto d3ydt3 = d2ydt2.diff("x") * dxdt + d2ydt2.diff("y") * dydt;
    s.add_expression("d3xdt3", d3xdt3);
    s.add_expression("d3ydt3", d3ydt3);
    d3xdt3 = s.to_expression("d3xdt3");
    d3ydt3 = s.to_expression("d3ydt3");
    std::cout << "Size of d3xdt3: " << s.dump_function("d3xdt3").size() << '\n';
    std::cout << "Size of d3ydt3: " << s.dump_function("d3ydt3").size() << '\n';

    // Fourth order
    auto d4xdt4 = d3xdt3.diff("x") * dxdt + d3xdt3.diff("y") * dydt;
    auto d4ydt4 = d3ydt3.diff("x") * dxdt + d3ydt3.diff("y") * dydt;
    s.add_expression("d4xdt4", d4xdt4);
    s.add_expression("d4ydt4", d4ydt4);
    d4xdt4 = s.to_expression("d4xdt4");
    d4ydt4 = s.to_expression("d4ydt4");
    std::cout << "Size of d4xdt4: " << s.dump_function("d4xdt4").size() << '\n';
    std::cout << "Size of d4ydt4: " << s.dump_function("d4ydt4").size() << '\n';

    // Fifth order
    auto d5xdt5 = d4xdt4.diff("x") * dxdt + d4xdt4.diff("y") * dydt;
    auto d5ydt5 = d4ydt4.diff("x") * dxdt + d4ydt4.diff("y") * dydt;
    s.add_expression("d5xdt5", d5xdt5);
    s.add_expression("d5ydt5", d5ydt5);
    d5xdt5 = s.to_expression("d5xdt5");
    d5ydt5 = s.to_expression("d5ydt5");
    std::cout << "Size of d5xdt5: " << s.dump_function("d5xdt5").size() << '\n';
    std::cout << "Size of d5ydt5: " << s.dump_function("d5ydt5").size() << '\n';

    // Sixth order
    auto d6xdt6 = d5xdt5.diff("x") * dxdt + d5xdt5.diff("y") * dydt;
    auto d6ydt6 = d5ydt5.diff("x") * dxdt + d5ydt5.diff("y") * dydt;
    s.add_expression("d6xdt6", d6xdt6);
    s.add_expression("d6ydt6", d6ydt6);
    d6xdt6 = s.to_expression("d6xdt6");
    d6ydt6 = s.to_expression("d6ydt6");
    std::cout << "Size of d6xdt6: " << s.dump_function("d6xdt6").size() << '\n';
    std::cout << "Size of d6ydt6: " << s.dump_function("d6ydt6").size() << '\n';

    // Seventh order
    auto d7xdt7 = d6xdt6.diff("x") * dxdt + d6xdt6.diff("y") * dydt;
    auto d7ydt7 = d6ydt6.diff("x") * dxdt + d6ydt6.diff("y") * dydt;
    s.add_expression("d7xdt7", d7xdt7);
    s.add_expression("d7ydt7", d7ydt7);
    d7xdt7 = s.to_expression("d7xdt7");
    d7ydt7 = s.to_expression("d7ydt7");
    std::cout << "Size of d7xdt7: " << s.dump_function("d7xdt7").size() << '\n';
    std::cout << "Size of d7ydt7: " << s.dump_function("d7ydt7").size() << '\n';

    // Eigth order
    auto d8xdt8 = d7xdt7.diff("x") * dxdt + d7xdt7.diff("y") * dydt;
    auto d8ydt8 = d7ydt7.diff("x") * dxdt + d7ydt7.diff("y") * dydt;
    s.add_expression("d8xdt8", d8xdt8);
    s.add_expression("d8ydt8", d8ydt8);
    d8xdt8 = s.to_expression("d8xdt8");
    d8ydt8 = s.to_expression("d8ydt8");
    std::cout << "Size of d8xdt8: " << s.dump_function("d8xdt8").size() << '\n';
    std::cout << "Size of d8ydt8: " << s.dump_function("d8ydt8").size() << '\n';

    // std::cout << s.dump() << '\n';

    return 0;
}
