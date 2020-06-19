#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <llvm/ExecutionEngine/JITSymbol.h>
#include <llvm/ExecutionEngine/Orc/CompileUtils.h>
#include <llvm/ExecutionEngine/Orc/Core.h>
#include <llvm/ExecutionEngine/Orc/ExecutionUtils.h>
#include <llvm/ExecutionEngine/Orc/IRCompileLayer.h>
#include <llvm/ExecutionEngine/Orc/JITTargetMachineBuilder.h>
#include <llvm/ExecutionEngine/Orc/ThreadSafeModule.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/DataLayout.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/TargetSelect.h>

#include <lambdifier/detail/jit.hpp>

#include <iostream>

namespace lambdifier::detail
{

namespace
{

std::once_flag nt_inited;

} // namespace

jit::jit()
    : object_layer(es, []() { return std::make_unique<llvm::SectionMemoryManager>(); }),
      ctx(std::make_unique<llvm::LLVMContext>())
{
    std::call_once(detail::nt_inited, []() {
        llvm::InitializeNativeTarget();
        llvm::InitializeNativeTargetAsmPrinter();
        llvm::InitializeNativeTargetAsmParser();
    });

    auto jtmb = llvm::orc::JITTargetMachineBuilder::detectHost();

    if (!jtmb) {
        throw std::invalid_argument("Error invoking llvm::orc::JITTargetMachineBuilder::detectHost()");
    }

    auto dlout = jtmb->getDefaultDataLayoutForTarget();
    if (!dlout) {
        throw std::invalid_argument("Error invoking getDefaultDataLayoutForTarget()");
    }

    compile_layer = std::make_unique<llvm::orc::IRCompileLayer>(es, object_layer,
                                                                llvm::orc::ConcurrentIRCompiler(std::move(*jtmb)));

    dl = std::make_unique<llvm::DataLayout>(std::move(*dlout));

    mangle = std::make_unique<llvm::orc::MangleAndInterner>(es, *dl);

    es.getMainJITDylib().setGenerator(
        llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(dl->getGlobalPrefix())));
}

jit::~jit() = default;

llvm::LLVMContext &jit::get_context()
{
    return *ctx.getContext();
}

const llvm::DataLayout &jit::get_data_layout() const
{
    return *dl;
}

void jit::add_module(std::unique_ptr<llvm::Module> &&m)
{
    auto handle = std::make_unique<llvm::Error>(
        compile_layer->add(es.getMainJITDylib(), llvm::orc::ThreadSafeModule(std::move(m), ctx)));

    if (*handle) {
        throw;
    }
}

llvm::Expected<llvm::JITEvaluatedSymbol> jit::lookup(const std::string &name)
{
    return es.lookup({&es.getMainJITDylib()}, (*mangle)(name));
}

} // namespace lambdifier::detail
