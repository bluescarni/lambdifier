#include <utility>
#include <vector>

#include <lambdifier/expression.hpp>
#include <lambdifier/function_call.hpp>
#include <lambdifier/math_functions.hpp>

namespace lambdifier
{

expression sin(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    return expression{function_call{"llvm.sin", std::move(args)}};
}

expression cos(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    return expression{function_call{"llvm.cos", std::move(args)}};
}

expression pow(expression e1, expression e2)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e1));
    args.emplace_back(std::move(e2));

    return expression{function_call{"llvm.pow", std::move(args)}};
}

expression exp(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    return expression{function_call{"llvm.exp", std::move(args)}};
}

expression exp2(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    return expression{function_call{"llvm.exp2", std::move(args)}};
}

expression log(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    return expression{function_call{"llvm.log", std::move(args)}};
}

expression log2(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    return expression{function_call{"llvm.log2", std::move(args)}};
}

expression log10(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    return expression{function_call{"llvm.log10", std::move(args)}};
}

expression sqrt(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    return expression{function_call{"llvm.sqrt", std::move(args)}};
}

expression abs(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    return expression{function_call{"llvm.fabs", std::move(args)}};
}

} // namespace lambdifier
