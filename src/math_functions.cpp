#include <initializer_list>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <llvm/IR/Attributes.h>

#include <lambdifier/expression.hpp>
#include <lambdifier/function_call.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/number.hpp>

namespace lambdifier
{

expression sin(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"llvm.sin", std::move(args)};
    fc.set_display_name("sin");
    fc.set_type(function_call::type::builtin);
    fc.set_eval_f([](const std::vector<expression> &args, std::unordered_map<std::string, double> &v) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when computing the std::sin (1 argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }

        return std::sin(args[0](v));
    });
    fc.set_eval_num_f([](const std::vector<double> &args) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when computing the std::sin (1 argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }

        return std::sin(args[0]);
    });
    fc.set_deval_num_f([](const std::vector<double> &args, unsigned i) {
        if (args.size() != 1u || i != 0u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments or derivative requested when computing the derivative of std::sin");
        }

        return std::cos(args[0]);
    });

    fc.set_eval_batch_f([](const std::vector<expression> &args,
                           std::unordered_map<std::string, std::vector<double>> &in, std::vector<double> &out) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when computing the std::sin (1 argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }
        args[0](in, out);
        for (auto i = 0u; i < out.size(); ++i) {
            out[i] = std::sin(out[i]);
        }
    });
    fc.set_diff_f([](const std::vector<expression> &args, const std::string &s) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when taking the derivative of the sine (1 argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }

        return cos(args[0]) * args[0].diff(s);
    });

    return expression{std::move(fc)};
}

expression cos(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"llvm.cos", std::move(args)};
    fc.set_display_name("cos");
    fc.set_type(function_call::type::builtin);
    fc.set_eval_f([](const std::vector<expression> &args, std::unordered_map<std::string, double> &v) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when computing the std::cos (1 argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }

        return std::cos(args[0](v));
    });
    fc.set_eval_batch_f([](const std::vector<expression> &args,
                           std::unordered_map<std::string, std::vector<double>> &in, std::vector<double> &out) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when computing the std::sin (1 argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }
        args[0](in, out);
        for (auto i = 0u; i < out.size(); ++i) {
            out[i] = std::cos(out[i]);
        }
    });
    fc.set_eval_num_f([](const std::vector<double> &args) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when computing the std::cos (1 argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }

        return std::cos(args[0]);
    });
    fc.set_deval_num_f([](const std::vector<double> &args, unsigned i) {
        if (args.size() != 1u || i != 0u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments or derivative requested when computing the derivative of std::sin");
        }

        return -std::sin(args[0]);
    });
    fc.set_diff_f([](const std::vector<expression> &args, const std::string &s) {
        if (args.size() != 1u) {
            throw std::invalid_argument("Inconsistent number of arguments when taking the derivative of the cosine (1 "
                                        "argument was expected, but "
                                        + std::to_string(args.size()) + " arguments were provided");
        }

        return -sin(args[0]) * args[0].diff(s);
    });

    return expression{std::move(fc)};
}

expression tan(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"tan", std::move(args)};
    fc.set_attributes({llvm::Attribute::NoUnwind, llvm::Attribute::Speculatable, llvm::Attribute::ReadNone,
                       llvm::Attribute::WillReturn});
    fc.set_type(function_call::type::external);
    fc.set_diff_f([](const std::vector<expression> &args, const std::string &s) {
        if (args.size() != 1u) {
            throw std::invalid_argument("Inconsistent number of arguments when taking the derivative of the tangent (1 "
                                        "argument was expected, but "
                                        + std::to_string(args.size()) + " arguments were provided");
        }

        return expression{number{1}} / (cos(args[0]) * cos(args[0])) * args[0].diff(s);
    });

    return expression{std::move(fc)};
}

expression asin(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"asin", std::move(args)};
    fc.set_attributes({llvm::Attribute::NoUnwind, llvm::Attribute::Speculatable, llvm::Attribute::ReadNone,
                       llvm::Attribute::WillReturn});
    fc.set_type(function_call::type::external);

    return expression{std::move(fc)};
}

expression acos(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"acos", std::move(args)};
    fc.set_attributes({llvm::Attribute::NoUnwind, llvm::Attribute::Speculatable, llvm::Attribute::ReadNone,
                       llvm::Attribute::WillReturn});
    fc.set_type(function_call::type::external);

    return expression{std::move(fc)};
}

expression atan(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"atan", std::move(args)};
    fc.set_attributes({llvm::Attribute::NoUnwind, llvm::Attribute::Speculatable, llvm::Attribute::ReadNone,
                       llvm::Attribute::WillReturn});
    fc.set_type(function_call::type::external);

    return expression{std::move(fc)};
}

expression atan2(expression e1, expression e2)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e1));
    args.emplace_back(std::move(e2));

    function_call fc{"atan2", std::move(args)};
    fc.set_attributes({llvm::Attribute::NoUnwind, llvm::Attribute::Speculatable, llvm::Attribute::ReadNone,
                       llvm::Attribute::WillReturn});
    fc.set_type(function_call::type::external);

    return expression{std::move(fc)};
}

expression pow(expression e1, expression e2)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e1));
    args.emplace_back(std::move(e2));

    function_call fc{"llvm.pow", std::move(args)};
    fc.set_display_name("pow");
    fc.set_type(function_call::type::builtin);
    fc.set_disable_verify(true);

    return expression{std::move(fc)};
}

expression exp(expression e)
{
    std::vector<expression> args;
    args.emplace_back(std::move(e));

    function_call fc{"llvm.exp", std::move(args)};
    fc.set_display_name("exp");
    fc.set_type(function_call::type::builtin);

    fc.set_eval_f([](const std::vector<expression> &args, std::unordered_map<std::string, double> &v) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when computing the std::exp (1 argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }

        return std::exp(args[0](v));
    });
    fc.set_eval_batch_f([](const std::vector<expression> &args,
                           std::unordered_map<std::string, std::vector<double>> &in, std::vector<double> &out) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when computing the std::sin (1 argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }
        args[0](in, out);
        for (auto i = 0u; i < out.size(); ++i) {
            out[i] = std::exp(out[i]);
        }
    });

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

    fc.set_eval_f([](const std::vector<expression> &args, std::unordered_map<std::string, double> &v) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when computing the std::log (1 argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }

        return std::log(args[0](v));
    });
    fc.set_eval_batch_f([](const std::vector<expression> &args,
                           std::unordered_map<std::string, std::vector<double>> &in, std::vector<double> &out) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when computing the std::sin (1 argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }
        args[0](in, out);
        for (auto i = 0u; i < out.size(); ++i) {
            out[i] = std::log(out[i]);
        }
    });

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
    fc.set_diff_f([](const std::vector<expression> &args, const std::string &s) {
        if (args.size() != 1u) {
            throw std::invalid_argument(
                "Inconsistent number of arguments when taking the derivative of the square root (1 "
                "argument was expected, but "
                + std::to_string(args.size()) + " arguments were provided");
        }

        return expression{number{0.5}} / sqrt(args[0]) * args[0].diff(s);
    });

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
