#include <cstdint>
#include <iostream>

#include <lambdifier/llvm_state.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

using namespace lambdifier::literals;

int main()
{
    // Init the LLVM machinery.
    lambdifier::llvm_state s{"my llvm module"};

    // Build the expression 42 * (x + y).
    auto c = 42_num;
    auto x = "x"_var;
    auto y = "y"_var;
    auto ex = c * (x + y);
    ex = ex * ex * ex * ex * ex * ex * ex * ex * ex;

    // Add the expression as a function of x and y
    // called "fappo" to the module.
    s.add_expression("fappo", ex);

    std::cout << s.dump() << '\n';
    std::cout << ex << '\n';

    // Compile all the functions in the module.
    s.compile();

    // Fetch the compiled function.
    auto func = reinterpret_cast<double (*)(double, double)>(s.fetch("fappo"));

    // Invoke it.
    std::cout << func(1, 2) << '\n';
}
