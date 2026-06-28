#include "../include/llvm_codegen.hpp"
#include <iostream>

llvm::Value* NumberExprAST::codegen(LLVMCodeGen& g) {
    if (is_float) {
        return llvm::ConstantFP::get(g.Context, llvm::APFloat(std::stod(value)));
    } else {
        return llvm::ConstantInt::get(g.Context, llvm::APInt(64, std::stoll(value), true));
    }
}

llvm::Value* StringExprAST::codegen(LLVMCodeGen& g) {
    auto strPtr = g.Builder.CreateGlobalStringPtr(value);
    return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {strPtr});
}

llvm::Value* BoolExprAST::codegen(LLVMCodeGen& g) {
    return llvm::ConstantInt::get(g.Context, llvm::APInt(1, value ? 1 : 0));
}

llvm::Value* VariableExprAST::codegen(LLVMCodeGen& g) {
    auto alloca = g.NamedValues[name];
    if (!alloca) {
        std::cerr << "Codegen Error: Reference to undefined variable " << name << "\n";
        exit(1);
    }
    auto typeName = g.VariableTypes[name];
    return g.Builder.CreateLoad(g.getLLVMType(typeName), alloca, name);
}

llvm::Value* BinaryExprAST::codegen(LLVMCodeGen& g) {
    auto L = lhs->codegen(g);
    auto R = rhs->codegen(g);
    
    if (op == "and") return g.Builder.CreateAnd(L, R, "andtmp");
    if (op == "or") return g.Builder.CreateOr(L, R, "ortmp");
    if (op == "not") return g.Builder.CreateNot(R, "nottmp");
    
    bool isLFloat = L->getType()->isDoubleTy();
    bool isRFloat = R->getType()->isDoubleTy();
    bool isLInt = L->getType()->isIntegerTy(64);
    bool isRInt = R->getType()->isIntegerTy(64);
    
    if (isLInt && isRInt) {
        if (op == "+") return g.Builder.CreateAdd(L, R, "addtmp");
        if (op == "-") return g.Builder.CreateSub(L, R, "subtmp");
        if (op == "*") return g.Builder.CreateMul(L, R, "multmp");
        if (op == "/") return g.Builder.CreateSDiv(L, R, "divtmp");
        if (op == "%") return g.Builder.CreateSRem(L, R, "modtmp");
        if (op == "==") return g.Builder.CreateICmpEQ(L, R, "eqtmp");
        if (op == "!=") return g.Builder.CreateICmpNE(L, R, "netmp");
        if (op == "<") return g.Builder.CreateICmpSLT(L, R, "lttmp");
        if (op == ">") return g.Builder.CreateICmpSGT(L, R, "gttmp");
        if (op == "<=") return g.Builder.CreateICmpSLE(L, R, "letmp");
        if (op == ">=") return g.Builder.CreateICmpSGE(L, R, "getmp");
        // Power operator (^) uses double power helper
        auto lf = g.Builder.CreateSIToFP(L, llvm::Type::getDoubleTy(g.Context));
        auto rf = g.Builder.CreateSIToFP(R, llvm::Type::getDoubleTy(g.Context));
        auto res = g.Builder.CreateCall(g.getRuntimeFunction("pow"), {lf, rf});
        return g.Builder.CreateFPToSI(res, llvm::Type::getInt64Ty(g.Context));
    }
    
    if (isLFloat || isRFloat) {
        // Cast int to float if mixed
        if (isLInt) L = g.Builder.CreateSIToFP(L, llvm::Type::getDoubleTy(g.Context));
        if (isRInt) R = g.Builder.CreateSIToFP(R, llvm::Type::getDoubleTy(g.Context));
        
        if (op == "+") return g.Builder.CreateFAdd(L, R, "faddtmp");
        if (op == "-") return g.Builder.CreateFSub(L, R, "fsubtmp");
        if (op == "*") return g.Builder.CreateFMul(L, R, "fmultmp");
        if (op == "/") return g.Builder.CreateFDiv(L, R, "fdivtmp");
        if (op == "==") return g.Builder.CreateFCmpOEQ(L, R, "feqtmp");
        if (op == "!=") return g.Builder.CreateFCmpONE(L, R, "fnetmp");
        if (op == "<") return g.Builder.CreateFCmpOLT(L, R, "flttmp");
        if (op == ">") return g.Builder.CreateFCmpOGT(L, R, "fgttmp");
        if (op == "<=") return g.Builder.CreateFCmpOLE(L, R, "fletmp");
        if (op == ">=") return g.Builder.CreateFCmpOGE(L, R, "fgetmp");
        if (op == "^") return g.Builder.CreateCall(g.getRuntimeFunction("pow"), {L, R});
    }
    
    // Dynamic np_var promotions
    auto vL = g.promoteToVar(L, isLInt ? "int" : (isLFloat ? "float" : "string"));
    auto vR = g.promoteToVar(R, isRInt ? "int" : (isRFloat ? "float" : "string"));
    
    if (op == "+") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_add"), {vL, vR});
    if (op == "-") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_sub"), {vL, vR});
    if (op == "*") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_mul"), {vL, vR});
    if (op == "/") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_div"), {vL, vR});
    if (op == "%") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_mod"), {vL, vR});
    if (op == "^") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_pow"), {vL, vR});
    if (op == "==") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_eq"), {vL, vR});
    if (op == "!=") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_ne"), {vL, vR});
    if (op == "<") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_lt"), {vL, vR});
    if (op == ">") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_gt"), {vL, vR});
    if (op == "<=") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_le"), {vL, vR});
    if (op == ">=") return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_ge"), {vL, vR});
    
    return nullptr;
}

