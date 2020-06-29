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

    s.add_taylor("sbaffo", {y, (1_num - x * x) * y - x});

    std::cout << s.dump() << '\n';

    s.compile();

    double state[] = {1, 2};

    s.fetch_taylor("sbaffo")(state, 1.2, 1);

    for (auto x : state) {
        std::cout << x << '\n';
    }

    return 0;

    for (const auto &ex : lambdifier::taylor_decompose({0_num, (1_num - x * x) * y - x})) {
        std::cout << ex << '\n';
    }

    return 0;

    auto ex = sin(x) * cos(x);
    std::cout << ex << '\n';

    s.add_expression("f", ex, 1);
    s.add_expression("f1", ex.diff("x"), 1);
    s.add_expression("f2", s.to_expression("f1").diff("x"), 1);
    s.add_expression("f3", s.to_expression("f2").diff("x"), 1);
    s.add_expression("f4", s.to_expression("f3").diff("x"), 1);
    s.add_expression("f5", s.to_expression("f4").diff("x"), 1);
    s.add_expression("f6", s.to_expression("f5").diff("x"), 1);
    s.add_expression("f7", s.to_expression("f6").diff("x"), 1);
    s.add_expression("f8", s.to_expression("f7").diff("x"), 1);

    std::cout << s.dump() << '\n';

    return 0;

    std::cout << s.to_expression("f") << '\n';

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
