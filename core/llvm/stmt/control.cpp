#include "llvm_codegen.hpp"
#include <iostream>

llvm::Value* IfStmtAST::codegen(LLVMCodeGen& g) {
    auto parentF = g.Builder.GetInsertBlock()->getParent();
    
    // Create blocks for then, else, merge
    std::vector<llvm::BasicBlock*> thenBlocks;
    std::vector<llvm::BasicBlock*> condBlocks;
    
    for (size_t i = 0; i < cases.size(); ++i) {
        thenBlocks.push_back(llvm::BasicBlock::Create(g.Context, "then", parentF));
        if (i > 0) {
            condBlocks.push_back(llvm::BasicBlock::Create(g.Context, "cond", parentF));
        }
    }
    
    auto elseBB = llvm::BasicBlock::Create(g.Context, "else");
    auto mergeBB = llvm::BasicBlock::Create(g.Context, "ifcont");
    
    // Codegen main case
    auto condV = cases[0].cond->codegen(g);
    auto nextBB = cases.size() > 1 ? condBlocks[0] : (else_block ? elseBB : mergeBB);
    g.Builder.CreateCondBr(condV, thenBlocks[0], nextBB);
    
    // Emit then blocks
    for (size_t i = 0; i < cases.size(); ++i) {
        g.Builder.SetInsertPoint(thenBlocks[i]);
        cases[i].block->codegen(g);
        if (!g.Builder.GetInsertBlock()->getTerminator()) {
            g.Builder.CreateBr(mergeBB);
        }
        
        if (i > 0) {
            condBlocks[i - 1]->moveAfter(&parentF->back());
            g.Builder.SetInsertPoint(condBlocks[i - 1]);
            auto elicond = cases[i].cond->codegen(g);
            auto elicondnext = (i + 1 < cases.size()) ? condBlocks[i] : (else_block ? elseBB : mergeBB);
            g.Builder.CreateCondBr(elicond, thenBlocks[i], elicondnext);
        }
    }
    
    // Emit else block
    elseBB->insertInto(parentF);
    g.Builder.SetInsertPoint(elseBB);
    if (else_block) {
        else_block->codegen(g);
    }
    if (!g.Builder.GetInsertBlock()->getTerminator()) {
        g.Builder.CreateBr(mergeBB);
    }
    
    // Emit merge block
    mergeBB->insertInto(parentF);
    g.Builder.SetInsertPoint(mergeBB);
    
    return nullptr;
}

llvm::Value* BreakStmtAST::codegen(LLVMCodeGen& g) {
    if (g.LoopStack.empty()) {
        std::cerr << "Codegen Error: 'break' statement outside loop\n";
        exit(1);
    }
    g.Builder.CreateBr(g.LoopStack.back().second);
    return nullptr;
}

llvm::Value* ContinueStmtAST::codegen(LLVMCodeGen& g) {
    if (g.LoopStack.empty()) {
        std::cerr << "Codegen Error: 'continue' statement outside loop\n";
        exit(1);
    }
    g.Builder.CreateBr(g.LoopStack.back().first);
    return nullptr;
}

llvm::Value* DeferStmtAST::codegen(LLVMCodeGen& g) {
    if (stmt) {
        g.DeferStack.push_back(stmt.get());
    }
    return nullptr;
}

