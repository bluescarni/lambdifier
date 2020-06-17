#include <string>
#include <unordered_map>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Value.h>
#include <llvm/Support/raw_ostream.h>

#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

llvm_state::llvm_state(const std::string &name) : context{}, builder(context), module(name, context) {}

llvm_state::~llvm_state() = default;

llvm::LLVMContext &llvm_state::get_context()
{
    return context;
}

llvm::IRBuilder<> &llvm_state::get_builder()
{
    return builder;
}

std::unordered_map<std::string, llvm::Value *> &llvm_state::get_named_values()
{
    return named_values;
}

std::string llvm_state::dump() const
{
    std::string out;
    llvm::raw_string_ostream ostr(out);
    module.print(ostr, nullptr);
    return out;
}

} // namespace lambdifier
