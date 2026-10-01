#include "gpu_codegen.hpp"
#include <iostream>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Constants.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Support/raw_ostream.h>

GPUCodeGen& GPUCodeGen::instance() {
    static GPUCodeGen inst;
    return inst;
}

GPUCodeGen::GPUCodeGen() {
    reset();
}

GPUCodeGen::~GPUCodeGen() {}

void GPUCodeGen::reset() {
    builder.reset();
    mod.reset();
    ctx.reset();
    ctx = std::make_unique<llvm::LLVMContext>();
    mod = std::make_unique<llvm::Module>("np_gpu_device", *ctx);
    mod->setTargetTriple("nvptx64-nvidia-cuda");
    mod->setDataLayout("e-p:64:64:64-i1:8:8-i8:8:8-i16:16:16-i32:32:32-i64:64:64-f32:32:32-f64:64:64-v16:16:16-v32:32:32-v64:64:64-v128:128:128-n16:32:64");
    kernel_names.clear();
}

llvm::Type* GPUCodeGen::getGpuType(const std::string& type_name, bool is_param_pointer) {
    if (is_param_pointer) {
        return llvm::PointerType::get(*ctx, 0); // LLVM 18 opaque pointer
    }
    if (type_name == "float" || type_name == "float32") {
        return llvm::Type::getFloatTy(*ctx);
    }
    if (type_name == "double" || type_name == "float64") {
        return llvm::Type::getDoubleTy(*ctx);
    }
    if (type_name == "int" || type_name == "int64") {
        return llvm::Type::getInt64Ty(*ctx);
    }
    if (type_name == "int32") {
        return llvm::Type::getInt32Ty(*ctx);
    }
    if (type_name == "bool") {
        return llvm::Type::getInt1Ty(*ctx);
    }
    // Default scalar type is int64
    return llvm::Type::getInt64Ty(*ctx);
}

void GPUCodeGen::compileKernel(FuncDeclStmtAST* funcAst) {
    if (!funcAst) return;

    std::vector<llvm::Type*> paramTypes;
    std::vector<std::string> paramNames;
    std::unordered_map<std::string, llvm::Type*> elemTypes;

    for (const auto& p : funcAst->params) {
        bool isPointer = (p.type == "array" || p.type.find('*') != std::string::npos ||
                          p.type == "auto" || p.type.empty() || p.type == "var");
        paramTypes.push_back(getGpuType(p.type, isPointer));
        paramNames.push_back(p.name);
        
        // Determine element type for memory operations (defaults to double in np-lang)
        if (p.type == "float32") {
            elemTypes[p.name] = llvm::Type::getFloatTy(*ctx);
        } else if (p.type.find("int") != std::string::npos) {
            elemTypes[p.name] = llvm::Type::getInt64Ty(*ctx);
        } else {
            elemTypes[p.name] = llvm::Type::getDoubleTy(*ctx);
        }
    }

    auto ft = llvm::FunctionType::get(llvm::Type::getVoidTy(*ctx), paramTypes, false);
    auto kernelFunc = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, funcAst->name, mod.get());
    kernel_names.push_back(funcAst->name);

    // NVVM metadata: mark function as GPU entry kernel
    auto* mdKernel = llvm::MDString::get(*ctx, "kernel");
    auto* mdOne = llvm::ConstantAsMetadata::get(llvm::ConstantInt::get(llvm::Type::getInt32Ty(*ctx), 1));
    auto* mdNode = llvm::MDNode::get(*ctx, {llvm::ValueAsMetadata::get(kernelFunc), mdKernel, mdOne});
    mod->getOrInsertNamedMetadata("nvvm.annotations")->addOperand(mdNode);

    auto bb = llvm::BasicBlock::Create(*ctx, "entry", kernelFunc);
    builder = std::make_unique<llvm::IRBuilder<>>(bb);
    auto& b = *builder;

    std::unordered_map<std::string, llvm::Value*> namedValues;
    std::unordered_map<std::string, llvm::Type*> valueTypes;

    unsigned idx = 0;
    for (auto& arg : kernelFunc->args()) {
        arg.setName(paramNames[idx]);
        auto* alloca = b.CreateAlloca(arg.getType(), nullptr, paramNames[idx] + ".addr");
        b.CreateStore(&arg, alloca);
        namedValues[paramNames[idx]] = alloca;
        valueTypes[paramNames[idx]] = elemTypes[paramNames[idx]];
        idx++;
    }

    // Codegen the function body statements
    if (funcAst->body) {
        for (const auto& stmt : funcAst->body->statements) {
            codegenStmt(stmt.get(), kernelFunc, namedValues, valueTypes);
        }
    }

    // Ensure terminating return void
    if (!builder->GetInsertBlock()->getTerminator()) {
        builder->CreateRetVoid();
    }
}

