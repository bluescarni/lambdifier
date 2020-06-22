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

    // Build an expression.
    auto c = 42_num;
    auto x = "x"_var, y = "y"_var;

    auto ex = sin(x) * sqrt(x);
    auto ex_prime = ex.diff("x").diff("x").diff("x");
    std::cout << ex_prime << '\n';

    s.add_expression("f", ex);
    s.add_expression("fprime", ex_prime);

    std::cout << s.dump() << '\n';


    // Compile all the functions in the module.
    s.compile();

    // Fetch the compiled function.
    auto func1 = s.fetch("f");
    auto func2 = s.fetch_vararg<2>("f");

    // Invoke it.
    double args[] = {3.45, 6.78};
    std::cout << func1(args) << '\n';
    std::cout << func1(args) << '\n';
    std::cout << func1(args) << '\n';
    std::cout << func1(args) << '\n';

    std::cout << func2(args[0], args[1]) << '\n';
    std::cout << func2(args[0], args[1]) << '\n';
    std::cout << func2(args[0], args[1]) << '\n';
    std::cout << func2(args[0], args[1]) << '\n';
    return 0;

}
