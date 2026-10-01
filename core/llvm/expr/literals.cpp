#include "llvm_codegen.hpp"
#include <iostream>

llvm::Value* NumberExprAST::codegen(LLVMCodeGen& g) {
    if (is_float) {
        return llvm::ConstantFP::get(g.Context, llvm::APFloat(std::stod(value)));
    } else {
        bool fits_in_64 = false;
        if (value.length() < 19) {
            fits_in_64 = true;
        } else if (value.length() == 19) {
            if (value <= "9223372036854775807") {
                fits_in_64 = true;
            }
        }
        
        if (fits_in_64) {
            long long val = std::stoll(value);
            return llvm::ConstantInt::get(g.Context, llvm::APInt(64, val, true));
        }
        
        // Large integer: compile as call to np_rt_var_create_int128 or np_rt_var_create_int256
        auto strPtr = g.Builder.CreateGlobalStringPtr(value);
        if (value.length() <= 38) {
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_int128"), {strPtr});
        } else {
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_int256"), {strPtr});
        }
    }
}

llvm::Value* StringExprAST::codegen(LLVMCodeGen& g) {
    auto strPtr = g.Builder.CreateGlobalStringPtr(value);
    return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {strPtr});
}

llvm::Value* BoolExprAST::codegen(LLVMCodeGen& g) {
    return llvm::ConstantInt::get(g.Context, llvm::APInt(1, value ? 1 : 0));
}

llvm::Value* NilExprAST::codegen(LLVMCodeGen& g) {
    return llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0));
}

llvm::Value* NamedArgExprAST::codegen(LLVMCodeGen& g) {
    return value ? value->codegen(g) : nullptr;
}

llvm::Value* VariableExprAST::codegen(LLVMCodeGen& g) {
    auto alloca = g.NamedValues[name];
    if (!alloca) {
        if (name == "sys_argv") {
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_sys_get_argv"), {});
        }
        if (name == "threads_num_cpu") {
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_threads_num_cpu"), {});
        }
        if (name == "gpu_is_available") {
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_gpu_is_available"), {});
        }
        if (name == "gpu_device_count") {
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_gpu_device_count"), {});
        }
        // First-class function reference
        if (auto* fn = g.TheModule.getFunction(name)) {
            return fn;
        }
        if (auto* fn = g.getRuntimeFunction(name)) {
            return fn;
        }
        if (auto* fn = g.getRuntimeFunction("np_rt_" + name)) {
            return fn;
        }
        std::cerr << "Codegen Error: Reference to undefined variable " << name << "\n";
        exit(1);
    }
    auto typeName = g.VariableTypes[name];
    auto loadType = (typeName == "void" || typeName == "*void" || typeName == "any" || typeName == "var")
        ? llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0)
        : g.getLLVMType(typeName);
    return g.Builder.CreateLoad(loadType, alloca, name);
}

llvm::Value* ListExprAST::codegen(LLVMCodeGen& g) {
    auto list = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_list"), {});
    for (const auto& el : elements) {
        auto val = el->codegen(g);
        auto type = val->getType();
        
        bool isVar = false;
        if (type->isPointerTy()) {
            isVar = true;
            if (el->getType() == ASTNodeType::STRING_LITERAL) isVar = false;
            else if (el->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto name = static_cast<VariableExprAST*>(el.get())->name;
                if (g.VariableTypes.count(name) && g.VariableTypes[name] == "string") isVar = false;
            } else if (el->getType() == ASTNodeType::CALL_EXPR) {
                auto callee = static_cast<CallExprAST*>(el.get())->callee;
                if (callee == "string" || callee == "read_file" || callee == "input_string" || 
                    callee == "time_format" || callee == "json_stringify" || 
                    callee == "regex_find" || callee == "regex_replace" || callee == "net_recv") {
                    isVar = false;
                }
            } else if (el->getType() == ASTNodeType::SLICE_EXPR) {
                auto slice = static_cast<SliceExprAST*>(el.get());
                bool containerIsString = false;
                if (slice->container->getType() == ASTNodeType::STRING_LITERAL) containerIsString = true;
                else if (slice->container->getType() == ASTNodeType::VARIABLE_EXPR) {
                    auto name = static_cast<VariableExprAST*>(slice->container.get())->name;
                    if (g.VariableTypes.count(name) && g.VariableTypes[name] == "string") containerIsString = true;
                }
                if (containerIsString) isVar = false;
            }
        }
        
        llvm::Value* varVal = nullptr;
        if (isVar) {
            varVal = val;
        } else {
            varVal = g.promoteToVar(val, type->isIntegerTy(64) ? "int" : (type->isDoubleTy() ? "float" : (type->isIntegerTy(1) ? "bool" : "string")));
        }
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
        
        bool isVar = false;
        if (typeV->isPointerTy()) {
            isVar = true;
            if (v->getType() == ASTNodeType::STRING_LITERAL) isVar = false;
            else if (v->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto name = static_cast<VariableExprAST*>(v.get())->name;
                if (g.VariableTypes.count(name) && g.VariableTypes[name] == "string") isVar = false;
            } else if (v->getType() == ASTNodeType::CALL_EXPR) {
                auto callee = static_cast<CallExprAST*>(v.get())->callee;
                if (callee == "string" || callee == "read_file" || callee == "input_string" || 
                    callee == "time_format" || callee == "json_stringify" || 
                    callee == "regex_find" || callee == "regex_replace" || callee == "net_recv") {
                    isVar = false;
                }
            } else if (v->getType() == ASTNodeType::SLICE_EXPR) {
                auto slice = static_cast<SliceExprAST*>(v.get());
                bool containerIsString = false;
                if (slice->container->getType() == ASTNodeType::STRING_LITERAL) containerIsString = true;
                else if (slice->container->getType() == ASTNodeType::VARIABLE_EXPR) {
                    auto name = static_cast<VariableExprAST*>(slice->container.get())->name;
                    if (g.VariableTypes.count(name) && g.VariableTypes[name] == "string") containerIsString = true;
                }
                if (containerIsString) isVar = false;
            }
        }
        
        llvm::Value* varVal = nullptr;
        if (isVar) {
            varVal = valVal;
        } else {
            varVal = g.promoteToVar(valVal, typeV->isIntegerTy(64) ? "int" : (typeV->isDoubleTy() ? "float" : (typeV->isIntegerTy(1) ? "bool" : "string")));
        }
        
        auto cStrKey = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_c_str"), {keyVal});
        g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_set_key"), {dict, cStrKey, varVal});
    }
    return dict;
}
