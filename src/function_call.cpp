#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <llvm/IR/Attributes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Intrinsics.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>

#include <lambdifier/detail/check_symbol_name.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/function_call.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

function_call::function_call(std::string s, std::vector<expression> args)
    : name(std::move(s)), display_name(name), args(std::move(args))
{
}

function_call::function_call(const function_call &) = default;
function_call::function_call(function_call &&) noexcept = default;
function_call::~function_call() = default;

// Getters.
const std::string &function_call::get_name() const
{
    return name;
}

const std::string &function_call::get_display_name() const
{
    return display_name;
}

const std::vector<expression> &function_call::get_args() const
{
    return args;
}

std::vector<expression> &function_call::access_args()
{
    return args;
}
const std::vector<llvm::Attribute::AttrKind> &function_call::get_attributes() const
{
    return attributes;
}

function_call::type function_call::get_type() const
{
    return ty;
}

// Setters.
void function_call::set_name(std::string s)
{
    name = std::move(s);
}

void function_call::set_display_name(std::string s)
{
    display_name = std::move(s);
}

void function_call::set_args(std::vector<expression> a)
{
    args = std::move(a);
}

void function_call::set_attributes(std::vector<llvm::Attribute::AttrKind> att)
{
    attributes = std::move(att);
}

void function_call::set_type(type t)
{
    switch (t) {
        case type::internal:
            [[fallthrough]];
        case type::external:
            [[fallthrough]];
        case type::builtin:
            ty = t;
            break;
        default:
            throw std::invalid_argument("Invalid funciton type selected: " + std::to_string(static_cast<int>(t)));
    }
}

llvm::Value *function_call::codegen(llvm_state &s) const
{
    llvm::Function *callee_f;

    if (ty == type::internal) {
        // Look up the name in the global module table.
        callee_f = s.get_module().getFunction(name);

        if (!callee_f) {
            throw std::invalid_argument("Unknown internal function referenced: '" + name + "'");
        }

        if (callee_f->empty()) {
            // An internal function cannot be empty (i.e., we need declaration
            // and definition).
            throw std::invalid_argument("The internal function '" + name + "' is empty");
        }
    } else if (ty == type::external) {
        // Look up the name in the global module table.
        callee_f = s.get_module().getFunction(name);

        if (callee_f) {
            // The function declaration exists already. Check that it is only a
            // declaration and not a definition.
            if (!callee_f->empty()) {
                throw std::invalid_argument(
                    "Cannot call the function '" + name
                    + "' as an external function, because it is defined as an internal module function");
            }
        } else {
            // The function does not exist yet, make the prototype.
            detail::check_symbol_name(name);
            std::vector<llvm::Type *> doubles(args.size(), s.get_builder().getDoubleTy());
            auto *ft = llvm::FunctionType::get(s.get_builder().getDoubleTy(), doubles, false);
            callee_f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, &s.get_module());

            if (!callee_f) {
                throw std::invalid_argument("The creation of the prototype of the external function '" + name
                                            + "' failed");
            }

            // Add the function attributes.
            for (const auto &att : attributes) {
                callee_f->addFnAttr(att);
            }
        }
    } else {
        // Builtin.
        const auto intrinsic_ID = llvm::Function::lookupIntrinsicID(name);
        if (!intrinsic_ID) {
            throw std::invalid_argument("Cannot fetch the ID of the intrinsic '" + name + "'");
        }

        // NOTE: for generic intrinsics to work, we need to specify
        // the desired argument types. See:
        // https://stackoverflow.com/questions/11985247/llvm-insert-intrinsic-function-cos
        // And the docs of the getDeclaration() function.
        const std::vector<llvm::Type *> doubles(args.size(), s.get_builder().getDoubleTy());

        callee_f = llvm::Intrinsic::getDeclaration(&s.get_module(), intrinsic_ID, doubles);

        if (!callee_f) {
            throw std::invalid_argument("Error getting the declaration of the intrinsic '" + name + "'");
        }

        if (!callee_f->empty()) {
            // It does not make sense to have a definition of a builtin.
            throw std::invalid_argument("The intrinsic '" + name + "' must be an empty function");
        }
    }

    // Check the number of arguments.
    if (callee_f->arg_size() != args.size()) {
        throw std::invalid_argument("Incorrect # of arguments passed in a function call: "
                                    + std::to_string(callee_f->arg_size()) + " are expected, but "
                                    + std::to_string(args.size()) + " were provided instead");
    }

    // Create the function arguments.
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
    auto retval = display_name;
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

double function_call::evaluate(std::unordered_map<std::string, double> &values) const
{
    if (name.compare("llvm.sin") == 0) {
        return std::sin(args[0](values));
    } else if (name.compare("llvm.cos") == 0) {
        return std::cos(args[0](values));
    } else if (name.compare("llvm.pow") == 0) {
        return std::pow(args[0](values), args[1](values));
    } else if (name.compare("llvm.exp") == 0) {
        return std::exp(args[0](values));
    } else if (name.compare("llvm.exp2") == 0) {
        return std::exp2(args[0](values));
    } else if (name.compare("llvm.log") == 0) {
        return std::log(args[0](values));
    } else if (name.compare("llvm.log2") == 0) {
        return std::log2(args[0](values));
    } else if (name.compare("llvm.log10") == 0) {
        return std::log10(args[0](values));
    } else if (name.compare("llvm.sqrt") == 0) {
        return std::sqrt(args[0](values));
    } else {
        assert(name.compare("llvm.fabs"));
        return std::fabs(args[0](values));
    }
}
} // namespace lambdifier