llvm::Value* GoStmtAST::codegen(LLVMCodeGen& g) {
    if (!call) return nullptr;
    if (call->getType() == ASTNodeType::CALL_EXPR) {
        auto callExpr = static_cast<CallExprAST*>(call.get());
        auto calleeFunc = g.TheModule.getFunction(callExpr->callee);
        if (!calleeFunc) {
            calleeFunc = g.getRuntimeFunction(callExpr->callee);
        }
        if (calleeFunc) {
            static int thunk_id = 0;
            std::string thunkName = "__go_thunk_" + std::to_string(++thunk_id);
            auto voidTy = llvm::Type::getVoidTy(g.Context);
            auto i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0);
            auto ft = llvm::FunctionType::get(voidTy, {i8PtrTy}, false);
            auto thunkFunc = llvm::Function::Create(ft, llvm::Function::InternalLinkage, thunkName, g.TheModule);
            
            auto oldIP = g.Builder.saveIP();
            auto bb = llvm::BasicBlock::Create(g.Context, "entry", thunkFunc);
            g.Builder.SetInsertPoint(bb);
            
            auto argVar = thunkFunc->arg_begin();
            std::vector<llvm::Value*> actualArgs;
            unsigned paramIdx = 0;
            for (auto& param : calleeFunc->args()) {
                auto idxVal = llvm::ConstantInt::get(g.Context, llvm::APInt(64, paramIdx++));
                auto item = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_index"), {argVar, idxVal});
                llvm::Value* castVal = item;
                if (param.getType()->isIntegerTy(64)) {
                    castVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {item});
                } else if (param.getType()->isDoubleTy()) {
                    castVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_float_var"), {item});
                } else if (param.getType()->isIntegerTy(1)) {
                    auto intVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {item});
                    castVal = g.Builder.CreateICmpNE(intVal, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)));
                } else if (param.getType()->isPointerTy()) {
                    castVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_ptr_var"), {item});
                    castVal = g.Builder.CreateBitCast(castVal, param.getType());
                }
                actualArgs.push_back(castVal);
            }
            g.Builder.CreateCall(calleeFunc, actualArgs);
            g.Builder.CreateRetVoid();
            g.Builder.restoreIP(oldIP);
            
            auto listVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_list"), {});
            for (const auto& a : callExpr->args) {
                auto val = a->codegen(g);
                llvm::Value* promoted = nullptr;
                if (val->getType()->isIntegerTy(64)) {
                    promoted = g.promoteToVar(val, "int");
                } else if (val->getType()->isDoubleTy()) {
                    promoted = g.promoteToVar(val, "float");
                } else if (val->getType()->isIntegerTy(1)) {
                    promoted = g.promoteToVar(val, "bool");
                } else if (val->getType()->isPointerTy()) {
                    auto i8Ptr = g.Builder.CreateBitCast(val, llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0));
                    promoted = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_ptr"), {i8Ptr});
                } else {
                    promoted = val;
                }
                g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_append"), {listVal, promoted});
            }
            
            g.Builder.CreateCall(g.getRuntimeFunction("np_rt_go_spawn"), {thunkFunc, listVal});
            return nullptr;
        }
    }
    call->codegen(g);
    return nullptr;
}

llvm::Value* WhileStmtAST::codegen(LLVMCodeGen& g) {
    auto parentF = g.Builder.GetInsertBlock()->getParent();
    auto loopHeader = llvm::BasicBlock::Create(g.Context, "loopheader", parentF);
    auto loopBody = llvm::BasicBlock::Create(g.Context, "loopbody", parentF);
    auto loopExit = llvm::BasicBlock::Create(g.Context, "loopexit", parentF);
    
    g.LoopStack.push_back({loopHeader, loopExit});
    g.Builder.CreateBr(loopHeader);
    
    g.Builder.SetInsertPoint(loopHeader);
    auto condV = cond->codegen(g);
    g.Builder.CreateCondBr(condV, loopBody, loopExit);
    
    g.Builder.SetInsertPoint(loopBody);
    body->codegen(g);
    if (!g.Builder.GetInsertBlock()->getTerminator()) {
        g.Builder.CreateBr(loopHeader);
    }
    
    g.LoopStack.pop_back();
    g.Builder.SetInsertPoint(loopExit);
    return nullptr;
}

