#include <cstdint>
#include <iostream>

#include <lambdifier/function_call.hpp>
#include <lambdifier/llvm_state.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

using namespace lambdifier::literals;

int main()
{
    // Init the LLVM machinery.
    lambdifier::llvm_state s{"my llvm module"};

    // Build the expression 42 * (x + y).
    auto c = 42_num;
    auto x = "x"_var, y = "y"_var;

    s.add_expression("f", sin(c) * sin(c) + cos(c) * cos(c) + abs(x + y));
    // s.add_expression("g", "f"_func(x, y) + "f"_func(x, y));

    std::cout << s.dump() << '\n';

    // Compile all the functions in the module.
    s.compile();

    // Fetch the compiled function.
    auto func1 = reinterpret_cast<double (*)(double[2])>(s.fetch("f.vecargs"));
    auto func2 = reinterpret_cast<double (*)(double, double)>(s.fetch("f"));

    // Invoke it.
    double args[] = {1, 2};
    std::cout << func1(args) << '\n';
    std::cout << func1(args) << '\n';
    std::cout << func1(args) << '\n';
    std::cout << func1(args) << '\n';

    std::cout << func2(1, 2) << '\n';
    std::cout << func2(1, 2) << '\n';
    std::cout << func2(1, 2) << '\n';
    std::cout << func2(1, 2) << '\n';
}
