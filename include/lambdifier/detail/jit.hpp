#ifndef LAMBDIFIER_JIT_HPP
#define LAMBDIFIER_JIT_HPP

#include <memory>
#include <string>

#include <llvm/Config/llvm-config.h>
#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/RTDyldObjectLinkingLayer.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>

#include <lambdifier/detail/visibility.hpp>

namespace lambdifier::detail
{

class jit
{
    llvm::orc::ExecutionSession es;
    llvm::orc::RTDyldObjectLinkingLayer object_layer;
    std::unique_ptr<llvm::orc::IRCompileLayer> compile_layer;
    std::unique_ptr<llvm::DataLayout> dl;
    std::unique_ptr<llvm::orc::MangleAndInterner> mangle;
    llvm::orc::ThreadSafeContext ctx;
#if LLVM_VERSION_MAJOR == 10
    llvm::orc::JITDylib &main_jd;
#endif

public:
    jit();

    jit(const jit &) = delete;
    jit(jit &&) = delete;
    jit &operator=(const jit &) = delete;
    jit &operator=(jit &&) = delete;

    ~jit();

    llvm::LLVMContext &get_context();

    const llvm::DataLayout &get_data_layout() const;

    void add_module(std::unique_ptr<llvm::Module> &&);

    llvm::Expected<llvm::JITEvaluatedSymbol> lookup(const std::string &);
};

} // namespace lambdifier::detail

#endif
