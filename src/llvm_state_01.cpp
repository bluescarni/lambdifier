#include <cassert>
#include <optional>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include <llvm/IR/Constant.h>
#include <llvm/IR/Instruction.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/Casting.h>
#include <llvm/Support/raw_ostream.h>

#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>
#include <lambdifier/math_functions.hpp>
#include <lambdifier/number.hpp>
#include <lambdifier/variable.hpp>

namespace lambdifier
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

void add_llvm_inst_to_value_exp_map(std::unordered_map<const llvm::Value *, expression> &value_exp_map,
                                    const llvm::Instruction &inst, std::optional<expression> &retval)
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

            if (func_name == "llvm.sin.f64") {
                // Sine.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, sin(llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.cos.f64") {
                // Cosine.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, cos(llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.exp.f64") {
                // Exp.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, exp(llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.exp2.f64") {
                // Exp2.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, exp2(llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.log.f64") {
                // log.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, log(llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.log2.f64") {
                // log2.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, log2(llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.log10.f64") {
                // log10.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, log10(llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.sqrt.f64") {
                // sqrt.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, sqrt(llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.fabs.f64") {
                // abs.
                assert(op_n == 2u);
                value_exp_map.emplace(&inst, abs(llvm_value_to_expression(op_ptr[0].get(), value_exp_map)));
            } else if (func_name == "llvm.pow.f64.f64") {
                // Pow.
                assert(op_n == 3u);
                value_exp_map.emplace(&inst, pow(llvm_value_to_expression(op_ptr[0].get(), value_exp_map),
                                                 llvm_value_to_expression(op_ptr[1].get(), value_exp_map)));
            } else if (func_name == "llvm.powi.f64") {
                // Powi.
                assert(op_n == 3u);
                value_exp_map.emplace(&inst, pow(llvm_value_to_expression(op_ptr[0].get(), value_exp_map),
                                                 llvm_value_to_expression(op_ptr[1].get(), value_exp_map)));
            } else {
                throw std::runtime_error("Unable to convert an IR call to the function '" + func_name
                                         + "' into an expression");
            }

            break;
        }
        case llvm::Instruction::FAdd:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             + llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::FMul:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             * llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::FSub:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             - llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::FDiv:
            assert(op_n == 2u);
            value_exp_map.emplace(&inst, llvm_value_to_expression(op_ptr[0].get(), value_exp_map)
                                             / llvm_value_to_expression(op_ptr[1].get(), value_exp_map));
            break;

        case llvm::Instruction::Ret:
            assert(!retval);
            assert(op_n == 1u);
            retval.emplace(llvm_value_to_expression(op_ptr[0].get(), value_exp_map));
            break;

        default:
            throw std::runtime_error(std::string{"Unknown instruction encountered while converting IR to expression: "}
                                     + inst.getOpcodeName());
    }
}

} // namespace

expression llvm_state::to_expression(const std::string &name) const
{
    // Fetch the function.
    auto f = module->getFunction(name);
    assert(f != nullptr);

    // value_exp_map will map LLVM values to corresponding expressions.
    // retval will end up containing the return value of the function.
    std::unordered_map<const llvm::Value *, expression> value_exp_map;
    std::optional<expression> retval;

    // Add the function arguments to value_exp_map.
    for (auto arg_it = f->arg_begin(); arg_it != f->arg_end(); ++arg_it) {
        [[maybe_unused]] const auto res = value_exp_map.emplace(&*arg_it, variable{arg_it->getName()});
        assert(res.second);
    }

    // Fetch the function's entry block.
    const auto &eb = f->getEntryBlock();
    assert(f->getBasicBlockList().size() == 1u);

    // Iterate over the instructions in the block.
    for (auto it = eb.begin(); it != eb.end();) {
        add_llvm_inst_to_value_exp_map(value_exp_map, *it, retval);
        ++it;
        assert(!retval || it == eb.end());
    }

    assert(retval);
    return std::move(*retval);
}

} // namespace lambdifier
