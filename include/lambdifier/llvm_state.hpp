#ifndef LAMBDIFIER_LLVM_STATE_HPP
#define LAMBDIFIER_LLVM_STATE_HPP

#include <memory>
#include <string>
#include <unordered_map>

#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Value.h>

#include <lambdifier/detail/fwd_decl.hpp>
#include <lambdifier/detail/jit.hpp>
#include <lambdifier/detail/visibility.hpp>

namespace lambdifier
{

class LAMBDIFIER_DLL_PUBLIC llvm_state
{
    detail::jit jitter;
    std::unique_ptr<llvm::Module> module;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::unique_ptr<llvm::legacy::FunctionPassManager> fpm;
    std::unordered_map<std::string, llvm::Value *> named_values;

    LAMBDIFIER_DLL_LOCAL void add_varargs_expression(const std::string &, const expression &, bool,
                                                     const std::vector<std::string> &);
    LAMBDIFIER_DLL_LOCAL void add_vecargs_expression(const std::string &, bool, const std::vector<std::string> &);
    LAMBDIFIER_DLL_LOCAL void verify_function(llvm::Function *);
    LAMBDIFIER_DLL_LOCAL void optimize_function(llvm::Function *);

public:
    explicit llvm_state(const std::string &);

    llvm_state(const llvm_state &) = delete;
    llvm_state(llvm_state &&) = delete;
    llvm_state &operator=(const llvm_state &) = delete;
    llvm_state &operator=(llvm_state &&) = delete;

    ~llvm_state();

    void add_expression(const std::string &, const expression &, bool = true);

    llvm::LLVMContext &get_context();
    llvm::IRBuilder<> &get_builder();
    std::unordered_map<std::string, llvm::Value *> &get_named_values();
    llvm::Module &get_module();

    std::string dump() const;

    void compile();

    using f_ptr = double (*)(const double *);
    f_ptr fetch(const std::string &);
    void *fetch_vararg(const std::string &);
};

} // namespace lambdifier

#endif
