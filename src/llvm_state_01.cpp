#include <cassert>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>

#include <llvm/IR/Attributes.h>
#include <llvm/IR/Constant.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <lambdifier/expression.hpp>
#include <lambdifier/function_call.hpp>
#include <lambdifier/llvm_state.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

namespace lambdifier
{

namespace detail
{

namespace
{

expression llvm_value_to_expression(const llvm::Value *val,
                                    const std::unordered_map<const llvm::Value *, expression> &value_exp_map)
{
    // Helper to extract the representation
    // of val in order to produce better error
    // messages.
    auto get_val_repr = [val]() {
        std::string out;
        llvm::raw_string_ostream ostr(out);
        val->print(ostr);
        return ostr.str();
    };

    if (auto it = value_exp_map.find(val); it == value_exp_map.end()) {
        // The value is not in the map. We can deal with it only
        // if it is a constant.
        if (llvm::isa<llvm::Constant>(val)) {
            if (auto cfp = llvm::dyn_cast<llvm::ConstantFP>(val)) {
                // Double-precision constant.
                return expression{number{cfp->getValueAPF().convertToDouble()}};
            } else if (auto cint = llvm::dyn_cast<llvm::ConstantInt>(val)) {
                // 32-bit signed integer.
                const auto &v = cint->getValue();
                assert(v.isSignedIntN(32));
                return expression{number{static_cast<double>(v.getSExtValue())}};
            }
            throw std::runtime_error("An unknown constant type was encountered while converting IR to expression. The "
                                     "value representation is: "
                                     + get_val_repr());
        } else {
            throw std::runtime_error("A value of unknown type was encountered while converting IR to expression. The "
                                     "value representation is: "
                                     + get_val_repr());
        }
    } else {
        // The value is already in the map. Fetch the corresponding expression.
        return it->second;
    }
}

} // namespace

} // namespace detail

void llvm_state::add_llvm_inst_to_value_exp_map(std::unordered_map<const llvm::Value *, expression> &value_exp_map,
                                                const llvm::Instruction &inst, std::optional<expression> &retval) const
{
    // The current instruction cannot be in the map.
    assert(value_exp_map.find(&inst) == value_exp_map.end());

    // Fetch the operands array (as a pointer to
    // the first operand) and its size.
    auto op_ptr = inst.getOperandList();
    const auto op_n = inst.getNumOperands();

    switch (inst.getOpcode()) {
        case llvm::Instruction::Call: {
            // The instruction is a function call.
            // Get the name of the function.
            // NOTE: in a function call instruction,
            // the arguments come first, while the function name
            // is the last operand.
            assert(op_n > 0u);
            const auto func_name = std::string{op_ptr[op_n - 1u].get()->getName()};

            // NOTE: this if-cascade should become a global unordered_map
            // encoding how to translate llvm function calls into expressions.
            // Entries into this map could be added to handle user-defined functions
            // (i.e., the sin() implementation in math_functions should add the
            // handler for llvm.sin.f64).
            if (func_name == "llvm.sin.f64") {
                // Sine.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, sin(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.cos.f64") {
                // Cosine.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, cos(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.exp.f64") {
                // Exp.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, exp(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.exp2.f64") {
                // Exp2.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, exp2(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.log.f64") {
                // log.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, log(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.log2.f64") {
                // log2.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, log2(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.log10.f64") {
                // log10.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, log10(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.sqrt.f64") {
                // sqrt.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, sqrt(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.fabs.f64") {
                // abs.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, abs(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.pow.f64.f64") {
                // Pow.
                assert(op_n == 3u);
                value_exp_map.emplace(&inst, pow(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map),
                                                 detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map)));
            } else if (func_name == "llvm.powi.f64") {
                // Powi.
                assert(op_n == 3u);
                value_exp_map.emplace(&inst, pow(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map),
                                                 detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map)));
            } else {
                // In the default case, we will try to
                // see if the function call refers to a
                // non-empty function from the module.
                if (auto f = module->getFunction(func_name); f != nullptr && !f->empty()) {
                    // Build the arguments for the function call.
                    std::vector<expression> f_args;
                    for (auto arg_it = f->arg_begin(); arg_it != f->arg_end(); ++arg_it) {
                        f_args.emplace_back(variable{arg_it->getName()});
                    }

                    // Prepare the function call.
                    function_call fc{func_name, std::move(f_args)};
                    fc.set_type(function_call::type::internal);
                    // NOTE: use the same function attributes
                    // as the functions implemented in math_functions.cpp.
                    fc.set_attributes({llvm::Attribute::NoUnwind, llvm::Attribute::Speculatable,
                                       llvm::Attribute::ReadNone, llvm::Attribute::WillReturn});

                    // For the derivative, we fetch the expression representation
                    // of func_name and we derive it.
                    // NOTE: other possible approaches (not clear which one is better):
                    // - look for the derivative of func_name as an LLVM function in the module
                    //   (following some naming convention, e.g., func_name.d.x) and convert that
                    //   into an expression instead. This would have the advantage of letting LLVM
                    //   simplify the derivative;
                    // - as above, but instead of extracting the expression representation of func_name.d.x,
                    //   return a function call to it.
                    fc.set_diff_f([this, func_name](const std::vector<expression> &, const std::string &s) {
                        auto func_ex = this->to_expression(func_name);
                        return func_ex.diff(s);
                    });

                    value_exp_map.emplace(&inst, expression{std::move(fc)});
                } else {
                    throw std::runtime_error("Unable to convert an IR call to the function '" + func_name
                                             + "' into an expression: the function is either not present in the "
                                               "module, or it is an empty function");
                }
            }

            break;
        }
        case llvm::Instruction::FAdd:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             + detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::FMul:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             * detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::FSub:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             - detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::FDiv:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             / detail::llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::FNeg:
            assert(op_n == 1u);
            value_exp_map.emplace(&inst, -detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map));
            break;

        case llvm::Instruction::Ret:
            assert(op_n == 1u);
            // NOTE: we will stop processing instructions
            // the moment we have the first ret statement.
            assert(!retval);
            retval.emplace(detail::llvm_value_to_expression(op_ptr[0].get(), value_exp_map));
            break;

        default:
            throw std::runtime_error(std::string{"Unknown instruction encountered while converting IR to expression: "}
                                     + inst.getOpcodeName());
    }
}

