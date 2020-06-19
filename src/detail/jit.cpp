#include <memory>
#include <mutex>
#include <string>
#include <utility>

#include <llvm/Config/llvm-config.h>
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
#if LLVM_VERSION_MAJOR == 10
      ,
      main_jd(es.createJITDylib("<main>"))
#endif
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

#if LLVM_VERSION_MAJOR == 10
    compile_layer = std::make_unique<llvm::orc::IRCompileLayer>(
        es, object_layer, std::make_unique<llvm::orc::ConcurrentIRCompiler>(std::move(*jtmb)));
#else
    compile_layer = std::make_unique<llvm::orc::IRCompileLayer>(es, object_layer,
                                                                llvm::orc::ConcurrentIRCompiler(std::move(*jtmb)));
#endif

    dl = std::make_unique<llvm::DataLayout>(std::move(*dlout));

    mangle = std::make_unique<llvm::orc::MangleAndInterner>(es, *dl);

#if LLVM_VERSION_MAJOR == 10
    main_jd.addGenerator(
        llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(dl->getGlobalPrefix())));
#else
    es.getMainJITDylib().setGenerator(
        llvm::cantFail(llvm::orc::DynamicLibrarySearchGenerator::GetForCurrentProcess(dl->getGlobalPrefix())));
#endif
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
#if LLVM_VERSION_MAJOR == 10
    auto handle = compile_layer->add(main_jd, llvm::orc::ThreadSafeModule(std::move(m), ctx));
#else
    auto handle = compile_layer->add(es.getMainJITDylib(), llvm::orc::ThreadSafeModule(std::move(m), ctx));
#endif

    if (handle) {
        throw;
    }
}

llvm::Expected<llvm::JITEvaluatedSymbol> jit::lookup(const std::string &name)
{
#if LLVM_VERSION_MAJOR == 10
    return es.lookup({&main_jd}, (*mangle)(name));
#else
    return es.lookup({&es.getMainJITDylib()}, (*mangle)(name));
#endif
}

} // namespace lambdifier::detail