llvm::Value* ForStmtAST::codegen(LLVMCodeGen& g) {
    auto parentF = g.Builder.GetInsertBlock()->getParent();
    
    if (is_range) {
        auto startVal = range_start->codegen(g);
        auto endVal = range_end->codegen(g);
        
        auto loopVarType = llvm::Type::getInt64Ty(g.Context);
        auto alloca = g.Builder.CreateAlloca(loopVarType, nullptr, var_name);
        g.Builder.CreateStore(startVal, alloca);
        
        g.NamedValues[var_name] = alloca;
        g.VariableTypes[var_name] = "int";
        
        auto loopHeader = llvm::BasicBlock::Create(g.Context, "forheader", parentF);
        auto loopBody = llvm::BasicBlock::Create(g.Context, "forbody", parentF);
        auto loopInc = llvm::BasicBlock::Create(g.Context, "forinc", parentF);
        auto loopExit = llvm::BasicBlock::Create(g.Context, "forexit", parentF);
        
        g.LoopStack.push_back({loopInc, loopExit});
        g.Builder.CreateBr(loopHeader);
        
        g.Builder.SetInsertPoint(loopHeader);
        auto curVar = g.Builder.CreateLoad(loopVarType, alloca, var_name);
        auto condVal = g.Builder.CreateICmpSLT(curVar, endVal, "loopcond");
        g.Builder.CreateCondBr(condVal, loopBody, loopExit);
        
        g.Builder.SetInsertPoint(loopBody);
        body->codegen(g);
        if (!g.Builder.GetInsertBlock()->getTerminator()) {
            g.Builder.CreateBr(loopInc);
        }
        
        // Increment block
        g.Builder.SetInsertPoint(loopInc);
        auto reloadVar = g.Builder.CreateLoad(loopVarType, alloca, var_name);
        auto nextVar = g.Builder.CreateAdd(reloadVar, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 1)), "nextvar");
        g.Builder.CreateStore(nextVar, alloca);
        g.Builder.CreateBr(loopHeader);
        
        g.LoopStack.pop_back();
        g.Builder.SetInsertPoint(loopExit);
    } else {
        auto collVal = collection->codegen(g);
        auto lenVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_len"), {collVal});
        
        auto loopVarType = llvm::Type::getInt64Ty(g.Context);
        auto idxAlloca = g.Builder.CreateAlloca(loopVarType, nullptr, "idx");
        g.Builder.CreateStore(llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)), idxAlloca);
        
        auto varPtrType = llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0);
        auto varAlloca = g.Builder.CreateAlloca(varPtrType, nullptr, var_name);
        g.NamedValues[var_name] = varAlloca;
        g.VariableTypes[var_name] = "np_var";
        
        auto loopHeader = llvm::BasicBlock::Create(g.Context, "forheader", parentF);
        auto loopBody = llvm::BasicBlock::Create(g.Context, "forbody", parentF);
        auto loopInc = llvm::BasicBlock::Create(g.Context, "forinc", parentF);
        auto loopExit = llvm::BasicBlock::Create(g.Context, "forexit", parentF);
        
        g.LoopStack.push_back({loopInc, loopExit});
        g.Builder.CreateBr(loopHeader);
        
        g.Builder.SetInsertPoint(loopHeader);
        auto curIdx = g.Builder.CreateLoad(loopVarType, idxAlloca, "curidx");
        auto condVal = g.Builder.CreateICmpSLT(curIdx, lenVal, "loopcond");
        g.Builder.CreateCondBr(condVal, loopBody, loopExit);
        
        g.Builder.SetInsertPoint(loopBody);
        auto itemVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_index"), {collVal, curIdx});
        g.Builder.CreateStore(itemVal, varAlloca);
        
        body->codegen(g);
        if (!g.Builder.GetInsertBlock()->getTerminator()) {
            g.Builder.CreateBr(loopInc);
        }
        
        // Increment block
        g.Builder.SetInsertPoint(loopInc);
        auto reloadIdx = g.Builder.CreateLoad(loopVarType, idxAlloca, "curidx");
        auto nextIdx = g.Builder.CreateAdd(reloadIdx, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 1)), "nextidx");
        g.Builder.CreateStore(nextIdx, idxAlloca);
        g.Builder.CreateBr(loopHeader);
        
        g.LoopStack.pop_back();
        g.Builder.SetInsertPoint(loopExit);
    }
    return nullptr;
}

llvm::Value* TryExceptStmtAST::codegen(LLVMCodeGen& g) {
    try_block->codegen(g);
    return nullptr;
}

llvm::Value* ThrowStmtAST::codegen(LLVMCodeGen& g) {
    return nullptr;
}

