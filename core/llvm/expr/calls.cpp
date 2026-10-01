#include "llvm_codegen.hpp"
#include <iostream>

llvm::Value* CallExprAST::codegen(LLVMCodeGen& g) {
    if (callee_expr) {
        auto calleeVal = callee_expr->codegen(g);
        std::vector<llvm::Value*> ArgsV;
        for (unsigned i = 0, e = args.size(); i != e; ++i) {
            ArgsV.push_back(args[i]->codegen(g));
        }
        if (auto* CalleeF = llvm::dyn_cast<llvm::Function>(calleeVal)) {
            bool isVoid = CalleeF->getReturnType()->isVoidTy();
            return g.Builder.CreateCall(CalleeF, ArgsV, isVoid ? "" : "calltmp");
        } else {
            std::vector<llvm::Type*> argTypes;
            for (auto* argVal : ArgsV) {
                argTypes.push_back(argVal->getType());
            }
            auto* retType = llvm::Type::getInt64Ty(g.Context);
            auto* funcType = llvm::FunctionType::get(retType, argTypes, false);
            return g.Builder.CreateCall(funcType, calleeVal, ArgsV, "calltmp");
        }
    }

    if (g.NamedValues.count(callee)) {
        auto alloca = g.NamedValues[callee];
        auto typeName = g.VariableTypes[callee];
        auto fnPtr = g.Builder.CreateLoad(g.getLLVMType(typeName), alloca, callee);
        std::vector<llvm::Value*> ArgsV;
        for (unsigned i = 0, e = args.size(); i != e; ++i) {
            ArgsV.push_back(args[i]->codegen(g));
        }
        std::vector<llvm::Type*> argTypes;
        for (auto* argVal : ArgsV) {
            argTypes.push_back(argVal->getType());
        }
        auto* retType = llvm::Type::getInt64Ty(g.Context);
        auto* funcType = llvm::FunctionType::get(retType, argTypes, false);
        return g.Builder.CreateCall(funcType, fnPtr, ArgsV, "calltmp");
    }

    if (callee.rfind("@method_", 0) == 0) {
        std::string method = callee.substr(8);
        auto objVal = args[0]->codegen(g);
        
        // If the object is a string, promote it to np_var first
        bool isStringObj = false;
        if (args[0]->getType() == ASTNodeType::STRING_LITERAL) {
            isStringObj = true;
        } else if (args[0]->getType() == ASTNodeType::VARIABLE_EXPR) {
            auto name = static_cast<VariableExprAST*>(args[0].get())->name;
            auto varType = g.VariableTypes[name];
            if (varType == "string") isStringObj = true;
        }
        
        if (isStringObj) {
            objVal = g.promoteToVar(objVal, "string");
        }
        
        // 1. Compile arguments
        std::vector<llvm::Value*> ArgsV;
        ArgsV.push_back(objVal);
        for (size_t i = 1; i < args.size(); ++i) {
            auto val = args[i]->codegen(g);
            if (!val->getType()->isPointerTy()) {
                val = g.promoteToVar(val, val->getType()->isIntegerTy(64) ? "int" : (val->getType()->isDoubleTy() ? "float" : (val->getType()->isIntegerTy(1) ? "bool" : "string")));
            }
            ArgsV.push_back(val);
        }
        
        // 2. Map method name to runtime API function name
        std::string rt_func_name;
        if (method == "pop") rt_func_name = "np_rt_var_pop";
        else if (method == "clear") rt_func_name = "np_rt_var_clear";
        else if (method == "keys") rt_func_name = "np_rt_var_keys";
        else if (method == "values") rt_func_name = "np_rt_var_values";
        else if (method == "append") rt_func_name = "np_rt_var_append";
        else if (method == "sort") rt_func_name = "np_rt_var_sort";
        else if (method == "reverse") rt_func_name = "np_rt_var_reverse";
        else if (method == "contains") rt_func_name = "np_rt_var_contains";
        else if (method == "split") rt_func_name = "np_rt_var_split";
        else if (method == "join") rt_func_name = "np_rt_var_join";
        else if (method == "trim") rt_func_name = "np_rt_var_trim";
        else if (method == "len" || method == "length") rt_func_name = "np_rt_var_len";
        else if (method == "wait") rt_func_name = "np_rt_task_wait";
        else {
            std::cerr << "Codegen Error: Unknown method " << method << "\n";
            exit(1);
        }
        
        return g.Builder.CreateCall(g.getRuntimeFunction(rt_func_name), ArgsV);
    }

    if (callee.rfind("@interface_", 0) == 0) {
        std::string rest = callee.substr(11); // after "@interface_"
        size_t under = rest.find('_');
        std::string iface = rest.substr(0, under);
        std::string method = rest.substr(under + 1);
        
        auto objVal = args[0]->codegen(g);
        std::vector<llvm::Value*> callArgs;
        for (size_t i = 1; i < args.size(); ++i) {
            callArgs.push_back(args[i]->codegen(g));
        }
        
        auto typeKey = g.Builder.CreateGlobalStringPtr("__type__");
        auto typeVar = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_key"), {objVal, typeKey});
        auto typeStr = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_string_var"), {typeVar});
        
        auto parentF = g.Builder.GetInsertBlock()->getParent();
        auto endBB = llvm::BasicBlock::Create(g.Context, "iface.end");
        
        // Find all candidate functions matching struct_<method>
        std::vector<llvm::Function*> candidateFuncs;
        std::string methodSuffix = "_" + method;
        for (auto& func : g.TheModule.functions()) {
            std::string fName = func.getName().str();
            if (fName.length() > methodSuffix.length() &&
                fName.substr(fName.length() - methodSuffix.length()) == methodSuffix &&
                fName.find('@') == std::string::npos &&
                fName.rfind("np_rt_", 0) != 0) {
                candidateFuncs.push_back(&func);
            }
        }
        
        llvm::Type* retType = llvm::Type::getInt64Ty(g.Context);
        if (!candidateFuncs.empty()) {
            retType = candidateFuncs[0]->getReturnType();
        }
        
        auto nextCheckBB = llvm::BasicBlock::Create(g.Context, "iface.check", parentF);
        g.Builder.CreateBr(nextCheckBB);
        
        std::vector<std::pair<llvm::Value*, llvm::BasicBlock*>> phiValues;
        
        for (size_t ci = 0; ci < candidateFuncs.size(); ++ci) {
            g.Builder.SetInsertPoint(nextCheckBB);
            auto candFunc = candidateFuncs[ci];
            std::string candStruct = candFunc->getName().str().substr(0, candFunc->getName().str().length() - methodSuffix.length());
            
            auto candStrPtr = g.Builder.CreateGlobalStringPtr(candStruct);
            auto candStr = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {candStrPtr});
            auto matchCond = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_eq"), {typeStr, candStr});
            
            auto matchBB = llvm::BasicBlock::Create(g.Context, "iface.match", parentF);
            nextCheckBB = llvm::BasicBlock::Create(g.Context, "iface.next", parentF);
            
            g.Builder.CreateCondBr(matchCond, matchBB, nextCheckBB);
            
            g.Builder.SetInsertPoint(matchBB);
            std::vector<llvm::Value*> actualArgs;
            actualArgs.push_back(objVal);
            for (size_t a = 0; a < callArgs.size(); ++a) {
                actualArgs.push_back(callArgs[a]);
            }
            auto callRes = g.Builder.CreateCall(candFunc, actualArgs);
            phiValues.push_back({callRes, g.Builder.GetInsertBlock()});
            g.Builder.CreateBr(endBB);
        }
        
        g.Builder.SetInsertPoint(nextCheckBB);
        auto defRes = llvm::ConstantInt::get(retType, 0);
        phiValues.push_back({defRes, nextCheckBB});
        g.Builder.CreateBr(endBB);
        
        endBB->insertInto(parentF);
        g.Builder.SetInsertPoint(endBB);
        if (!retType->isVoidTy()) {
            auto phi = g.Builder.CreatePHI(retType, phiValues.size(), "iface.res");
            for (const auto& pv : phiValues) {
                phi->addIncoming(pv.first, pv.second);
            }
            return phi;
        }
        return nullptr;
    }

    if (callee == "print") {
        for (const auto& arg : args) {
            auto val = arg->codegen(g);
            auto type = val->getType();
            if (type->isIntegerTy(1)) {
                g.Builder.CreateCall(g.getRuntimeFunction("np_rt_print_bool"), {val});
            } else if (type->isIntegerTy()) {
                if (!type->isIntegerTy(64)) {
                    val = g.Builder.CreateZExtOrTrunc(val, llvm::Type::getInt64Ty(g.Context));
                }
                g.Builder.CreateCall(g.getRuntimeFunction("np_rt_print_int"), {val});
            } else if (type->isDoubleTy()) {
                g.Builder.CreateCall(g.getRuntimeFunction("np_rt_print_float"), {val});
            } else {
                bool isString = false;
                if (arg->getType() == ASTNodeType::STRING_LITERAL) {
                    isString = true;
                } else if (arg->getType() == ASTNodeType::VARIABLE_EXPR) {
                    auto name = static_cast<VariableExprAST*>(arg.get())->name;
                    auto varType = g.VariableTypes[name];
                    if (varType == "string") isString = true;
                } else if (arg->getType() == ASTNodeType::CALL_EXPR) {
                    auto call = static_cast<CallExprAST*>(arg.get());
                    if (call->callee == "type" || call->callee == "string" || call->callee == "input_string" ||
                        call->callee == "read_file" || call->callee == "time_format" || call->callee == "json_stringify" ||
                        call->callee == "regex_find" || call->callee == "regex_replace" || call->callee == "net_recv" ||
                        call->callee == "os_exec" || call->callee == "exec" || call->callee == "os_getenv" ||
                        call->callee == "crypto_sha256" || call->callee == "sha256" ||
                        call->callee == "gpu_device_name" || call->callee == "device_name") {
                        isString = true;
                    }
                } else if (arg->getType() == ASTNodeType::SLICE_EXPR) {
                    auto slice = static_cast<SliceExprAST*>(arg.get());
                    bool containerIsString = false;
                    if (slice->container->getType() == ASTNodeType::STRING_LITERAL) containerIsString = true;
                    else if (slice->container->getType() == ASTNodeType::VARIABLE_EXPR) {
                        auto name = static_cast<VariableExprAST*>(slice->container.get())->name;
                        auto varType = g.VariableTypes[name];
                        if (varType == "string") containerIsString = true;
                    }
                    if (containerIsString) isString = true;
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

    if (callee == "sqrt") {
        auto val = args[0]->codegen(g);
        if (val->getType()->isIntegerTy(64)) {
            val = g.Builder.CreateSIToFP(val, llvm::Type::getDoubleTy(g.Context), "floatcast");
        }
        return g.Builder.CreateCall(g.getRuntimeFunction("sqrt"), {val});
    }
    if (callee == "abs") {
        auto val = args[0]->codegen(g);
        if (val->getType()->isIntegerTy(64)) {
            val = g.Builder.CreateSIToFP(val, llvm::Type::getDoubleTy(g.Context), "floatcast");
        }
        return g.Builder.CreateCall(g.getRuntimeFunction("fabs"), {val});
    }
    if (callee == "round") {
        auto val = args[0]->codegen(g);
        if (val->getType()->isIntegerTy(64)) {
            val = g.Builder.CreateSIToFP(val, llvm::Type::getDoubleTy(g.Context), "floatcast");
        }
        return g.Builder.CreateCall(g.getRuntimeFunction("round"), {val});
    }
    if (callee == "min") {
        auto val1 = args[0]->codegen(g);
        auto val2 = args[1]->codegen(g);
        if (val1->getType()->isIntegerTy(64)) val1 = g.Builder.CreateSIToFP(val1, llvm::Type::getDoubleTy(g.Context), "floatcast");
        if (val2->getType()->isIntegerTy(64)) val2 = g.Builder.CreateSIToFP(val2, llvm::Type::getDoubleTy(g.Context), "floatcast");
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_min"), {val1, val2});
    }
    if (callee == "max") {
        auto val1 = args[0]->codegen(g);
        auto val2 = args[1]->codegen(g);
        if (val1->getType()->isIntegerTy(64)) val1 = g.Builder.CreateSIToFP(val1, llvm::Type::getDoubleTy(g.Context), "floatcast");
        if (val2->getType()->isIntegerTy(64)) val2 = g.Builder.CreateSIToFP(val2, llvm::Type::getDoubleTy(g.Context), "floatcast");
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_max"), {val1, val2});
    }

    if (callee == "len") {
        auto val = args[0]->codegen(g);
        bool isString = false;
        if (args[0]->getType() == ASTNodeType::STRING_LITERAL) {
            isString = true;
        } else if (args[0]->getType() == ASTNodeType::VARIABLE_EXPR) {
            auto name = static_cast<VariableExprAST*>(args[0].get())->name;
            auto varType = g.VariableTypes[name];
            if (varType == "string") isString = true;
        }
        
        if (isString) {
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_len"), {val});
        } else {
            return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_len"), {val});
        }
    }

    if (callee == "make_chan") {
        llvm::Value* capVal = nullptr;
        if (!args.empty()) {
            capVal = args[0]->codegen(g);
            if (!capVal->getType()->isIntegerTy(64)) {
                capVal = g.Builder.CreateZExtOrTrunc(capVal, llvm::Type::getInt64Ty(g.Context));
            }
        } else {
            capVal = llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0));
        }
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_chan_create"), {capVal});
    }

    if (callee == "chan_send") {
        auto chVal = args[0]->codegen(g);
        auto val = args[1]->codegen(g);
        if (!val->getType()->isPointerTy()) {
            val = g.promoteToVar(val, val->getType()->isIntegerTy(64) ? "int" : (val->getType()->isDoubleTy() ? "float" : (val->getType()->isIntegerTy(1) ? "bool" : "string")));
        } else if (args[1]->getType() == ASTNodeType::STRING_LITERAL) {
            val = g.promoteToVar(val, "string");
        }
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_chan_send"), {chVal, val});
    }

    if (callee == "chan_recv") {
        auto chVal = args[0]->codegen(g);
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_chan_recv"), {chVal});
    }

    if (callee == "chan_close") {
        auto chVal = args[0]->codegen(g);
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_chan_close"), {chVal});
    }

    if (callee == "threads_run" || callee == "threads") {
        if (args.empty()) {
            std::cerr << "Codegen Error: threads.run requires at least one argument (the target function)\n";
            exit(1);
        }
        std::string targetFuncName = "";
        if (args[0]->getType() == ASTNodeType::VARIABLE_EXPR) {
            targetFuncName = static_cast<VariableExprAST*>(args[0].get())->name;
        } else {
            std::cerr << "Codegen Error: First argument to threads.run must be a function identifier\n";
            exit(1);
        }

        llvm::Function* calleeFunc = g.TheModule.getFunction(targetFuncName);
        if (!calleeFunc) {
            calleeFunc = g.getRuntimeFunction(targetFuncName);
        }
        if (!calleeFunc) {
            std::cerr << "Codegen Error: Unknown function '" << targetFuncName << "' passed to threads.run\n";
            exit(1);
        }

        static int thread_thunk_id = 0;
        std::string thunkName = "__thread_thunk_" + std::to_string(++thread_thunk_id);
        auto i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0);
        auto ft = llvm::FunctionType::get(i8PtrTy, {i8PtrTy}, false);
        auto thunkFunc = llvm::Function::Create(ft, llvm::Function::InternalLinkage, thunkName, g.TheModule);

        auto oldIP = g.Builder.saveIP();
        auto bb = llvm::BasicBlock::Create(g.Context, "entry", thunkFunc);
        g.Builder.SetInsertPoint(bb);

        auto argVar = thunkFunc->arg_begin();
        std::vector<llvm::Value*> actualArgs;
        unsigned paramIdx = 0;
        for (auto& param : calleeFunc->args()) {
            auto idxVal = llvm::ConstantInt::get(g.Context, llvm::APInt(64, paramIdx));
            auto item = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_index"), {argVar, idxVal});
            llvm::Value* castVal = item;
            
            std::string npParamType = "";
            if (g.FunctionParamTypes.count(targetFuncName) && paramIdx < g.FunctionParamTypes[targetFuncName].size()) {
                npParamType = g.FunctionParamTypes[targetFuncName][paramIdx];
            }
            paramIdx++;

            if (param.getType()->isIntegerTy(64)) {
                castVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {item});
            } else if (param.getType()->isDoubleTy()) {
                castVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_float_var"), {item});
            } else if (param.getType()->isIntegerTy(1)) {
                auto intVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {item});
                castVal = g.Builder.CreateICmpNE(intVal, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)));
            } else if (param.getType()->isPointerTy()) {
                if (npParamType == "string") {
                    castVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_string_var"), {item});
                } else if (npParamType == "array" || npParamType == "dict" || npParamType == "var" || 
                           npParamType == "any" || npParamType == "auto" || npParamType.empty()) {
                    castVal = item;
                } else {
                    castVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_ptr_var"), {item});
                    castVal = g.Builder.CreateBitCast(castVal, param.getType());
                }
            }
            actualArgs.push_back(castVal);
        }

        auto callRet = g.Builder.CreateCall(calleeFunc, actualArgs);

        llvm::Value* retVar = nullptr;
        if (calleeFunc->getReturnType()->isVoidTy()) {
            retVar = llvm::ConstantPointerNull::get(i8PtrTy);
        } else if (calleeFunc->getReturnType()->isIntegerTy(64)) {
            retVar = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_int"), {callRet});
        } else if (calleeFunc->getReturnType()->isDoubleTy()) {
            retVar = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_float"), {callRet});
        } else if (calleeFunc->getReturnType()->isIntegerTy(1)) {
            retVar = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_bool"), {callRet});
        } else if (calleeFunc->getReturnType()->isPointerTy()) {
            bool isRetString = false;
            if (g.FunctionReturnTypes.count(targetFuncName) && g.FunctionReturnTypes[targetFuncName] == "string") {
                isRetString = true;
            }
            if (isRetString) {
                retVar = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_string"), {callRet});
            } else {
                retVar = g.Builder.CreateBitCast(callRet, i8PtrTy);
            }
        } else {
            retVar = llvm::ConstantPointerNull::get(i8PtrTy);
        }

        g.Builder.CreateRet(retVar);
        g.Builder.restoreIP(oldIP);

        llvm::Value* isoVal = llvm::ConstantInt::get(llvm::Type::getInt1Ty(g.Context), 0);
        auto listVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_list"), {});
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i]->getType() == ASTNodeType::NAMED_ARG_EXPR) {
                auto named = static_cast<NamedArgExprAST*>(args[i].get());
                if (named->name == "isolated") {
                    if (named->value) {
                        auto rawIso = named->value->codegen(g);
                        if (rawIso->getType()->isIntegerTy(1)) {
                            isoVal = rawIso;
                        } else if (rawIso->getType()->isIntegerTy(64)) {
                            isoVal = g.Builder.CreateICmpNE(rawIso, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)));
                        } else if (rawIso->getType()->isPointerTy()) {
                            auto intVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {rawIso});
                            isoVal = g.Builder.CreateICmpNE(intVal, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)));
                        }
                    }
                    continue;
                }
            }
            auto val = args[i]->codegen(g);
            llvm::Value* promoted = nullptr;
            if (val->getType()->isIntegerTy(64)) {
                promoted = g.promoteToVar(val, "int");
            } else if (val->getType()->isDoubleTy()) {
                promoted = g.promoteToVar(val, "float");
            } else if (val->getType()->isIntegerTy(1)) {
                promoted = g.promoteToVar(val, "bool");
            } else if (val->getType()->isPointerTy()) {
                bool isArgStr = false;
                if (args[i]->getType() == ASTNodeType::STRING_LITERAL) isArgStr = true;
                else if (args[i]->getType() == ASTNodeType::VARIABLE_EXPR) {
                    auto vname = static_cast<VariableExprAST*>(args[i].get())->name;
                    if (g.VariableTypes.count(vname) && g.VariableTypes[vname] == "string") isArgStr = true;
                }
                if (isArgStr) {
                    promoted = g.promoteToVar(val, "string");
                } else {
                    promoted = val;
                }
            } else {
                promoted = val;
            }
            g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_append"), {listVal, promoted});
        }

        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_threads_run"), {thunkFunc, listVal, isoVal});
    }

    if (callee == "gpu_launch" || callee == "gpu_run") {
        if (args.empty()) {
            std::cerr << "Codegen Error: gpu.launch requires at least (kernel, grid, block, ...args)\n";
            exit(1);
        }
        std::string kernelName = "";
        if (args[0]->getType() == ASTNodeType::VARIABLE_EXPR) {
            kernelName = static_cast<VariableExprAST*>(args[0].get())->name;
        } else if (args[0]->getType() == ASTNodeType::STRING_LITERAL) {
            kernelName = static_cast<StringExprAST*>(args[0].get())->value;
        }

        llvm::Value* gridVal = nullptr;
        llvm::Value* blockVal = nullptr;
        bool hasNamedGridOrBlock = false;
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i]->getType() == ASTNodeType::NAMED_ARG_EXPR) {
                auto named = static_cast<NamedArgExprAST*>(args[i].get());
                if (named->name == "grid" && named->value) {
                    gridVal = named->value->codegen(g);
                    hasNamedGridOrBlock = true;
                } else if (named->name == "block" && named->value) {
                    blockVal = named->value->codegen(g);
                    hasNamedGridOrBlock = true;
                }
            }
        }

        auto listVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_list"), {});
        int positionalIdx = 0;
        for (size_t i = 1; i < args.size(); ++i) {
            if (args[i]->getType() == ASTNodeType::NAMED_ARG_EXPR) {
                continue;
            }
            positionalIdx++;
            if (!hasNamedGridOrBlock && positionalIdx == 1) {
                gridVal = args[i]->codegen(g);
            } else if (!hasNamedGridOrBlock && positionalIdx == 2) {
                blockVal = args[i]->codegen(g);
            } else {
                auto val = args[i]->codegen(g);
                llvm::Value* promoted = nullptr;
                if (val->getType()->isIntegerTy(64)) {
                    promoted = g.promoteToVar(val, "int");
                } else if (val->getType()->isDoubleTy()) {
                    promoted = g.promoteToVar(val, "float");
                } else if (val->getType()->isIntegerTy(1)) {
                    promoted = g.promoteToVar(val, "bool");
                } else {
                    promoted = val;
                }
                g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_append"), {listVal, promoted});
            }
        }

        if (!gridVal) gridVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(g.Context), 1);
        if (!blockVal) blockVal = llvm::ConstantInt::get(llvm::Type::getInt64Ty(g.Context), 256);

        if (!gridVal->getType()->isIntegerTy(64)) {
            gridVal = g.Builder.CreateZExtOrTrunc(gridVal, llvm::Type::getInt64Ty(g.Context));
        }
        if (!blockVal->getType()->isIntegerTy(64)) {
            blockVal = g.Builder.CreateZExtOrTrunc(blockVal, llvm::Type::getInt64Ty(g.Context));
        }

        auto i8PtrTy = llvm::PointerType::get(llvm::Type::getInt8Ty(g.Context), 0);
        llvm::Value* ptxPtr = nullptr;
        if (auto* gv = g.TheModule.getNamedGlobal("__np_gpu_ptx_code")) {
            ptxPtr = g.Builder.CreatePointerCast(gv, i8PtrTy);
        } else {
            ptxPtr = g.Builder.CreateGlobalStringPtr("");
        }
        auto ptxStr = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {ptxPtr});

        auto kernelStrPtr = g.Builder.CreateGlobalStringPtr(kernelName);
        auto kernelStr = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_create"), {kernelStrPtr});

        g.Builder.CreateCall(g.getRuntimeFunction("np_rt_gpu_launch"), {ptxStr, kernelStr, gridVal, blockVal, listVal});
        return llvm::ConstantPointerNull::get(i8PtrTy);
    }
    
    std::string actual_callee = callee;
    if (actual_callee == "main" && g.TheModule.getFunction("__np_user_main")) actual_callee = "__np_user_main";
    else if (callee == "json_marshal") actual_callee = "json_stringify";
    else if (callee == "json_unmarshal") actual_callee = "json_parse";
    else if (callee == "exec") actual_callee = "os_exec";
    else if (callee == "sha256") actual_callee = "crypto_sha256";
    else if (callee == "threads_wait") actual_callee = "task_wait";
    else if (callee == "threads_sleep") actual_callee = "time_sleep";
    
    llvm::Function* CalleeF = g.TheModule.getFunction(actual_callee);
    if (!CalleeF) {
        CalleeF = g.getRuntimeFunction("np_rt_" + actual_callee);
        if (!CalleeF) CalleeF = g.getRuntimeFunction(actual_callee);
    }
    if (!CalleeF) {
        size_t under = actual_callee.find('_');
        while (under != std::string::npos && !CalleeF) {
            std::string fallback = actual_callee.substr(under + 1);
            CalleeF = g.TheModule.getFunction(fallback);
            if (!CalleeF) CalleeF = g.getRuntimeFunction("np_rt_" + fallback);
            if (!CalleeF) CalleeF = g.getRuntimeFunction(fallback);
            under = actual_callee.find('_', under + 1);
        }
    }
    
    if (!CalleeF) {
        std::cerr << "Codegen Error: Reference to unknown function " << callee << "\n";
        exit(1);
    }
    
    std::vector<llvm::Value*> ArgsV;
    for (unsigned i = 0, e = args.size(); i != e; ++i) {
        auto val = args[i]->codegen(g);
        if (i < CalleeF->getFunctionType()->getNumParams()) {
            auto expectedType = CalleeF->getFunctionType()->getParamType(i);
            std::string npParamType = "";
            if (g.FunctionParamTypes.count(actual_callee) && i < g.FunctionParamTypes[actual_callee].size()) {
                npParamType = g.FunctionParamTypes[actual_callee][i];
            }
            bool isGenericParam = (npParamType == "T" || npParamType == "U" || npParamType == "V" || 
                                   npParamType == "any" || npParamType == "var");
            
            if (isGenericParam && val->getType()->isPointerTy()) {
                bool isCallerString = (args[i]->getType() == ASTNodeType::STRING_LITERAL);
                if (args[i]->getType() == ASTNodeType::VARIABLE_EXPR) {
                    auto vname = static_cast<VariableExprAST*>(args[i].get())->name;
                    if (g.VariableTypes.count(vname) && g.VariableTypes[vname] == "string") isCallerString = true;
                }
                if (isCallerString) {
                    val = g.promoteToVar(val, "string");
                }
            } else if (expectedType->isPointerTy() && !val->getType()->isPointerTy()) {
                val = g.promoteToVar(val, val->getType()->isIntegerTy(64) ? "int" : (val->getType()->isDoubleTy() ? "float" : "bool"));
            } else if (!expectedType->isPointerTy() && val->getType()->isPointerTy()) {
                if (expectedType->isIntegerTy(64)) {
                    val = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {val});
                } else if (expectedType->isDoubleTy()) {
                    val = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_float_var"), {val});
                } else if (expectedType->isIntegerTy(1)) {
                    auto intVal = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_to_int_var"), {val});
                    val = g.Builder.CreateICmpNE(intVal, llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0)));
                }
            } else if (expectedType->isIntegerTy(64) && val->getType()->isIntegerTy() && !val->getType()->isIntegerTy(64)) {
                val = g.Builder.CreateZExtOrTrunc(val, llvm::Type::getInt64Ty(g.Context));
            }
        }
        ArgsV.push_back(val);
    }
    
    bool isVoid = CalleeF->getReturnType()->isVoidTy();
    return g.Builder.CreateCall(CalleeF, ArgsV, isVoid ? "" : "calltmp");
}

