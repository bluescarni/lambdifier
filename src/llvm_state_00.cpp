#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include <llvm/IR/Attributes.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/InstrTypes.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Operator.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Support/Error.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Transforms/IPO.h>
#include <llvm/Transforms/IPO/PassManagerBuilder.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <llvm/Transforms/Utils.h>
#include <llvm/Transforms/Vectorize.h>

#include <lambdifier/detail/check_symbol_name.hpp>
#include <lambdifier/expression.hpp>
#include <lambdifier/llvm_state.hpp>

namespace lambdifier
{

llvm_state::llvm_state(const std::string &name, unsigned l) : opt_level(l)
{
    // Create the module.
    module = std::make_unique<llvm::Module>(name, get_context());
    module->setDataLayout(jitter.get_data_layout());

    // Create a new builder for the module.
    builder = std::make_unique<llvm::IRBuilder<>>(get_context());
    // Set a couple of flags for faster math at the
    // price of potential change of semantics.
    llvm::FastMathFlags fmf;
    fmf.setFast();
    builder->setFastMathFlags(fmf);

    // Create the optimization passes.
    if (opt_level > 0u) {
        // Create the function pass manager.
        fpm = std::make_unique<llvm::legacy::FunctionPassManager>(module.get());
        fpm->add(llvm::createPromoteMemoryToRegisterPass());
        fpm->add(llvm::createInstructionCombiningPass());
        fpm->add(llvm::createReassociatePass());
        fpm->add(llvm::createGVNPass());
        fpm->add(llvm::createCFGSimplificationPass());
        fpm->add(llvm::createLoopVectorizePass());
        fpm->add(llvm::createSLPVectorizerPass());
        fpm->add(llvm::createLoadStoreVectorizerPass());
        fpm->add(llvm::createLoopUnrollPass());
        fpm->doInitialization();

        // The module-level optimizer. See:
        // https://stackoverflow.com/questions/48300510/llvm-api-optimisation-run
        pm = std::make_unique<llvm::legacy::PassManager>();
        llvm::PassManagerBuilder pm_builder;
        // See here for the defaults:
        // https://llvm.org/doxygen/PassManagerBuilder_8cpp_source.html
        pm_builder.OptLevel = opt_level;
        pm_builder.VerifyInput = true;
        pm_builder.VerifyOutput = true;
        pm_builder.Inliner = llvm::createFunctionInliningPass();
        if (opt_level >= 3u) {
            pm_builder.SLPVectorize = true;
            pm_builder.MergeFunctions = true;
        }
        pm_builder.populateModulePassManager(*pm);
        pm_builder.populateFunctionPassManager(*fpm);
    }
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
    return ostr.str();
}

void llvm_state::verify_function(llvm::Function *f)
{
    std::string err_report;
    llvm::raw_string_ostream ostr(err_report);
    if (llvm::verifyFunction(*f, &ostr) && verify) {
        // Remove function before throwing.
        f->eraseFromParent();
        throw std::invalid_argument("Function verification failed. The full error message:\n" + ostr.str());
    }
}

void llvm_state::add_varargs_expression(const std::string &name, const expression &e,
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
    } else {
        // Error reading body, remove function.
        f->eraseFromParent();
    }
}

void llvm_state::add_vecargs_expression(const std::string &name, const std::vector<std::string> &vars)
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
    // Set the name of the function argument.
    const auto arg_rng = f->args();
    assert(arg_rng.begin() != arg_rng.end() && arg_rng.begin() + 1 == arg_rng.end());
    auto &vec_arg = *arg_rng.begin();
    vec_arg.setName("arg.vector");
    // Specify that this is a read-only pointer argument.
    vec_arg.addAttr(llvm::Attribute::ReadOnly);
    // Specify that the function does not make any copies of the
    // pointer argument that outlive the function itself.
    vec_arg.addAttr(llvm::Attribute::NoCapture);

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
        ret_val->setTailCall(true);

        // Finish off the function.
        builder->CreateRet(ret_val);

        // Verify it.
        verify_function(f);
    } else {
        // Error reading body, remove function.
        f->eraseFromParent();
    }
}

