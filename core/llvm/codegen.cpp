#include "llvm_codegen.hpp"
#include "gpu_codegen.hpp"
#include <iostream>

LLVMCodeGen::LLVMCodeGen() 
    : TheModule("np_module", Context), Builder(Context) {
    declareRuntime();
}

static bool isTopLevelCallToMain(const ASTNode* node) {
    if (!node || node->getType() != ASTNodeType::EXPR_STMT) return false;
    auto exprStmt = static_cast<const ExprStmtAST*>(node);
    if (!exprStmt->expr || exprStmt->expr->getType() != ASTNodeType::CALL_EXPR) return false;
    auto callExpr = static_cast<const CallExprAST*>(exprStmt->expr.get());
    return (callExpr->callee == "main" || callExpr->callee == "__np_user_main");
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

    // Pass 1.5: Compile GPU kernels and embed PTX in TheModule so gpu.launch can reference it
    GPUCodeGen::instance().reset();
    for (const auto& node : ast) {
        if (node->getType() == ASTNodeType::FUNC_DECL) {
            auto funcNode = static_cast<FuncDeclStmtAST*>(node.get());
            if (funcNode->is_kernel) {
                GPUCodeGen::instance().compileKernel(funcNode);
            }
        }
    }
    if (GPUCodeGen::instance().hasKernels()) {
        std::string ptx = GPUCodeGen::instance().compileToPTX();
        auto* strConst = llvm::ConstantDataArray::getString(Context, ptx, true);
        new llvm::GlobalVariable(TheModule, strConst->getType(), true,
                                 llvm::GlobalValue::InternalLinkage, strConst, "__np_gpu_ptx_code");
    }

    // Check if user defined a main function
    bool hasUserMain = false;
    for (const auto& node : ast) {
        if (node->getType() == ASTNodeType::FUNC_DECL) {
            auto funcNode = static_cast<const FuncDeclStmtAST*>(node.get());
            if (funcNode->name == "main") {
                hasUserMain = true;
                break;
            }
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
            // Skip top-level call to main() if user defined func main(),
            // as it will be invoked as the program entry point.
            if (hasUserMain && isTopLevelCallToMain(node.get())) {
                continue;
            }
            // Regular global statement compiled inside main
            node->codegen(*this);
        }
    }

    // 3. If user defined a main function, invoke it as the entry point like C/C++
    if (hasUserMain) {
        if (auto userMain = TheModule.getFunction("__np_user_main")) {
            std::vector<llvm::Value*> callArgs;
            unsigned paramCount = userMain->arg_size();
            auto paramTypes = userMain->getFunctionType()->params();

            if (paramCount == 1) {
                if (paramTypes[0]->isIntegerTy()) {
                    if (paramTypes[0]->getIntegerBitWidth() == 64) {
                        callArgs.push_back(Builder.CreateZExt(argcVal, llvm::Type::getInt64Ty(Context)));
                    } else {
                        callArgs.push_back(argcVal);
                    }
                } else {
                    auto getArgvFunc = getRuntimeFunction("np_rt_sys_get_argv");
                    if (getArgvFunc) {
                        callArgs.push_back(Builder.CreateCall(getArgvFunc, {}));
                    }
                }
            } else if (paramCount >= 2) {
                if (paramTypes[0]->isIntegerTy()) {
                    if (paramTypes[0]->getIntegerBitWidth() == 64) {
                        callArgs.push_back(Builder.CreateZExt(argcVal, llvm::Type::getInt64Ty(Context)));
                    } else {
                        callArgs.push_back(argcVal);
                    }
                } else {
                    callArgs.push_back(argcVal);
                }

                if (paramTypes[1] == argvVal->getType()) {
                    callArgs.push_back(argvVal);
                } else {
                    callArgs.push_back(Builder.CreateBitCast(argvVal, paramTypes[1]));
                }
            }

            auto userRet = Builder.CreateCall(userMain, callArgs);

            if (!Builder.GetInsertBlock()->getTerminator()) {
                if (userMain->getReturnType()->isIntegerTy()) {
                    llvm::Value* exitCode = userRet;
                    if (userRet->getType()->getIntegerBitWidth() > 32) {
                        exitCode = Builder.CreateTrunc(userRet, i32Ty);
                    } else if (userRet->getType()->getIntegerBitWidth() < 32) {
                        exitCode = Builder.CreateZExt(userRet, i32Ty);
                    }
                    Builder.CreateRet(exitCode);
                } else {
                    Builder.CreateRet(llvm::ConstantInt::get(Context, llvm::APInt(32, 0, true)));
                }
            }
        }
    }

    // 4. Close main function with return 0
    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateRet(llvm::ConstantInt::get(Context, llvm::APInt(32, 0, true)));
    }
}

llvm::AllocaInst* LLVMCodeGen::createEntryBlockAlloca(llvm::Function* fn, llvm::Type* type, const std::string& varName) {
    if (!fn) {
        fn = Builder.GetInsertBlock()->getParent();
    }
    llvm::IRBuilder<> tmpB(&fn->getEntryBlock(), fn->getEntryBlock().begin());
    return tmpB.CreateAlloca(type, nullptr, varName);
}