llvm::Value* GPUCodeGen::codegenExpr(ExprAST* expr, llvm::Function* kernelFunc,
                                     std::unordered_map<std::string, llvm::Value*>& namedValues,
                                     std::unordered_map<std::string, llvm::Type*>& valueTypes) {
    if (!expr) return nullptr;
    auto& b = *builder;

    switch (expr->getType()) {
        case ASTNodeType::NUMBER_LITERAL: {
            auto num = static_cast<NumberExprAST*>(expr);
            if (num->is_float) {
                return llvm::ConstantFP::get(llvm::Type::getDoubleTy(*ctx), std::stod(num->value));
            } else {
                return llvm::ConstantInt::get(*ctx, llvm::APInt(64, std::stoll(num->value), true));
            }
        }
        case ASTNodeType::BOOL_LITERAL: {
            auto bl = static_cast<BoolExprAST*>(expr);
            return llvm::ConstantInt::get(*ctx, llvm::APInt(1, bl->value ? 1 : 0));
        }
        case ASTNodeType::VARIABLE_EXPR: {
            auto vname = static_cast<VariableExprAST*>(expr)->name;
            if (namedValues.count(vname)) {
                auto* alloca = namedValues[vname];
                auto* allocaInst = llvm::dyn_cast<llvm::AllocaInst>(alloca);
                auto allocType = allocaInst ? allocaInst->getAllocatedType() : llvm::Type::getInt64Ty(*ctx);
                return b.CreateLoad(allocType, alloca, vname);
            }
            // Check if it's a GPU index variable shorthand (e.g. threadIdx.x or gpu.thread_idx_x)
            if (vname == "gpu_thread_idx_x" || vname == "thread_idx_x") {
                auto tidFunc = mod->getOrInsertFunction("llvm.nvvm.read.ptx.sreg.tid.x", llvm::Type::getInt32Ty(*ctx));
                return b.CreateZExtOrTrunc(b.CreateCall(tidFunc), llvm::Type::getInt64Ty(*ctx));
            }
            if (vname == "gpu_block_idx_x" || vname == "block_idx_x") {
                auto bidFunc = mod->getOrInsertFunction("llvm.nvvm.read.ptx.sreg.ctaid.x", llvm::Type::getInt32Ty(*ctx));
                return b.CreateZExtOrTrunc(b.CreateCall(bidFunc), llvm::Type::getInt64Ty(*ctx));
            }
            if (vname == "gpu_block_dim_x" || vname == "block_dim_x") {
                auto bdimFunc = mod->getOrInsertFunction("llvm.nvvm.read.ptx.sreg.ntid.x", llvm::Type::getInt32Ty(*ctx));
                return b.CreateZExtOrTrunc(b.CreateCall(bdimFunc), llvm::Type::getInt64Ty(*ctx));
            }
            return llvm::ConstantInt::get(*ctx, llvm::APInt(64, 0));
        }
        case ASTNodeType::INDEX_ACCESS: {
            auto idxExpr = static_cast<IndexAccessExprAST*>(expr);
            auto varName = static_cast<VariableExprAST*>(idxExpr->container.get())->name;
            auto* alloca = namedValues[varName];
            auto* ptrVal = b.CreateLoad(llvm::PointerType::get(*ctx, 0), alloca);
            auto* idxVal = codegenExpr(idxExpr->index.get(), kernelFunc, namedValues, valueTypes);
            if (!idxVal->getType()->isIntegerTy(64)) {
                idxVal = b.CreateZExtOrTrunc(idxVal, llvm::Type::getInt64Ty(*ctx));
            }
            auto elemType = valueTypes.count(varName) ? valueTypes[varName] : llvm::Type::getDoubleTy(*ctx);
            auto* gep = b.CreateGEP(elemType, ptrVal, idxVal);
            return b.CreateLoad(elemType, gep);
        }
        case ASTNodeType::CALL_EXPR: {
            auto call = static_cast<CallExprAST*>(expr);
            std::string callee = call->callee;
            if (callee == "gpu_thread_idx_x" || callee == "thread_idx_x") {
                auto fn = mod->getOrInsertFunction("llvm.nvvm.read.ptx.sreg.tid.x", llvm::Type::getInt32Ty(*ctx));
                return b.CreateZExtOrTrunc(b.CreateCall(fn), llvm::Type::getInt64Ty(*ctx));
            }
            if (callee == "gpu_thread_idx_y" || callee == "thread_idx_y") {
                auto fn = mod->getOrInsertFunction("llvm.nvvm.read.ptx.sreg.tid.y", llvm::Type::getInt32Ty(*ctx));
                return b.CreateZExtOrTrunc(b.CreateCall(fn), llvm::Type::getInt64Ty(*ctx));
            }
            if (callee == "gpu_thread_idx_z" || callee == "thread_idx_z") {
                auto fn = mod->getOrInsertFunction("llvm.nvvm.read.ptx.sreg.tid.z", llvm::Type::getInt32Ty(*ctx));
                return b.CreateZExtOrTrunc(b.CreateCall(fn), llvm::Type::getInt64Ty(*ctx));
            }
            if (callee == "gpu_block_idx_x" || callee == "block_idx_x") {
                auto fn = mod->getOrInsertFunction("llvm.nvvm.read.ptx.sreg.ctaid.x", llvm::Type::getInt32Ty(*ctx));
                return b.CreateZExtOrTrunc(b.CreateCall(fn), llvm::Type::getInt64Ty(*ctx));
            }
            if (callee == "gpu_block_idx_y" || callee == "block_idx_y") {
                auto fn = mod->getOrInsertFunction("llvm.nvvm.read.ptx.sreg.ctaid.y", llvm::Type::getInt32Ty(*ctx));
                return b.CreateZExtOrTrunc(b.CreateCall(fn), llvm::Type::getInt64Ty(*ctx));
            }
            if (callee == "gpu_block_dim_x" || callee == "block_dim_x") {
                auto fn = mod->getOrInsertFunction("llvm.nvvm.read.ptx.sreg.ntid.x", llvm::Type::getInt32Ty(*ctx));
                return b.CreateZExtOrTrunc(b.CreateCall(fn), llvm::Type::getInt64Ty(*ctx));
            }
            if (callee == "gpu_grid_dim_x" || callee == "grid_dim_x") {
                auto fn = mod->getOrInsertFunction("llvm.nvvm.read.ptx.sreg.nctaid.x", llvm::Type::getInt32Ty(*ctx));
                return b.CreateZExtOrTrunc(b.CreateCall(fn), llvm::Type::getInt64Ty(*ctx));
            }
            if (callee == "gpu_sync_threads" || callee == "sync_threads") {
                auto fn = mod->getOrInsertFunction("llvm.nvvm.barrier0", llvm::Type::getVoidTy(*ctx));
                return b.CreateCall(fn);
            }
            return llvm::ConstantInt::get(*ctx, llvm::APInt(64, 0));
        }
        case ASTNodeType::BINARY_EXPR: {
            auto bin = static_cast<BinaryExprAST*>(expr);
            auto lhs = codegenExpr(bin->lhs.get(), kernelFunc, namedValues, valueTypes);
            auto rhs = codegenExpr(bin->rhs.get(), kernelFunc, namedValues, valueTypes);
            if (!lhs || !rhs) return nullptr;

            bool isFloat = lhs->getType()->isFloatingPointTy() || rhs->getType()->isFloatingPointTy();
            if (isFloat) {
                if (!lhs->getType()->isFloatingPointTy()) {
                    lhs = b.CreateSIToFP(lhs, llvm::Type::getDoubleTy(*ctx));
                }
                if (!rhs->getType()->isFloatingPointTy()) {
                    rhs = b.CreateSIToFP(rhs, llvm::Type::getDoubleTy(*ctx));
                }
                if (bin->op == "+") return b.CreateFAdd(lhs, rhs);
                if (bin->op == "-") return b.CreateFSub(lhs, rhs);
                if (bin->op == "*") return b.CreateFMul(lhs, rhs);
                if (bin->op == "/") return b.CreateFDiv(lhs, rhs);
                if (bin->op == "<") return b.CreateFCmpOLT(lhs, rhs);
                if (bin->op == "<=") return b.CreateFCmpOLE(lhs, rhs);
                if (bin->op == ">") return b.CreateFCmpOGT(lhs, rhs);
                if (bin->op == ">=") return b.CreateFCmpOGE(lhs, rhs);
                if (bin->op == "==") return b.CreateFCmpOEQ(lhs, rhs);
                if (bin->op == "!=") return b.CreateFCmpONE(lhs, rhs);
            } else {
                if (lhs->getType() != rhs->getType()) {
                    if (lhs->getType()->isIntegerTy() && rhs->getType()->isIntegerTy()) {
                        unsigned lBits = lhs->getType()->getIntegerBitWidth();
                        unsigned rBits = rhs->getType()->getIntegerBitWidth();
                        if (lBits < rBits) lhs = b.CreateZExt(lhs, rhs->getType());
                        else rhs = b.CreateZExt(rhs, lhs->getType());
                    }
                }
                if (bin->op == "+") return b.CreateAdd(lhs, rhs);
                if (bin->op == "-") return b.CreateSub(lhs, rhs);
                if (bin->op == "*") return b.CreateMul(lhs, rhs);
                if (bin->op == "/") return b.CreateSDiv(lhs, rhs);
                if (bin->op == "%") return b.CreateSRem(lhs, rhs);
                if (bin->op == "<") return b.CreateICmpSLT(lhs, rhs);
                if (bin->op == "<=") return b.CreateICmpSLE(lhs, rhs);
                if (bin->op == ">") return b.CreateICmpSGT(lhs, rhs);
                if (bin->op == ">=") return b.CreateICmpSGE(lhs, rhs);
                if (bin->op == "==") return b.CreateICmpEQ(lhs, rhs);
                if (bin->op == "!=") return b.CreateICmpNE(lhs, rhs);
                if (bin->op == "and") return b.CreateAnd(lhs, rhs);
                if (bin->op == "or") return b.CreateOr(lhs, rhs);
            }
            return lhs;
        }
        default:
            return llvm::ConstantInt::get(*ctx, llvm::APInt(64, 0));
    }
}