void llvm_state::add_batch_expression(const std::string &name, const std::vector<std::string> &vars,
                                      unsigned batch_size)
{
    // NOTE: we support indices within the unsigned range below.
    if (vars.size() > std::numeric_limits<unsigned>::max()) {
        throw std::overflow_error("The number of variables in an expression, " + std::to_string(vars.size())
                                  + ", is too large");
    }

    // Prepare the function prototype. Two pointers, one out, one in.
    std::vector<llvm::Type *> fargs(2, llvm::PointerType::getUnqual(builder->getDoubleTy()));

    // Then the return type.
    auto *ft = llvm::FunctionType::get(builder->getVoidTy(), fargs, false);
    assert(ft != nullptr);

    // Now create the function.
    auto *f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name + ".batch", module.get());
    assert(f != nullptr);

    // Set the name of the function arguments.
    const auto arg_rng = f->args();
    assert(arg_rng.begin() != arg_rng.end() && arg_rng.begin() + 2 == arg_rng.end());

    // The output argument.
    auto out_arg = arg_rng.begin();
    out_arg->setName("batcharg.out");
    // Specify that this is a write-only pointer argument.
    out_arg->addAttr(llvm::Attribute::WriteOnly);
    // Specify that the function does not make any copies of the
    // pointer argument that outlive the function itself.
    out_arg->addAttr(llvm::Attribute::NoCapture);
    // No aliasing is allowed on the pointer arguments
    // for this function.
    out_arg->addAttr(llvm::Attribute::NoAlias);

    // The input argument.
    auto in_arg = out_arg + 1;
    in_arg->setName("batcharg.in");
    // Specify that this is a read-only pointer argument.
    in_arg->addAttr(llvm::Attribute::ReadOnly);
    // Specify that the function does not make any copies of the
    // pointer argument that outlive the function itself.
    in_arg->addAttr(llvm::Attribute::NoCapture);
    // No aliasing is allowed on the pointer arguments
    // for this function.
    in_arg->addAttr(llvm::Attribute::NoAlias);

    // Lookup the vector function.
    auto vec_f = module->getFunction(name + ".vecargs");
    assert(vec_f != nullptr);

    // Create a new basic block to start insertion into.
    auto *bb = llvm::BasicBlock::Create(get_context(), "entry", f);
    assert(bb != nullptr);
    builder->SetInsertPoint(bb);

    // Init the variable for the start of the loop (i = 0).
    auto start_val = builder->getInt32(0);
    assert(start_val);

    // Make the new basic block for the loop header,
    // inserting after current block.
    assert(builder->GetInsertBlock()->getParent() == f);
    auto *preheader_bb = builder->GetInsertBlock();
    auto *loop_bb = llvm::BasicBlock::Create(get_context(), "loop", f);

    // Insert an explicit fall through from the current block to the loop_bb.
    builder->CreateBr(loop_bb);

    // Start insertion in loop_bb.
    builder->SetInsertPoint(loop_bb);

    // Start the PHI node with an entry for Start.
    auto *variable = builder->CreatePHI(builder->getInt32Ty(), 2, "i");
    variable->addIncoming(start_val, preheader_bb);

    // Emit the body of the loop.
    // First we need to compute the pointers corresponding to this
    // loop iteration.
    // The output pointer.
    auto out_ptr = builder->CreateInBoundsGEP(
        // The underlying type for the array.
        builder->getDoubleTy(),
        // The array (that is, the pointer argument passed
        // to this function).
        &*out_arg,
        // The offset.
        variable,
        // Name for the pointer variable.
        "out_ptr");

    // The input pointer.
    auto in_ptr = builder->CreateInBoundsGEP(
        // The underlying type for the array.
        builder->getDoubleTy(),
        // The array (that is, the pointer argument passed
        // to this function).
        &*in_arg,
        // The offset.
        builder->CreateMul(variable, builder->getInt32(static_cast<std::uint32_t>(vars.size())), "in_offset"),
        // Name for the pointer variable.
        "in_ptr");

    // Invoke the vector function, and store the result in out_ptr.
    auto vec_f_call = builder->CreateCall(vec_f, {in_ptr}, "calltmp");
    vec_f_call->setTailCall(true);
    builder->CreateStore(vec_f_call, out_ptr);

    // Compute the next value of the iteration.
    auto *next_var = builder->CreateAdd(variable, builder->getInt32(1), "nextvar");

    // Compute the end condition.
    auto *end_cond = builder->CreateICmp(llvm::CmpInst::ICMP_ULT, next_var, builder->getInt32(batch_size), "loopcond");

    // Create the "after loop" block and insert it.
    auto *loop_end_bb = builder->GetInsertBlock();
    auto *after_bb = llvm::BasicBlock::Create(get_context(), "afterloop", f);

    // Insert the conditional branch into the end of loop_end_bb.
    builder->CreateCondBr(end_cond, loop_bb, after_bb);

    // Any new code will be inserted in after_bb.
    builder->SetInsertPoint(after_bb);

    // Add a new entry to the PHI node for the backedge.
    variable->addIncoming(next_var, loop_end_bb);

    // Finish off the function.
    builder->CreateRetVoid();

    // Verify it.
    verify_function(f);
}

void llvm_state::add_expression(const std::string &name, const expression &e, unsigned batch_size)
{
    detail::check_symbol_name(name);

    if (module->getNamedValue(name) != nullptr) {
        throw std::invalid_argument("The name '" + name + "' already exists in the module");
    }

    // Fetch the number and names of the
    // variables from the expression.
    const auto vars = e.get_variables();

    add_varargs_expression(name, e, vars);
    add_vecargs_expression(name, vars);
    add_batch_expression(name, vars, batch_size);

    // Run the optimization pass.
    if (opt_level > 0u) {
        pm->run(*module);
    }
}

void llvm_state::compile()
{
    jitter.add_module(std::move(module));
}

std::uintptr_t llvm_state::jit_lookup(const std::string &name)
{
    auto sym = llvm::ExitOnError()(jitter.lookup(name));
    return static_cast<std::uintptr_t>(sym.getAddress());
}

llvm_state::f_ptr llvm_state::fetch(const std::string &name)
{
    return reinterpret_cast<f_ptr>(jit_lookup(name + ".vecargs"));
}

llvm_state::f_batch_ptr llvm_state::fetch_batch(const std::string &name)
{
    return reinterpret_cast<f_batch_ptr>(jit_lookup(name + ".batch"));
}

void llvm_state::set_verify(bool f)
{
    verify = f;
}

bool llvm_state::get_verify() const
{
    return verify;
}

} // namespace lambdifier
