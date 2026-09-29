#include "llvm_codegen.hpp"

llvm::Value* BlockStmtAST::codegen(LLVMCodeGen& g) {
    for (const auto& stmt : statements) {
        stmt->codegen(g);
    }
    return nullptr;
}

llvm::Value* VarDeclStmtAST::codegen(LLVMCodeGen& g) {
    if (initializer && vars.size() > 1) {
        auto initVal = initializer->codegen(g);
        for (size_t i = 0; i < vars.size(); ++i) {
            const auto& v = vars[i];
            auto type = g.getLLVMType(v.type_name);
            auto alloca = g.Builder.CreateAlloca(type, nullptr, v.var_name);
            g.NamedValues[v.var_name] = alloca;
            g.VariableTypes[v.var_name] = v.type_name;
            
            auto idxVal = llvm::ConstantInt::get(g.Context, llvm::APInt(64, i));
            auto itemVar = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_index"), {initVal, idxVal}, "item");
            
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
        for (const auto& v : vars) {
            std::string actual_type = v.type_name;
            llvm::Type* type = nullptr;
            llvm::Value* val = nullptr;
            
            if (initializer) {
                val = initializer->codegen(g);
                if (actual_type == "auto" || actual_type.empty()) {
                    type = val->getType();
                    if (type->isIntegerTy(1)) actual_type = "bool";
                    else if (type->isIntegerTy(8)) actual_type = "uint8";
                    else if (type->isIntegerTy(16)) actual_type = "int16";
                    else if (type->isIntegerTy(32)) actual_type = "int32";
                    else if (type->isIntegerTy(64)) actual_type = "int";
                    else if (type->isDoubleTy()) actual_type = "float";
                    else {
                        if (initializer->getType() == ASTNodeType::STRING_LITERAL) {
                            actual_type = "string";
                        } else if (initializer->getType() == ASTNodeType::CALL_EXPR) {
                            auto callee = static_cast<CallExprAST*>(initializer.get())->callee;
                            if (callee == "string" || callee == "input_string" || callee == "read_file" ||
                                callee == "time_format" || callee == "json_stringify" || callee == "regex_find" ||
                                callee == "regex_replace" || callee == "net_recv" ||
                                callee == "os_exec" || callee == "exec" || callee == "os_getenv" ||
                                callee == "crypto_sha256" || callee == "sha256") {
                                actual_type = "string";
                            } else {
                                actual_type = "var";
                            }
                        } else {
                            actual_type = "var";
                        }
                    }
                } else {
                    type = g.getLLVMType(actual_type);
                }
            } else {
                type = g.getLLVMType(actual_type);
            }
            
            auto alloca = g.Builder.CreateAlloca(type, nullptr, v.var_name);
            g.NamedValues[v.var_name] = alloca;
            g.VariableTypes[v.var_name] = actual_type;
            
            if (initializer) {
                llvm::Value* finalVal = val;
                bool isTargetPointer = (actual_type != "int" && actual_type != "float" && actual_type != "bool" &&
                                        actual_type != "uint8" && actual_type != "byte" && actual_type != "int8" &&
                                        actual_type != "uint16" && actual_type != "int16" && actual_type != "uint32" &&
                                        actual_type != "uint64" && actual_type != "uint");
                if (isTargetPointer && !val->getType()->isPointerTy()) {
                    finalVal = g.promoteToVar(val, val->getType()->isIntegerTy(64) ? "int" : (val->getType()->isDoubleTy() ? "float" : (val->getType()->isIntegerTy(1) ? "bool" : "string")));
                } else if (!isTargetPointer && val->getType()->isPointerTy()) {
                    if (actual_type == "int" || actual_type == "uint64" || actual_type == "uint") {
                        finalVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {val}, "intval");
                    } else if (actual_type == "float") {
                        finalVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_float_var"), {val}, "floatval");
                    } else if (actual_type == "bool") {
                        auto intVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {val}, "boolval_i");
                        finalVal = g.Builder.CreateICmpNE(intVal, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)), "boolval");
                    }
                }
                g.Builder.CreateStore(finalVal, alloca);
            } else {
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
                    g.Builder.CreateStore(llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0)), alloca);
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
        auto typeName = g.VariableTypes[name];
        
        llvm::Value* finalVal = rhsVal;
        bool isTargetPointer = (typeName != "int" && typeName != "float" && typeName != "bool");
        if (isTargetPointer && !rhsVal->getType()->isPointerTy()) {
            finalVal = g.promoteToVar(rhsVal, rhsVal->getType()->isIntegerTy(64) ? "int" : (rhsVal->getType()->isDoubleTy() ? "float" : (rhsVal->getType()->isIntegerTy(1) ? "bool" : "string")));
        } else if (!isTargetPointer && rhsVal->getType()->isPointerTy()) {
            if (typeName == "int") {
                finalVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {rhsVal}, "intval");
            } else if (typeName == "float") {
                finalVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_float_var"), {rhsVal}, "floatval");
            } else if (typeName == "bool") {
                auto intVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {rhsVal}, "boolval_i");
                finalVal = g.Builder.CreateICmpNE(intVal, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)), "boolval");
            }
        }
        g.Builder.CreateStore(finalVal, alloca);
    } else if (lvalue->getType() == ASTNodeType::LIST_LITERAL) {
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
                    callee == "regex_find" || callee == "regex_replace" || callee == "net_recv" ||
                    callee == "os_exec" || callee == "exec" || callee == "os_getenv" ||
                    callee == "crypto_sha256" || callee == "sha256") {
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
    } else if (lvalue->getType() == ASTNodeType::DOT_ACCESS) {
        auto dotAccess = static_cast<DotAccessExprAST*>(lvalue.get());
        auto cont = dotAccess->object->codegen(g);
        auto memberStr = g.Builder.CreateGlobalStringPtr(dotAccess->member);
        
        llvm::Value* varRhs = nullptr;
        auto rhsType = rhsVal->getType();
        if (rhsType->isIntegerTy(64)) {
            varRhs = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_int"), {rhsVal});
        } else if (rhsType->isIntegerTy() && !rhsType->isIntegerTy(1)) {
            auto zext = g.Builder.CreateZExtOrTrunc(rhsVal, llvm::Type::getInt64Ty(g.Context));
            varRhs = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_int"), {zext});
        } else if (rhsType->isDoubleTy()) {
            varRhs = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_float"), {rhsVal});
        } else if (rhsType->isIntegerTy(1)) {
            varRhs = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_bool"), {rhsVal});
        } else {
            varRhs = rhsVal;
        }
        g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_set_key"), {cont, memberStr, varRhs});
    }
    return nullptr;
}

llvm::Value* ImportStmtAST::codegen(LLVMCodeGen& g) {
    return nullptr;
}

llvm::Value* ExprStmtAST::codegen(LLVMCodeGen& g) {
    return expr->codegen(g);
}
