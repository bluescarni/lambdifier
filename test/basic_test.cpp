#include <iostream>

#include <lambdifier/llvm_state.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

using namespace lambdifier::literals;

int main()
{
    lambdifier::llvm_state s{"my llvm state"};

    auto c1 = 42_num;
    auto c2 = -42_num;
    auto x = "x"_var;
    auto y = "y"_var;
    auto z = "z"_var;

    auto ex = (x + (c1 * y - z)) - c2;
    ex = ex * ex + ex;

    std::cout << ex << '\n';

    auto vars = ex.get_variables();
    for (auto v : vars) {
        std::cout << v << '\n';
    }
}
