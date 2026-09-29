#include "llvm_codegen.hpp"

llvm::Value* ReturnStmtAST::codegen(LLVMCodeGen& g) {
    llvm::Value* val = nullptr;
    if (expr) {
        val = expr->codegen(g);
        auto currentF = g.Builder.GetInsertBlock()->getParent();
        auto expectedRetType = currentF->getReturnType();
        if (expectedRetType->isIntegerTy(64) && val->getType()->isPointerTy()) {
            val = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {val});
        } else if (expectedRetType->isDoubleTy() && val->getType()->isPointerTy()) {
            val = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_float_var"), {val});
        } else if (expectedRetType->isIntegerTy(1) && val->getType()->isPointerTy()) {
            auto intVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {val});
            val = g.Builder.CreateICmpNE(intVal, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)));
        }
    }
    for (auto it = g.DeferStack.rbegin(); it != g.DeferStack.rend(); ++it) {
        (*it)->codegen(g);
    }
    if (val) {
        g.Builder.CreateRet(val);
    } else {
        g.Builder.CreateRetVoid();
    }
    return nullptr;
}

llvm::Function* FuncDeclStmtAST::declarePrototype(LLVMCodeGen& g) {
    if (auto existing = g.TheModule.getFunction(name)) {
        return existing;
    }
    std::vector<std::string> paramTypeNames;
    std::vector<llvm::Type*> argTypes;
    for (const auto& p : params) {
        paramTypeNames.push_back(p.type);
        auto paramType = (p.type == "void" || p.type == "*void" || p.type == "any" || p.type == "var")
            ? llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0)
            : g.getLLVMType(p.type);
        argTypes.push_back(paramType);
    }
    g.FunctionParamTypes[name] = paramTypeNames;
    
    auto retType = g.getLLVMType(return_type);
    auto funcType = llvm::FunctionType::get(retType, argTypes, false);
    return llvm::Function::Create(funcType, llvm::Function::ExternalLinkage, name, g.TheModule);
}

llvm::Value* FuncDeclStmtAST::codegen(LLVMCodeGen& g) {
    auto func = declarePrototype(g);
    
    auto savedNamedValues = g.NamedValues;
    auto savedVariableTypes = g.VariableTypes;
    auto savedDeferStack = g.DeferStack;
    auto savedLoopStack = g.LoopStack;
    auto oldIP = g.Builder.saveIP();
    
    g.NamedValues.clear();
    g.VariableTypes.clear();
    g.DeferStack.clear();
    g.LoopStack.clear();

    // Create entry basic block
    auto BB = llvm::BasicBlock::Create(g.Context, "entry", func);
    g.Builder.SetInsertPoint(BB);
    
    unsigned idx = 0;
    for (auto& arg : func->args()) {
        auto const& p = params[idx++];
        arg.setName(p.name);
        auto paramType = (p.type == "void" || p.type == "*void" || p.type == "any" || p.type == "var")
            ? llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0)
            : g.getLLVMType(p.type);
        auto alloca = g.Builder.CreateAlloca(paramType, nullptr, p.name);
        g.Builder.CreateStore(&arg, alloca);
        g.NamedValues[p.name] = alloca;
        g.VariableTypes[p.name] = (p.type == "void") ? "*void" : p.type;
    }
    
    body->codegen(g);
    
    // Check if block has return instruction
    if (!g.Builder.GetInsertBlock()->getTerminator()) {
        for (auto it = g.DeferStack.rbegin(); it != g.DeferStack.rend(); ++it) {
            (*it)->codegen(g);
        }
        if (return_type == "void" || return_type == "") {
            g.Builder.CreateRetVoid();
        } else {
            // Default return value
            g.Builder.CreateRet(llvm::ConstantPointerNull::get(llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0)));
        }
    }
    
    g.NamedValues = savedNamedValues;
    g.VariableTypes = savedVariableTypes;
    g.DeferStack = savedDeferStack;
    g.LoopStack = savedLoopStack;
    if (oldIP.isSet()) {
        g.Builder.restoreIP(oldIP);
    }
    
    return func;
}

llvm::Value* StructDeclStmtAST::codegen(LLVMCodeGen& g) {
    std::vector<llvm::Type*> argTys;
    for (const auto& field : fields) {
        argTys.push_back(g.getLLVMType(field.type));
    }
    
    auto i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0);
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
    
    // Tag struct instance with its concrete type name for dynamic interface dispatch
    auto typeKey = g.Builder.CreateGlobalStringPtr("__type__");
    auto typeCStr = g.Builder.CreateGlobalStringPtr(name);
    auto typeStrObj = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {typeCStr});
    auto typeVar = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_string"), {typeStrObj});
    g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_set_key"), {dictVal, typeKey, typeVar});

    g.Builder.CreateRet(dictVal);
    g.Builder.restoreIP(oldIP);
    
    return f;
}

llvm::Value* InterfaceDeclStmtAST::codegen(LLVMCodeGen& g) {
    return nullptr;
}