void GPUCodeGen::codegenStmt(ASTNode* node, llvm::Function* kernelFunc,
                             std::unordered_map<std::string, llvm::Value*>& namedValues,
                             std::unordered_map<std::string, llvm::Type*>& valueTypes) {
    if (!node) return;
    auto& b = *builder;

    switch (node->getType()) {
        case ASTNodeType::VAR_ASSIGN: {
            auto assign = static_cast<VarAssignStmtAST*>(node);
            auto rval = codegenExpr(assign->rvalue.get(), kernelFunc, namedValues, valueTypes);

            if (assign->lvalue->getType() == ASTNodeType::INDEX_ACCESS) {
                auto idxExpr = static_cast<IndexAccessExprAST*>(assign->lvalue.get());
                auto varName = static_cast<VariableExprAST*>(idxExpr->container.get())->name;
                auto* alloca = namedValues[varName];
                auto* ptrVal = b.CreateLoad(llvm::PointerType::get(*ctx, 0), alloca);
                auto* idxVal = codegenExpr(idxExpr->index.get(), kernelFunc, namedValues, valueTypes);
                if (!idxVal->getType()->isIntegerTy(64)) {
                    idxVal = b.CreateZExtOrTrunc(idxVal, llvm::Type::getInt64Ty(*ctx));
                }
                auto elemType = valueTypes.count(varName) ? valueTypes[varName] : llvm::Type::getDoubleTy(*ctx);
                if (rval->getType() != elemType) {
                    if (elemType->isFloatingPointTy() && rval->getType()->isIntegerTy()) {
                        rval = b.CreateSIToFP(rval, elemType);
                    } else if (elemType->isIntegerTy() && rval->getType()->isFloatingPointTy()) {
                        rval = b.CreateFPToSI(rval, elemType);
                    }
                }
                auto* gep = b.CreateGEP(elemType, ptrVal, idxVal);
                b.CreateStore(rval, gep);
            } else if (assign->lvalue->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto varName = static_cast<VariableExprAST*>(assign->lvalue.get())->name;
                if (!namedValues.count(varName)) {
                    auto* alloca = b.CreateAlloca(rval->getType(), nullptr, varName);
                    namedValues[varName] = alloca;
                }
                b.CreateStore(rval, namedValues[varName]);
            }
            break;
        }
        case ASTNodeType::VAR_DECL: {
            auto decl = static_cast<VarDeclStmtAST*>(node);
            llvm::Value* rval = nullptr;
            if (decl->initializer) {
                rval = codegenExpr(decl->initializer.get(), kernelFunc, namedValues, valueTypes);
            }
            for (const auto& item : decl->vars) {
                auto type = (item.type_name == "auto" && rval) ? rval->getType() : getGpuType(item.type_name, false);
                auto* alloca = b.CreateAlloca(type, nullptr, item.var_name);
                namedValues[item.var_name] = alloca;
                valueTypes[item.var_name] = type;
                if (rval) {
                    if (rval->getType() != type) {
                        if (type->isFloatingPointTy() && rval->getType()->isIntegerTy()) rval = b.CreateSIToFP(rval, type);
                        else if (type->isIntegerTy() && rval->getType()->isFloatingPointTy()) rval = b.CreateFPToSI(rval, type);
                    }
                    b.CreateStore(rval, alloca);
                }
            }
            break;
        }
        case ASTNodeType::IF_STMT: {
            auto ifStmt = static_cast<IfStmtAST*>(node);
            if (ifStmt->cases.empty()) break;
            auto condVal = codegenExpr(ifStmt->cases[0].cond.get(), kernelFunc, namedValues, valueTypes);
            if (!condVal->getType()->isIntegerTy(1)) {
                condVal = b.CreateICmpNE(condVal, llvm::ConstantInt::get(condVal->getType(), 0));
            }

            auto thenBB = llvm::BasicBlock::Create(*ctx, "if.then", kernelFunc);
            auto elseBB = llvm::BasicBlock::Create(*ctx, "if.else", kernelFunc);
            auto mergeBB = llvm::BasicBlock::Create(*ctx, "if.end", kernelFunc);

            b.CreateCondBr(condVal, thenBB, ifStmt->else_block ? elseBB : mergeBB);

            // Then Block
            b.SetInsertPoint(thenBB);
            if (ifStmt->cases[0].block) {
                for (const auto& s : ifStmt->cases[0].block->statements) {
                    codegenStmt(s.get(), kernelFunc, namedValues, valueTypes);
                }
            }
            if (!b.GetInsertBlock()->getTerminator()) b.CreateBr(mergeBB);

            // Else Block
            if (ifStmt->else_block) {
                b.SetInsertPoint(elseBB);
                for (const auto& s : ifStmt->else_block->statements) {
                    codegenStmt(s.get(), kernelFunc, namedValues, valueTypes);
                }
                if (!b.GetInsertBlock()->getTerminator()) b.CreateBr(mergeBB);
            } else {
                elseBB->eraseFromParent();
            }

            // Merge Block
            b.SetInsertPoint(mergeBB);
            break;
        }
        case ASTNodeType::EXPR_STMT: {
            auto exprStmt = static_cast<ExprStmtAST*>(node);
            codegenExpr(exprStmt->expr.get(), kernelFunc, namedValues, valueTypes);
            break;
        }
        case ASTNodeType::RETURN_STMT: {
            b.CreateRetVoid();
            break;
        }
        case ASTNodeType::BLOCK_STMT: {
            auto block = static_cast<BlockStmtAST*>(node);
            for (const auto& s : block->statements) {
                codegenStmt(s.get(), kernelFunc, namedValues, valueTypes);
            }
            break;
        }
        case ASTNodeType::FOR_STMT: {
            auto forStmt = static_cast<ForStmtAST*>(node);
            if (forStmt->is_range) {
                auto startVal = codegenExpr(forStmt->range_start.get(), kernelFunc, namedValues, valueTypes);
                auto endVal = codegenExpr(forStmt->range_end.get(), kernelFunc, namedValues, valueTypes);
                if (!startVal->getType()->isIntegerTy(64)) startVal = b.CreateZExtOrTrunc(startVal, llvm::Type::getInt64Ty(*ctx));
                if (!endVal->getType()->isIntegerTy(64)) endVal = b.CreateZExtOrTrunc(endVal, llvm::Type::getInt64Ty(*ctx));

                auto loopVarType = llvm::Type::getInt64Ty(*ctx);
                auto* alloca = b.CreateAlloca(loopVarType, nullptr, forStmt->var_name);
                b.CreateStore(startVal, alloca);
                namedValues[forStmt->var_name] = alloca;
                valueTypes[forStmt->var_name] = loopVarType;

                auto loopHeader = llvm::BasicBlock::Create(*ctx, "gpu.for.header", kernelFunc);
                auto loopBody = llvm::BasicBlock::Create(*ctx, "gpu.for.body", kernelFunc);
                auto loopInc = llvm::BasicBlock::Create(*ctx, "gpu.for.inc", kernelFunc);
                auto loopExit = llvm::BasicBlock::Create(*ctx, "gpu.for.exit", kernelFunc);

                b.CreateBr(loopHeader);

                // Header
                b.SetInsertPoint(loopHeader);
                auto curVar = b.CreateLoad(loopVarType, alloca, forStmt->var_name);
                auto condVal = b.CreateICmpSLT(curVar, endVal, "gpu.loopcond");
                b.CreateCondBr(condVal, loopBody, loopExit);

                // Body
                b.SetInsertPoint(loopBody);
                if (forStmt->body) {
                    for (const auto& s : forStmt->body->statements) {
                        codegenStmt(s.get(), kernelFunc, namedValues, valueTypes);
                    }
                }
                if (!b.GetInsertBlock()->getTerminator()) {
                    b.CreateBr(loopInc);
                }

                // Increment
                b.SetInsertPoint(loopInc);
                auto reloadVar = b.CreateLoad(loopVarType, alloca, forStmt->var_name);
                auto nextVar = b.CreateAdd(reloadVar, llvm::ConstantInt::get(*ctx, llvm::APInt(64, 1)), "gpu.nextvar");
                b.CreateStore(nextVar, alloca);
                b.CreateBr(loopHeader);

                // Exit
                b.SetInsertPoint(loopExit);
            }
            break;
        }
        case ASTNodeType::WHILE_STMT: {
            auto whileStmt = static_cast<WhileStmtAST*>(node);
            auto loopHeader = llvm::BasicBlock::Create(*ctx, "gpu.while.header", kernelFunc);
            auto loopBody = llvm::BasicBlock::Create(*ctx, "gpu.while.body", kernelFunc);
            auto loopExit = llvm::BasicBlock::Create(*ctx, "gpu.while.exit", kernelFunc);

            b.CreateBr(loopHeader);

            b.SetInsertPoint(loopHeader);
            auto condVal = codegenExpr(whileStmt->cond.get(), kernelFunc, namedValues, valueTypes);
            if (!condVal->getType()->isIntegerTy(1)) {
                condVal = b.CreateICmpNE(condVal, llvm::ConstantInt::get(condVal->getType(), 0));
            }
            b.CreateCondBr(condVal, loopBody, loopExit);

            b.SetInsertPoint(loopBody);
            if (whileStmt->body) {
                for (const auto& s : whileStmt->body->statements) {
                    codegenStmt(s.get(), kernelFunc, namedValues, valueTypes);
                }
            }
            if (!b.GetInsertBlock()->getTerminator()) {
                b.CreateBr(loopHeader);
            }

            b.SetInsertPoint(loopExit);
            break;
        }
        default:
            break;
    }
}

