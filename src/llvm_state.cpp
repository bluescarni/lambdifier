#include <cassert>
#include <cstdint>
#include <limits>
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
#include <llvm/IR/Operator.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>

#include <lambdifier/detail/check_symbol_name.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

llvm_state::llvm_state(const std::string &name)
{
    // Create the module.
    module = std::make_unique<llvm::Module>(name, get_context());
    module->setDataLayout(jitter.get_data_layout());

    // Create a new builder for the module.
    builder = std::make_unique<llvm::IRBuilder<>>(get_context());
    // Set a couple of flags for faster math at the
    // price of potential change of semantics.
    // builder->setFastMathFlags(llvm::FastMathFlags::AllowReassoc | llvm::FastMathFlags::AllowReciprocal);

    // Create the function pass manager.
    fpm = std::make_unique<llvm::legacy::FunctionPassManager>(module.get());
    fpm->add(llvm::createPromoteMemoryToRegisterPass());
    fpm->add(llvm::createInstructionCombiningPass());
    fpm->add(llvm::createReassociatePass());
    fpm->add(llvm::createGVNPass());
    fpm->add(llvm::createCFGSimplificationPass());
    fpm->doInitialization();
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

llvm::Module &llvm_state::get_module()
{
    return *module;
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

void llvm_state::verify_function(llvm::Function *f)
{
    std::string err_report;
    llvm::raw_string_ostream ostr(err_report);
    if (llvm::verifyFunction(*f, &ostr)) {
        // Remove function before throwing.
        f->eraseFromParent();
        throw std::invalid_argument("Function verification failed. The full error message:\n" + err_report);
    }
}

void llvm_state::optimize_function(llvm::Function *f)
{
    fpm->run(*f);
}

void llvm_state::add_varargs_expression(const std::string &name, const expression &e, bool optimize,
                                        const std::vector<std::string> &vars)
{
    // Prepare the function prototype. First the function arguments.
    std::vector<llvm::Type *> fargs(vars.size(), builder->getDoubleTy());
    // Then the return type.
    auto *ft = llvm::FunctionType::get(builder->getDoubleTy(), fargs, false);
    assert(ft != nullptr);
    // Now create the function.
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

        // Verify it.
        verify_function(f);

        // Optimize it.
        if (optimize) {
            optimize_function(f);
        }
    } else {
        // Error reading body, remove function.
        f->eraseFromParent();
    }
}

void llvm_state::add_vecargs_expression(const std::string &name, bool optimize, const std::vector<std::string> &vars)
{
    // NOTE: we support indices within the unsigned range
    // below (when using the CreateConstInBoundsGEP1_32()
    // instruction).
    if (vars.size() > std::numeric_limits<unsigned>::max()) {
        throw std::overflow_error("The number of variables in an expression, " + std::to_string(vars.size())
                                  + ", is too large");
    }

    // Prepare the function prototype. The only argument is a pointer.
    std::vector<llvm::Type *> fargs(1, llvm::PointerType::getUnqual(builder->getDoubleTy()));
    // Then the return type.
    auto *ft = llvm::FunctionType::get(builder->getDoubleTy(), fargs, false);
    assert(ft != nullptr);
    // Now create the function.
    auto *f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name + ".vecargs", module.get());
    assert(f != nullptr);
    // Check about this.
    // f->addFnAttr(llvm::Attribute::get(get_context(), "readonly"));
    // Set the name of the function argument.
    const auto arg_rng = f->args();
    assert(arg_rng.begin() != arg_rng.end() && arg_rng.begin() + 1 == arg_rng.end());
    auto &vec_arg = *arg_rng.begin();
    vec_arg.setName("arg.vector");

    // Create a new basic block to start insertion into.
    auto *bb = llvm::BasicBlock::Create(get_context(), "entry", f);
    assert(bb != nullptr);
    builder->SetInsertPoint(bb);

    // Clear the map containing the variable names.
    named_values.clear();
    // Copy the elements of the input array
    // into local variables.
    for (decltype(vars.size()) i = 0; i < vars.size(); ++i) {
        const auto &var = vars[i];

        // NOTE: the purpose of this instruction is to compute
        // a pointer to the current variable in the array.
        // NOTE: we use the InBounds variant because that is
        // also used when compiling similar array-loading
        // code in clang. The docs say that it has something
        // to do with poisoning, but it's not really clear
        // to me what this implies. See here for more info:
        // https://llvm.org/docs/LangRef.html#getelementptr-instruction
        // https://stackoverflow.com/questions/26787341/inserting-getelementpointer-instruction-in-llvm-ir
        // https://llvm.org/docs/GetElementPtr.html
        // NOTE: we use the variants with 1d 32-bit indexing for
        // simplicity.
        auto ptr = builder->CreateConstInBoundsGEP1_32(
            // The underlying type for the array.
            builder->getDoubleTy(),
            // The array (that is, the pointer argument passed
            // to this function).
            &vec_arg,
            // The offset.
            static_cast<unsigned>(i),
            // Name for the pointer variable.
            "ptr_" + var);

        // Create a load instruction from the pointer
        // into a new variable.
        named_values[var] = builder->CreateLoad(builder->getDoubleTy(), ptr, var);
    }

    // NOTE: the idea now is that instead of re-generating
    // the code for the expression, we do instead a function
    // call to the varargs version of the function.
    //
    // Lookup the varargs function.
    auto varargs_f = module->getFunction(name);
    assert(varargs_f);
    assert(varargs_f->arg_size() == vars.size());

    // Create the function arguments.
    std::vector<llvm::Value *> args_v;
    args_v.reserve(vars.size());
    for (const auto &var : vars) {
        args_v.push_back(named_values[var]);
    }

    // Do the invocation.
    if (auto *ret_val = builder->CreateCall(varargs_f, args_v, "calltmp")) {
        // Finish off the function.
        builder->CreateRet(ret_val);

        // Verify it.
        verify_function(f);

        // Optimize it.
        if (optimize) {
            optimize_function(f);
        }
    } else {
        // Error reading body, remove function.
        f->eraseFromParent();
    }
}

void llvm_state::add_expression(const std::string &name, const expression &e, bool optimize)
{
    detail::check_symbol_name(name);

    if (module->getNamedValue(name) != nullptr) {
        throw std::invalid_argument("The name '" + name + "' already exists in the module");
    }

    // Fetch the number and names of the
    // variables from the expression.
    const auto vars = e.get_variables();

    add_varargs_expression(name, e, optimize, vars);
    add_vecargs_expression(name, optimize, vars);
}

void llvm_state::compile()
{
    jitter.add_module(std::move(module));
}

llvm_state::f_ptr llvm_state::fetch(const std::string &name)
{
    auto sym = llvm::ExitOnError()(jitter.lookup(name + ".vecargs"));

    return reinterpret_cast<double (*)(const double *)>(static_cast<std::uintptr_t>(sym.getAddress()));
}

void *llvm_state::fetch_vararg(const std::string &name)
{
    auto sym = llvm::ExitOnError()(jitter.lookup(name));

    return reinterpret_cast<void *>(static_cast<std::uintptr_t>(sym.getAddress()));
}

} // namespace lambdifier
