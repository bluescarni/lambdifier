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
    lambdifier::llvm_state s{"my llvm module", 3};

    // Build an expression.
    auto c = 42_num;
    auto x = "x"_var, y = "y"_var;

    auto ex = x + x - x * x;
    std::cout << ex << '\n';

    s.add_expression("f", ex, 10);

    std::cout << s.dump() << '\n';

    // Compile all the functions in the module.
    s.compile();

    return 0;

    // Fetch the compiled function.
    auto func1 = s.fetch("f");
    auto func2 = s.fetch_vararg<1>("f");
    auto func3 = s.fetch_batch("f");

    // Invoke it.
    double args[] = {3.45, 6.78};
    double out[] = {0, 0};
    std::cout << func1(args) << ", " << func1(args + 1) << '\n';
    std::cout << func2(args[0]) << ", " << func2(args[1]) << '\n';
    func3(out, args);
    std::cout << out[0] << ", " << out[1] << '\n';
}
