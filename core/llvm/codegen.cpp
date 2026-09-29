#include "llvm_codegen.hpp"

LLVMCodeGen::LLVMCodeGen() 
    : TheModule("np_module", Context), Builder(Context) {
    declareRuntime();
}

void LLVMCodeGen::compile(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    // 1. Declare and setup the main function
    auto i32Ty = llvm::Type::getInt32Ty(Context);
    auto i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(Context), 0);
    auto i8PtrPtrTy = llvm::PointerType::get(i8PtrTy, 0); // char**
    auto mainFuncType = llvm::FunctionType::get(i32Ty, {i32Ty, i8PtrPtrTy}, false);
    auto mainFunc = llvm::Function::Create(mainFuncType, llvm::Function::ExternalLinkage, "main", TheModule);
    
    auto mainBB = llvm::BasicBlock::Create(Context, "entry", mainFunc);
    Builder.SetInsertPoint(mainBB);
    
    // Call np_rt_sys_init_args(argc, argv)
    auto mainArgs = mainFunc->arg_begin();
    llvm::Value* argcVal = &(*mainArgs);
    llvm::Value* argvVal = &(*++mainArgs);
    Builder.CreateCall(getRuntimeFunction("np_rt_sys_init_args"), {argcVal, argvVal});
    
    // Pass 1: Declare all struct types and function prototypes first (order-independent forward calls)
    for (const auto& node : ast) {
        if (node->getType() == ASTNodeType::STRUCT_DECL) {
            node->codegen(*this);
        } else if (node->getType() == ASTNodeType::FUNC_DECL) {
            static_cast<FuncDeclStmtAST*>(node.get())->declarePrototype(*this);
        }
    }

    // Pass 2: Generate code for all AST nodes
    for (const auto& node : ast) {
        if (node->getType() == ASTNodeType::FUNC_DECL) {
            // Save insert point
            auto savedBB = Builder.GetInsertBlock();
            node->codegen(*this);
            // Restore insert point
            Builder.SetInsertPoint(savedBB);
        } else if (node->getType() == ASTNodeType::STRUCT_DECL) {
            // Already declared in Pass 1
        } else {
            // Regular global statement compiled inside main
            node->codegen(*this);
        }
    }
    
    // 3. Close main function with return 0
    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateRet(llvm::ConstantInt::get(Context, llvm::APInt(32, 0, true)));
    }
}
