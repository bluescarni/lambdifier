#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <llvm/IR/Function.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <lambdifier/expression.hpp>
#include <lambdifier/function_call.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

function_call::function_call(const std::string &name, std::vector<expression> args) : name(name), args(std::move(args))
{
}

function_call::function_call(const function_call &) = default;
function_call::function_call(function_call &&) noexcept = default;

function_call::~function_call() = default;

const std::string &function_call::get_name() const
{
    return name;
}

const std::vector<expression> &function_call::get_args() const
{
    return args;
}

std::vector<expression> &function_call::access_args()
{
    return args;
}

llvm::Value *function_call::codegen(llvm_state &s) const
{
    llvm::Function *callee_f;

    if (auto intrinsic_ID = llvm::Function::lookupIntrinsicID(name)) {
        // NOTE: for generic intrinsics to work, we need to specify
        // the desired argument types. See:
        // https://stackoverflow.com/questions/11985247/llvm-insert-intrinsic-function-cos
        // And the docs of the getDeclaration() function.
        const std::vector<llvm::Type *> doubles(args.size(), s.get_builder().getDoubleTy());

        // Intrinsic function.
        callee_f = llvm::Intrinsic::getDeclaration(&s.get_module(), intrinsic_ID, doubles);

        if (!callee_f) {
            throw std::invalid_argument("Error getting the declaration of the intrinsic '" + name + "'");
        }
    } else {
        // Look up the name in the global module table.
        callee_f = s.get_module().getFunction(name);

        if (!callee_f) {
            throw std::invalid_argument("Unknown function referenced: " + name);
        }
    }

    // If argument mismatch error.
    if (callee_f->arg_size() != args.size()) {
        throw std::invalid_argument("Incorrect # arguments passed: " + std::to_string(callee_f->arg_size())
                                    + " are expected, but " + std::to_string(args.size()) + " were provided instead");
    }

    std::vector<llvm::Value *> args_v;
    for (decltype(args.size()) i = 0, e = args.size(); i != e; ++i) {
        args_v.push_back(args[i].codegen(s));
        if (!args_v.back()) {
            return nullptr;
        }
    }

    return s.get_builder().CreateCall(callee_f, args_v, "calltmp");
}

std::string function_call::to_string() const
{
    auto retval = name;
    retval += "(";
    for (decltype(args.size()) i = 0; i < args.size(); ++i) {
        retval += args[i].to_string();
        if (i != args.size() - 1u) {
            retval += ",";
        }
    }
    retval += ")";

    return retval;
}

void function_call::set_name(std::string n)
{
    name = std::move(n);
}

void function_call::set_args(std::vector<expression> v)
{
    args = std::move(v);
}

namespace detail
{

function_caller::function_caller(const std::string &name) : name(name) {}

} // namespace detail

inline namespace literals
{

detail::function_caller operator""_func(const char *str, std::size_t size)
{
    return detail::function_caller{std::string{str, size}};
}

} // namespace literals

} // namespace lambdifier
