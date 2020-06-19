#include <cassert>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>

#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

llvm_state::llvm_state(const std::string &name)
{
    // Create the module.
    module = llvm::make_unique<llvm::Module>(name, get_context());
    module->setDataLayout(jitter.get_data_layout());

    // Create a new builder for the module.
    builder = llvm::make_unique<llvm::IRBuilder<>>(get_context());
}

llvm_state::~llvm_state() = default;

llvm::LLVMContext &llvm_state::get_context()
{
    return jitter.get_context();
}

llvm::IRBuilder<> &llvm_state::get_builder()
{
    return *builder;
}

std::unordered_map<std::string, llvm::Value *> &llvm_state::get_named_values()
{
    return named_values;
}

std::string llvm_state::dump() const
{
    std::string out;
    llvm::raw_string_ostream ostr(out);
    module->print(ostr, nullptr);
    return out;
}

void llvm_state::add_expression(const std::string &name, const expression &e, bool optimize)
{
    if (module->getNamedValue(name) != nullptr) {
        throw std::invalid_argument("The name '" + name + "' already exists in the module");
    }

    // Fetch the number and names of the
    // variables from the expression.
    const auto vars = e.get_variables();

    // Prepare the function prototype. First the function arguments.
    std::vector<llvm::Type *> fargs(vars.size(), llvm::Type::getDoubleTy(get_context()));
    // Then the return type.
    auto *ft = llvm::FunctionType::get(llvm::Type::getDoubleTy(get_context()), fargs, false);
    assert(ft != nullptr);
    // Now create the prototype.
    auto *f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, module.get());
    assert(f != nullptr);

    // Set names for all arguments.
    decltype(vars.size()) idx = 0;
    for (auto &arg : f->args()) {
        arg.setName(vars[idx++]);
    }

    // Create a new basic block to start insertion into.
    auto *bb = llvm::BasicBlock::Create(get_context(), "entry", f);
    assert(bb != nullptr);
    builder->SetInsertPoint(bb);

    // Record the function arguments in the NamedValues map.
    named_values.clear();
    for (auto &arg : f->args()) {
        named_values[arg.getName()] = &arg;
    }

    if (auto *ret_val = e.codegen(*this)) {
        // Finish off the function.
        builder->CreateRet(ret_val);

        // Validate the generated code, checking for consistency.
        std::string err_report;
        llvm::raw_string_ostream ostr(err_report);
        if (llvm::verifyFunction(*f, &ostr)) {
            // Remove function before throwing.
            f->eraseFromParent();

            throw std::invalid_argument("Function verification failed. The full error message:\n" + err_report);
        }

        if (optimize) {
            llvm::legacy::FunctionPassManager fpm(module.get());

            fpm.add(llvm::createInstructionCombiningPass());
            fpm.add(llvm::createReassociatePass());
            fpm.add(llvm::createGVNPass());
            fpm.add(llvm::createCFGSimplificationPass());
            fpm.doInitialization();

            fpm.run(*f);
        }
    } else {
        // Error reading body, remove function.
        f->eraseFromParent();
    }
}

void llvm_state::compile()
{
    jitter.add_module(std::move(module));
}

std::uintptr_t llvm_state::fetch(const std::string &name)
{
    auto sym = llvm::ExitOnError()(jitter.lookup(name));

    return static_cast<std::uintptr_t>(sym.getAddress());
}

} // namespace lambdifier
