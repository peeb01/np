#include "../include/llvm_codegen.hpp"
#include <iostream>

llvm::Value* VarDeclStmtAST::codegen(LLVMCodeGen& g) {
    if (initializer && vars.size() > 1) {
        auto initVal = initializer->codegen(g);
        for (size_t i = 0; i < vars.size(); ++i) {
            const auto& v = vars[i];
            auto type = g.getLLVMType(v.type_name);
            auto alloca = g.Builder.CreateAlloca(type, nullptr, v.var_name);
            g.NamedValues[v.var_name] = alloca;
            g.VariableTypes[v.var_name] = v.type_name;
            
            // Extract item at index i
            auto idxVal = llvm::ConstantInt::get(g.Context, llvm::APInt(64, i));
            auto itemVar = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_index"), {initVal, idxVal}, "item");
            
            // Convert to primitive if needed
            llvm::Value* finalVal = itemVar;
            if (v.type_name == "int") {
                finalVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {itemVar}, "intval");
            } else if (v.type_name == "float") {
                finalVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_float_var"), {itemVar}, "floatval");
            } else if (v.type_name == "string") {
                finalVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_string_var"), {itemVar}, "strval");
            } else if (v.type_name == "bool") {
                auto intVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {itemVar}, "boolval_i");
                finalVal = g.Builder.CreateICmpNE(intVal, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)), "boolval");
            }
            g.Builder.CreateStore(finalVal, alloca);
        }
    } else {
        // Standard variable declaration
        for (const auto& v : vars) {
            auto type = g.getLLVMType(v.type_name);
            auto alloca = g.Builder.CreateAlloca(type, nullptr, v.var_name);
            g.NamedValues[v.var_name] = alloca;
            g.VariableTypes[v.var_name] = v.type_name;
            
            if (initializer) {
                auto val = initializer->codegen(g);
                g.Builder.CreateStore(val, alloca);
            } else {
                // Store default
                if (v.type_name == "int") {
                    g.Builder.CreateStore(llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0, true)), alloca);
                } else if (v.type_name == "float") {
                    g.Builder.CreateStore(llvm::ConstantFP::get(g.Context, llvm::APFloat(0.0)), alloca);
                } else if (v.type_name == "bool") {
                    g.Builder.CreateStore(llvm::ConstantInt::get(g.Context, llvm::APInt(1, 0)), alloca);
                } else if (v.type_name == "array") {
                    llvm::Value* list = nullptr;
                    if (v.size_expr) {
                        auto sizeVal = v.size_expr->codegen(g);
                        list = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_list_size"), {sizeVal});
                    } else {
                        list = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_list"), {});
                    }
                    g.Builder.CreateStore(list, alloca);
                } else if (v.type_name == "dict") {
                    auto dict = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_dict"), {});
                    g.Builder.CreateStore(dict, alloca);
                } else {
                    g.Builder.CreateStore(llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(g.Context)), alloca);
                }
            }
        }
    }
    return nullptr;
}

