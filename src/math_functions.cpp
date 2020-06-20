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

    function_call fc{"llvm.sin", std::move(args)};
    fc.set_display_name("sin");
    fc.set_type(function_call::type::builtin);

    return expression{std::move(fc)};
}

expression cos(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"llvm.cos", std::move(args)};
    fc.set_display_name("cos");
    fc.set_type(function_call::type::builtin);

    return expression{std::move(fc)};
}

expression pow(expression e1, expression e2)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e1));
    args.emplace_back(std::move(e2));

    function_call fc{"pow", std::move(args)};
    fc.set_display_name("pow");
    fc.set_type(function_call::type::external);

    return expression{std::move(fc)};
}

expression exp(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"llvm.exp", std::move(args)};
    fc.set_display_name("exp");
    fc.set_type(function_call::type::builtin);

    return expression{std::move(fc)};
}

expression exp2(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"llvm.exp2", std::move(args)};
    fc.set_display_name("exp2");
    fc.set_type(function_call::type::builtin);

    return expression{std::move(fc)};
}

expression log(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"llvm.log", std::move(args)};
    fc.set_display_name("log");
    fc.set_type(function_call::type::builtin);

    return expression{std::move(fc)};
}

expression log2(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"llvm.log2", std::move(args)};
    fc.set_display_name("log2");
    fc.set_type(function_call::type::builtin);

    return expression{std::move(fc)};
}

expression log10(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"llvm.log10", std::move(args)};
    fc.set_display_name("log10");
    fc.set_type(function_call::type::builtin);

    return expression{std::move(fc)};
}

expression sqrt(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"llvm.sqrt", std::move(args)};
    fc.set_display_name("sqrt");
    fc.set_type(function_call::type::builtin);

    return expression{std::move(fc)};
}

expression abs(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"llvm.fabs", std::move(args)};
    fc.set_display_name("abs");
    fc.set_type(function_call::type::builtin);

    return expression{std::move(fc)};
}

} // namespace lambdifier
