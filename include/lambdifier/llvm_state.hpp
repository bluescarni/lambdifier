#ifndef LAMBDIFIER_LLVM_STATE_HPP
#define LAMBDIFIER_LLVM_STATE_HPP

#include <string>
#include <unordered_map>

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <lambdifier/detail/fwd_decl.hpp>
#include <lambdifier/detail/visibility.hpp>

namespace lambdifier
{

class LAMBDIFIER_DLL_PUBLIC llvm_state
{
    llvm::LLVMContext context;
    llvm::IRBuilder<> builder;
    llvm::Module module;
    std::unordered_map<std::string, llvm::Value *> named_values;

public:
    explicit llvm_state(const std::string &);

    llvm_state(const llvm_state &) = delete;
    llvm_state(llvm_state &&) = delete;
    llvm_state &operator=(const llvm_state &) = delete;
    llvm_state &operator=(llvm_state &&) = delete;

    void emit(const std::string &, const expression &, bool = true);

    ~llvm_state();

    llvm::LLVMContext &get_context();
    llvm::IRBuilder<> &get_builder();
    std::unordered_map<std::string, llvm::Value *> &get_named_values();

    std::string dump() const;
};

} // namespace lambdifier

#endif