llvm::Value* IndexAccessExprAST::codegen(LLVMCodeGen& g) {
    auto cont = container->codegen(g);
    auto idx = index->codegen(g);
    auto idxType = idx->getType();
    if (idxType->isIntegerTy(64)) {
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_index"), {cont, idx});
    } else {
        auto cStrIdx = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_c_str"), {idx});
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_key"), {cont, cStrIdx});
    }
}

llvm::Value* DotAccessExprAST::codegen(LLVMCodeGen& g) {
    auto objVal = object->codegen(g);
    if (member == "length" || member == "len") {
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_len"), {objVal});
    }
    if (member == "shape") {
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_shape"), {objVal});
    }
    auto strMember = g.Builder.CreateGlobalStringPtr(member);
    return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_get_key"), {objVal, strMember});
}

llvm::Value* SliceExprAST::codegen(LLVMCodeGen& g) {
    auto contVal = container->codegen(g);
    
    llvm::Value* startVal = nullptr;
    if (start) {
        startVal = start->codegen(g);
    } else {
        startVal = llvm::ConstantInt::get(g.Context, llvm::APInt(64, 0));
    }
    
    llvm::Value* endVal = nullptr;
    if (end) {
        endVal = end->codegen(g);
    } else {
        endVal = llvm::ConstantInt::get(g.Context, llvm::APInt(64, -999999, true));
    }
    
    bool isString = false;
    if (container->getType() == ASTNodeType::STRING_LITERAL) {
        isString = true;
    } else if (container->getType() == ASTNodeType::VARIABLE_EXPR) {
        auto name = static_cast<VariableExprAST*>(container.get())->name;
        auto varType = g.VariableTypes[name];
        if (varType == "string") isString = true;
    }
    
    if (isString) {
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_string_slice"), {contVal, startVal, endVal});
    } else {
        return g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_slice"), {contVal, startVal, endVal});
    }
}