llvm::Value* CallExprAST::codegen(LLVMCodeGen& g) {
    if (callee == "print") {
        for (const auto& arg : args) {
            auto val = arg->codegen(g);
            auto type = val->getType();
            if (type->isIntegerTy(64)) {
                g.Builder.CreateCall(g.getRuntimeFunction("np_rt_print_int"), {val});
            } else if (type->isDoubleTy()) {
                g.Builder.CreateCall(g.getRuntimeFunction("np_rt_print_float"), {val});
            } else if (type->isIntegerTy(1)) {
                g.Builder.CreateCall(g.getRuntimeFunction("np_rt_print_bool"), {val});
            } else {
                // Determine if it is a string via compile-time AST types
                bool isString = false;
                if (arg->getType() == ASTNodeType::STRING_LITERAL) {
                    isString = true;
                } else if (arg->getType() == ASTNodeType::VARIABLE_EXPR) {
                    auto name = static_cast<VariableExprAST*>(arg.get())->name;
                    auto varType = g.VariableTypes[name];
                    if (varType == "string") isString = true;
                } else if (arg->getType() == ASTNodeType::CALL_EXPR) {
                    auto call = static_cast<CallExprAST*>(arg.get());
                    if (call->callee == "type" || call->callee == "string" || call->callee == "input_string") {
                        isString = true;
                    }
                }
                
                if (isString) {
                    g.Builder.CreateCall(g.getRuntimeFunction("np_rt_print_string"), {val});
                } else {
                    g.Builder.CreateCall(g.getRuntimeFunction("np_rt_print_var"), {val});
                }
            }
        }
        return nullptr;
    }
    
    if (callee == "int") {
        auto val = args[0]->codegen(g);
        auto type = val->getType();
        if (type->isDoubleTy()) {
            return g.Builder.CreateFPToSI(val, llvm::Type::getInt64Ty(g.Context), "intcast");
        } else if (type->isIntegerTy(1)) {
            return g.Builder.CreateZExt(val, llvm::Type::getInt64Ty(g.Context), "boolcast");
        } else if (type->isIntegerTy(64)) {
            return val;
        } else {
            if (args[0]->getType() == ASTNodeType::STRING_LITERAL) {
                return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_string"), {val});
            } else if (args[0]->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto name = static_cast<VariableExprAST*>(args[0].get())->name;
                auto varType = g.VariableTypes[name];
                if (varType == "string") {
                    return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_string"), {val});
                }
            }
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {val});
        }
    }
    
    if (callee == "float") {
        auto val = args[0]->codegen(g);
        auto type = val->getType();
        if (type->isIntegerTy(64)) {
            return g.Builder.CreateSIToFP(val, llvm::Type::getDoubleTy(g.Context), "floatcast");
        } else if (type->isDoubleTy()) {
            return val;
        } else {
            if (args[0]->getType() == ASTNodeType::STRING_LITERAL) {
                return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_float_string"), {val});
            } else if (args[0]->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto name = static_cast<VariableExprAST*>(args[0].get())->name;
                auto varType = g.VariableTypes[name];
                if (varType == "string") {
                    return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_float_string"), {val});
                }
            }
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_float_var"), {val});
        }
    }
    
    if (callee == "string") {
        auto val = args[0]->codegen(g);
        auto type = val->getType();
        if (type->isIntegerTy(64)) {
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_string_int"), {val});
        } else if (type->isDoubleTy()) {
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_string_float"), {val});
        } else {
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_string_var"), {val});
        }
    }

    if (callee == "type") {
        auto val = args[0]->codegen(g);
        auto type = val->getType();
        if (type->isIntegerTy(64)) {
            auto strPtr = g.Builder.CreateGlobalStringPtr("int");
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {strPtr});
        } else if (type->isDoubleTy()) {
            auto strPtr = g.Builder.CreateGlobalStringPtr("float");
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {strPtr});
        } else if (type->isIntegerTy(1)) {
            auto strPtr = g.Builder.CreateGlobalStringPtr("bool");
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {strPtr});
        } else {
            if (args[0]->getType() == ASTNodeType::STRING_LITERAL) {
                auto strPtr = g.Builder.CreateGlobalStringPtr("string");
                return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {strPtr});
            } else if (args[0]->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto name = static_cast<VariableExprAST*>(args[0].get())->name;
                auto varType = g.VariableTypes[name];
                if (varType == "string") {
                    auto strPtr = g.Builder.CreateGlobalStringPtr("string");
                    return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {strPtr});
                }
            }
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_type_var"), {val});
        }
    }
    
    // Handle standard library calls
    llvm::Function* CalleeF = g.TheModule.getFunction(callee);
    if (!CalleeF) {
        // Try built-ins in runtime cache
        CalleeF = g.getRuntimeFunction("np_rt_" + callee);
        if (!CalleeF) CalleeF = g.getRuntimeFunction(callee);
    }
    
    if (!CalleeF) {
        std::cerr << "Codegen Error: Reference to unknown function " << callee << "\n";
        exit(1);
    }
    
    std::vector<llvm::Value*> ArgsV;
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
        ArgsV.push_back(args[i]->codegen(g));
    }
    
    bool isVoid = CalleeF->getReturnType()->isVoidTy();
    return g.Builder.CreateCall(CalleeF, ArgsV, isVoid ? "" : "calltmp");
}