llvm::Value* SwitchStmtAST::codegen(LLVMCodeGen& g) {
    auto parentF = g.Builder.GetInsertBlock()->getParent();
    auto condVal = cond->codegen(g);
    
    if (condVal->getType()->isPointerTy()) {
        condVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {condVal});
    } else if (condVal->getType()->isIntegerTy() && !condVal->getType()->isIntegerTy(64)) {
        condVal = g.Builder.CreateZExtOrTrunc(condVal, llvm::Type::getInt64Ty(g.Context));
    }
    
    auto defaultBB = llvm::BasicBlock::Create(g.Context, "sw.default");
    auto switchExit = llvm::BasicBlock::Create(g.Context, "sw.exit");
    
    g.LoopStack.push_back({switchExit, switchExit});
    
    auto switchInst = g.Builder.CreateSwitch(condVal, defaultBB, cases.size());
    
    for (size_t i = 0; i < cases.size(); ++i) {
        auto caseBB = llvm::BasicBlock::Create(g.Context, "sw.case", parentF);
        auto caseVal = cases[i].val->codegen(g);
        if (caseVal->getType()->isIntegerTy() && !caseVal->getType()->isIntegerTy(64)) {
            caseVal = g.Builder.CreateZExtOrTrunc(caseVal, llvm::Type::getInt64Ty(g.Context));
        }
        
        if (auto* constInt = llvm::dyn_cast<llvm::ConstantInt>(caseVal)) {
            switchInst->addCase(constInt, caseBB);
        } else {
            switchInst->addCase(llvm::ConstantInt::get(g.Context, llvm::APInt(64, i)), caseBB);
        }
        
        g.Builder.SetInsertPoint(caseBB);
        cases[i].body->codegen(g);
        if (!g.Builder.GetInsertBlock()->getTerminator()) {
            g.Builder.CreateBr(switchExit);
        }
    }
    
    defaultBB->insertInto(parentF);
    g.Builder.SetInsertPoint(defaultBB);
    if (default_block) {
        default_block->codegen(g);
    }
    if (!g.Builder.GetInsertBlock()->getTerminator()) {
        g.Builder.CreateBr(switchExit);
    }
    
    switchExit->insertInto(parentF);
    g.Builder.SetInsertPoint(switchExit);
    g.LoopStack.pop_back();
    
    return nullptr;
}

llvm::Value* EnumDeclStmtAST::codegen(LLVMCodeGen& g) {
    return nullptr;
}

llvm::Value* AssertStmtAST::codegen(LLVMCodeGen& g) {
    auto parentF = g.Builder.GetInsertBlock()->getParent();
    auto condVal = cond->codegen(g);
    
    // Convert condVal to i1 (boolean)
    if (condVal->getType()->isPointerTy()) {
        auto intVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {condVal});
        condVal = g.Builder.CreateICmpNE(intVal, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)));
    } else if (condVal->getType()->isIntegerTy() && !condVal->getType()->isIntegerTy(1)) {
        condVal = g.Builder.CreateICmpNE(condVal, llvm::ConstantInt::get(condVal->getType(), 0));
    }
    
    auto passBB = llvm::BasicBlock::Create(g.Context, "assert.pass", parentF);
    auto failBB = llvm::BasicBlock::Create(g.Context, "assert.fail", parentF);
    
    g.Builder.CreateCondBr(condVal, passBB, failBB);
    
    g.Builder.SetInsertPoint(failBB);
    auto lineVal = llvm::ConstantInt::get(g.Context, llvm::APInt(64, line));
    llvm::Value* msgVal = nullptr;
    if (message) {
        msgVal = message->codegen(g);
        if (message->getType() != ASTNodeType::STRING_LITERAL) {
            msgVal = g.promoteToVar(msgVal, "string");
            msgVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_string_var"), {msgVal});
        }
    } else {
        auto defMsg = g.Builder.CreateGlobalStringPtr("Assertion failed");
        msgVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {defMsg});
    }
    g.Builder.CreateCall(g.getRuntimeFunction("np_rt_assert_fail"), {lineVal, msgVal});
    g.Builder.CreateBr(passBB);
    
    g.Builder.SetInsertPoint(passBB);
    return nullptr;
}