std::string GPUCodeGen::compileToPTX(const std::string& sm_arch) {
    if (kernel_names.empty()) return "";

    LLVMInitializeNVPTXTargetInfo();
    LLVMInitializeNVPTXTarget();
    LLVMInitializeNVPTXTargetMC();
    LLVMInitializeNVPTXAsmPrinter();

    std::string errStr;
    const llvm::Target* target = llvm::TargetRegistry::lookupTarget("nvptx64-nvidia-cuda", errStr);
    if (!target) {
        std::cerr << "GPU Error: Failed to lookup nvptx64 target: " << errStr << std::endl;
        return "";
    }

    llvm::TargetOptions opt;
    auto tm = target->createTargetMachine("nvptx64-nvidia-cuda", sm_arch, "", opt, llvm::Reloc::Static);

    llvm::SmallVector<char, 0> ptxVec;
    llvm::raw_svector_ostream ptxStream(ptxVec);
    llvm::legacy::PassManager pm;
    if (tm->addPassesToEmitFile(pm, ptxStream, nullptr, llvm::CodeGenFileType::AssemblyFile)) {
        std::cerr << "GPU Error: Cannot emit PTX file" << std::endl;
        return "";
    }
    pm.run(*mod);

    return std::string(ptxVec.data(), ptxVec.size());
}
