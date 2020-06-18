#include <iostream>

#include <lambdifier/llvm_state.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

using namespace lambdifier::literals;

int main()
{
    lambdifier::llvm_state s{"my llvm module"};

    auto c1 = 42_num;
    auto x = "x"_var;
    auto y = "y"_var;

    auto ex = c1 * (x + y);
    ex = (ex + ex) * ex;
    ex = ex * ex * ex * ex * ex * ex * ex;
    ex = ex / 13_num;

    s.emit("fappo", ex);

    std::cout << s.dump() << '\n';
}