llvm::Value* VarAssignStmtAST::codegen(LLVMCodeGen& g) {
    auto rhsVal = rvalue->codegen(g);
    if (lvalue->getType() == ASTNodeType::VARIABLE_EXPR) {
        auto name = static_cast<VariableExprAST*>(lvalue.get())->name;
        auto alloca = g.NamedValues[name];
        g.Builder.CreateStore(rhsVal, alloca);
    } else if (lvalue->getType() == ASTNodeType::LIST_LITERAL) {
        // Destructuring assignment, e.g. q, r = divide(10, 3)
        auto listExpr = static_cast<ListExprAST*>(lvalue.get());
        for (size_t i = 0; i < listExpr->elements.size(); ++i) {
            auto el = listExpr->elements[i].get();
            if (el->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto name = static_cast<VariableExprAST*>(el)->name;
                auto alloca = g.NamedValues[name];
                auto typeName = g.VariableTypes[name];
                
                auto idxVal = llvm::ConstantInt::get(g.Context, llvm::APInt(64, i));
                auto itemVar = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_index"), {rhsVal, idxVal}, "item");
                
                llvm::Value* finalVal = itemVar;
                if (typeName == "int") {
                    finalVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {itemVar}, "intval");
                } else if (typeName == "float") {
                    finalVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_float_var"), {itemVar}, "floatval");
                } else if (typeName == "string") {
                    finalVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_string_var"), {itemVar}, "strval");
                } else if (typeName == "bool") {
                    auto intVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {itemVar}, "boolval_i");
                    finalVal = g.Builder.CreateICmpNE(intVal, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)), "boolval");
                }
                g.Builder.CreateStore(finalVal, alloca);
            }
        }
    } else if (lvalue->getType() == ASTNodeType::INDEX_ACCESS) {
        auto idxAccess = static_cast<IndexAccessExprAST*>(lvalue.get());
        auto cont = idxAccess->container->codegen(g);
        auto idx = idxAccess->index->codegen(g);
        
        llvm::Value* finalIdx = idx;
        if (!idx->getType()->isIntegerTy(64)) {
            finalIdx = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_c_str"), {idx});
        }
        
        llvm::Value* varRhs = nullptr;
        auto rhsType = rhsVal->getType();
        if (rhsType->isIntegerTy(64)) {
            varRhs = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_int"), {rhsVal});
        } else if (rhsType->isDoubleTy()) {
            varRhs = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_float"), {rhsVal});
        } else if (rhsType->isIntegerTy(1)) {
            varRhs = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_bool"), {rhsVal});
        } else {
            bool isRhsString = false;
            if (rvalue->getType() == ASTNodeType::STRING_LITERAL) isRhsString = true;
            else if (rvalue->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto name = static_cast<VariableExprAST*>(rvalue.get())->name;
                if (g.VariableTypes.count(name) && g.VariableTypes[name] == "string") isRhsString = true;
            } else if (rvalue->getType() == ASTNodeType::CALL_EXPR) {
                auto callee = static_cast<CallExprAST*>(rvalue.get())->callee;
                if (callee == "string" || callee == "read_file" || callee == "input_string" || 
                    callee == "time_format" || callee == "json_stringify" || 
                    callee == "regex_find" || callee == "regex_replace" || callee == "net_recv") {
                    isRhsString = true;
                }
            } else if (rvalue->getType() == ASTNodeType::SLICE_EXPR) {
                auto slice = static_cast<SliceExprAST*>(rvalue.get());
                bool containerIsString = false;
                if (slice->container->getType() == ASTNodeType::STRING_LITERAL) containerIsString = true;
                else if (slice->container->getType() == ASTNodeType::VARIABLE_EXPR) {
                    auto name = static_cast<VariableExprAST*>(slice->container.get())->name;
                    if (g.VariableTypes.count(name) && g.VariableTypes[name] == "string") containerIsString = true;
                }
                if (containerIsString) isRhsString = true;
            }
            
            if (isRhsString) {
                varRhs = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_string"), {rhsVal});
            } else {
                varRhs = rhsVal;
            }
        }
        
        if (idx->getType()->isIntegerTy(64)) {
            g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_set_index"), {cont, finalIdx, varRhs});
        } else {
            g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_set_key"), {cont, finalIdx, varRhs});
        }
    }
    return nullptr;
}

llvm::Value* BlockStmtAST::codegen(LLVMCodeGen& g) {
    for (const auto& stmt : statements) {
        stmt->codegen(g);
    }
    return nullptr;
}

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
    parentF->insert(parentF->end(), elseBB);
    g.Builder.SetInsertPoint(elseBB);
    if (else_block) {
        else_block->codegen(g);
    }
    if (!g.Builder.GetInsertBlock()->getTerminator()) {
        g.Builder.CreateBr(mergeBB);
    }
    
    // Emit merge block
    parentF->insert(parentF->end(), mergeBB);
    g.Builder.SetInsertPoint(mergeBB);
    
    return nullptr;
}

llvm::Value* WhileStmtAST::codegen(LLVMCodeGen& g) {
    auto parentF = g.Builder.GetInsertBlock()->getParent();
    auto loopHeader = llvm::BasicBlock::Create(g.Context, "loopheader", parentF);
    auto loopBody = llvm::BasicBlock::Create(g.Context, "loopbody", parentF);
    auto loopExit = llvm::BasicBlock::Create(g.Context, "loopexit", parentF);
    
    g.Builder.CreateBr(loopHeader);
    
    g.Builder.SetInsertPoint(loopHeader);
    auto condV = cond->codegen(g);
    g.Builder.CreateCondBr(condV, loopBody, loopExit);
    
    g.Builder.SetInsertPoint(loopBody);
    body->codegen(g);
    if (!g.Builder.GetInsertBlock()->getTerminator()) {
        g.Builder.CreateBr(loopHeader);
    }
    
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
        auto loopExit = llvm::BasicBlock::Create(g.Context, "forexit", parentF);
        
        g.Builder.CreateBr(loopHeader);
        
        g.Builder.SetInsertPoint(loopHeader);
        auto curVar = g.Builder.CreateLoad(loopVarType, alloca, var_name);
        auto condVal = g.Builder.CreateICmpSLT(curVar, endVal, "loopcond");
        g.Builder.CreateCondBr(condVal, loopBody, loopExit);
        
        g.Builder.SetInsertPoint(loopBody);
        body->codegen(g);
        
        // Increment
        if (!g.Builder.GetInsertBlock()->getTerminator()) {
            auto nextVar = g.Builder.CreateAdd(curVar, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 1)), "nextvar");
            g.Builder.CreateStore(nextVar, alloca);
            g.Builder.CreateBr(loopHeader);
        }
        
        g.Builder.SetInsertPoint(loopExit);
    } else {
        auto collVal = collection->codegen(g);
        auto lenVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_len"), {collVal});
        
        auto loopVarType = llvm::Type::getInt64Ty(g.Context);
        auto idxAlloca = g.Builder.CreateAlloca(loopVarType, nullptr, "idx");
        g.Builder.CreateStore(llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)), idxAlloca);
        
        auto varPtrType = llvm::PointerType::getUnqual(g.Context);
        auto varAlloca = g.Builder.CreateAlloca(varPtrType, nullptr, var_name);
        g.NamedValues[var_name] = varAlloca;
        g.VariableTypes[var_name] = "np_var";
        
        auto loopHeader = llvm::BasicBlock::Create(g.Context, "forheader", parentF);
        auto loopBody = llvm::BasicBlock::Create(g.Context, "forbody", parentF);
        auto loopExit = llvm::BasicBlock::Create(g.Context, "forexit", parentF);
        
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
            auto nextIdx = g.Builder.CreateAdd(curIdx, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 1)), "nextidx");
            g.Builder.CreateStore(nextIdx, idxAlloca);
            g.Builder.CreateBr(loopHeader);
        }
        
        g.Builder.SetInsertPoint(loopExit);
    }
    return nullptr;
}