expression llvm_state::to_expression(const std::string &name) const
{
    // Fetch the function.
    auto f = module->getFunction(name);
    if (f == nullptr || f->empty()) {
        throw std::runtime_error("Unable to convert an IR call to the function '" + name
                                 + "' into an expression: the function is either not present in the "
                                   "module, or it is an empty function");
    }

    if (f->getBasicBlockList().size() != 1u) {
        throw std::runtime_error("Only single-block functions can be converted to expressions, but the function '"
                                 + name + "' has " + std::to_string(f->getBasicBlockList().size()) + " blocks");
    }

    // Fetch the function's entry block.
    const auto &eb = f->getEntryBlock();

    // value_exp_map will map LLVM values to corresponding expressions.
    // retval will end up containing the return value of the function.
    std::unordered_map<const llvm::Value *, expression> value_exp_map;
    std::optional<expression> retval;

    // Add the function arguments to value_exp_map.
    for (auto arg_it = f->arg_begin(); arg_it != f->arg_end(); ++arg_it) {
        [[maybe_unused]] const auto res = value_exp_map.emplace(&*arg_it, variable{arg_it->getName()});
        assert(res.second);
    }

    // Iterate over the instructions in the block.
    for (const auto &inst : eb) {
        add_llvm_inst_to_value_exp_map(value_exp_map, inst, retval);
        if (retval) {
            // Forcibily break out the moment we have a return statement.
            break;
        }
    }

    if (!retval) {
        throw std::runtime_error("Unable to convert an IR call to the function '" + name
                                 + "' into an expression: the function has no return statement");
    }

    return std::move(*retval);
}

std::string llvm_state::dump_function(const std::string &name) const
{
    if (auto f = module->getFunction(name)) {
        std::string out;
        llvm::raw_string_ostream ostr(out);
        f->print(ostr);
        return ostr.str();
    } else {
        throw std::invalid_argument("Could not locate the function called '" + name + "'");
    }
}

} // namespace lambdifier