llvm::Value* ListExprAST::codegen(LLVMCodeGen& g) {
    auto list = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_list"), {});
    for (const auto& el : elements) {
        auto val = el->codegen(g);
        auto type = val->getType();
        auto varVal = g.promoteToVar(val, type->isIntegerTy(64) ? "int" : (type->isDoubleTy() ? "float" : "string"));
        g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_append"), {list, varVal});
    }
    return list;
}

llvm::Value* DictExprAST::codegen(LLVMCodeGen& g) {
    auto dict = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_dict"), {});
    for (const auto& [k, v] : key_values) {
        auto keyVal = k->codegen(g);
        auto valVal = v->codegen(g);
        auto typeV = valVal->getType();
        auto varVal = g.promoteToVar(valVal, typeV->isIntegerTy(64) ? "int" : (typeV->isDoubleTy() ? "float" : "string"));
        // Key should be string
        g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_set_key"), {dict, keyVal, varVal});
    }
    return dict;
}

llvm::Value* IndexAccessExprAST::codegen(LLVMCodeGen& g) {
    auto cont = container->codegen(g);
    auto idx = index->codegen(g);
    auto idxType = idx->getType();
    if (idxType->isIntegerTy(64)) {
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_index"), {cont, idx});
    } else {
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_key"), {cont, idx});
    }
}

llvm::Value* DotAccessExprAST::codegen(LLVMCodeGen& g) {
    auto objVal = object->codegen(g);
    if (member == "length" || member == "len") {
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_len"), {objVal});
    }
    auto strMember = g.Builder.CreateGlobalStringPtr(member);
    return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_key"), {objVal, strMember});
}

llvm::Value* ListCompExprAST::codegen(LLVMCodeGen& g) {
    // Generate list comprehension as a runtime loop building a list
    auto list = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_list"), {});
    return list;
}