llvm::Value* ReturnStmtAST::codegen(LLVMCodeGen& g) {
    if (expr) {
        auto val = expr->codegen(g);
        g.Builder.CreateRet(val);
    } else {
        g.Builder.CreateRetVoid();
    }
    return nullptr;
}

llvm::Value* FuncDeclStmtAST::codegen(LLVMCodeGen& g) {
    std::vector<llvm::Type*> argTypes;
    for (const auto& p : params) {
        argTypes.push_back(g.getLLVMType(p.type));
    }
    
    auto retType = g.getLLVMType(return_type);
    auto funcType = llvm::FunctionType::get(retType, argTypes, false);
    auto func = llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, g.TheModule);
    
    // Create entry basic block
    auto BB = llvm::BasicBlock::Create(g.Context, "entry", func);
    g.Builder.SetInsertPoint(BB);
    
    // Clear scopes
    g.NamedValues.clear();
    g.VariableTypes.clear();
    
    unsigned idx = 0;
    for (auto& arg : func->args()) {
        auto const& p = params[idx++];
        arg.setName(p.name);
        auto alloca = g.Builder.CreateAlloca(g.getLLVMType(p.type), nullptr, p.name);
        g.Builder.CreateStore(&arg, alloca);
        g.NamedValues[p.name] = alloca;
        g.VariableTypes[p.name] = p.type;
    }
    
    body->codegen(g);
    
    // Check if block has return instruction
    if (!g.Builder.GetInsertBlock()->getTerminator()) {
        if (return_type == "void" || return_type == "") {
            g.Builder.CreateRetVoid();
        } else {
            // Default return value
            g.Builder.CreateRet(llvm::ConstantPointerNull::get(llvm::PointerType::getUnqual(g.Context)));
        }
    }
    
    return func;
}

llvm::Value* StructDeclStmtAST::codegen(LLVMCodeGen& g) {
    std::vector<llvm::Type*> argTys;
    for (const auto& field : fields) {
        argTys.push_back(g.getLLVMType(field.type));
    }
    
    auto i8PtrTy = llvm::PointerType::getUnqual(g.Context);
    auto ft = llvm::FunctionType::get(i8PtrTy, argTys, false);
    auto f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, g.TheModule);
    
    auto oldIP = g.Builder.saveIP();
    auto block = llvm::BasicBlock::Create(g.Context, "entry", f);
    g.Builder.SetInsertPoint(block);
    
    auto dictVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_dict"), {});
    
    auto argsIt = f->arg_begin();
    for (size_t i = 0; i < fields.size(); ++i, ++argsIt) {
        llvm::Value* argVal = &(*argsIt);
        llvm::Value* varVal = nullptr;
        std::string type = fields[i].type;
        
        if (type == "int" || type == "int32" || type == "int64") {
            varVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_int"), {argVal});
        } else if (type == "float" || type == "float32" || type == "float64") {
            varVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_float"), {argVal});
        } else if (type == "bool") {
            varVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_bool"), {argVal});
        } else if (type == "string") {
            varVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_string"), {argVal});
        } else {
            varVal = argVal;
        }
        
        auto strKey = g.Builder.CreateGlobalStringPtr(fields[i].name);
        g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_set_key"), {dictVal, strKey, varVal});
    }
    
    g.Builder.CreateRet(dictVal);
    g.Builder.restoreIP(oldIP);
    
    return f;
}

llvm::Value* TryExceptStmtAST::codegen(LLVMCodeGen& g) {
    try_block->codegen(g);
    return nullptr;
}

llvm::Value* ThrowStmtAST::codegen(LLVMCodeGen& g) {
    return nullptr;
}

llvm::Value* ImportStmtAST::codegen(LLVMCodeGen& g) {
    return nullptr;
}

llvm::Value* ExprStmtAST::codegen(LLVMCodeGen& g) {
    return expr->codegen(g);
}
