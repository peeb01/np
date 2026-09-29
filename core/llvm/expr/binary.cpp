#include "llvm_codegen.hpp"

llvm::Value* BinaryExprAST::codegen(LLVMCodeGen& g) {
    auto L = lhs->codegen(g);
    auto R = rhs->codegen(g);
    
    if (op == "and") return g.Builder.CreateAnd(L, R, "andtmp");
    if (op == "or") return g.Builder.CreateOr(L, R, "ortmp");
    if (op == "not") return g.Builder.CreateNot(R, "nottmp");
    if (op == "~" && L->getType()->isIntegerTy() && llvm::isa<llvm::ConstantInt>(L) && llvm::cast<llvm::ConstantInt>(L)->isZero()) {
        return g.Builder.CreateNot(R, "bitnottmp");
    }
    
    if (lhs->getType() == ASTNodeType::NIL_LITERAL || rhs->getType() == ASTNodeType::NIL_LITERAL) {
        if (op == "==") return g.Builder.CreateICmpEQ(L, R, "nileq");
        if (op == "!=") return g.Builder.CreateICmpNE(L, R, "nilne");
    }
    
    bool isLFloat = L->getType()->isDoubleTy();
    bool isRFloat = R->getType()->isDoubleTy();
    bool isLBool = L->getType()->isIntegerTy(1);
    bool isRBool = R->getType()->isIntegerTy(1);
    bool isLInt = L->getType()->isIntegerTy() && !isLBool;
    bool isRInt = R->getType()->isIntegerTy() && !isRBool;
    
    if (isLInt && isRInt) {
        unsigned lBits = L->getType()->getIntegerBitWidth();
        unsigned rBits = R->getType()->getIntegerBitWidth();
        if (lBits < rBits) {
            L = g.Builder.CreateZExt(L, R->getType());
        } else if (rBits < lBits) {
            R = g.Builder.CreateZExt(R, L->getType());
        }

        if (op == "+") return g.Builder.CreateAdd(L, R, "addtmp");
        if (op == "-") return g.Builder.CreateSub(L, R, "subtmp");
        if (op == "*") return g.Builder.CreateMul(L, R, "multmp");
        if (op == "/") return g.Builder.CreateSDiv(L, R, "divtmp");
        if (op == "%") return g.Builder.CreateSRem(L, R, "modtmp");
        if (op == "&") return g.Builder.CreateAnd(L, R, "andtmp");
        if (op == "|") return g.Builder.CreateOr(L, R, "ortmp");
        if (op == "^" || op == "~") return g.Builder.CreateXor(L, R, "xortmp");
        if (op == "<<") return g.Builder.CreateShl(L, R, "shltmp");
        if (op == ">>") return g.Builder.CreateAShr(L, R, "shrtmp");
        if (op == "==") return g.Builder.CreateICmpEQ(L, R, "eqtmp");
        if (op == "!=") return g.Builder.CreateICmpNE(L, R, "netmp");
        if (op == "<") return g.Builder.CreateICmpSLT(L, R, "lttmp");
        if (op == ">") return g.Builder.CreateICmpSGT(L, R, "gttmp");
        if (op == "<=") return g.Builder.CreateICmpSLE(L, R, "letmp");
        if (op == ">=") return g.Builder.CreateICmpSGE(L, R, "getmp");
        
        // Power operator (^)
        auto lf = g.Builder.CreateSIToFP(L, llvm::Type::getDoubleTy(g.Context));
        auto rf = g.Builder.CreateSIToFP(R, llvm::Type::getDoubleTy(g.Context));
        auto res = g.Builder.CreateCall(g.getRuntimeFunction("pow"), {lf, rf});
        return g.Builder.CreateFPToSI(res, L->getType());
    }
    
    if ((isLInt || isLFloat) && (isRInt || isRFloat) && (isLFloat || isRFloat)) {
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
    bool isLVar = false;
    if (L->getType()->isPointerTy()) {
        isLVar = true;
        if (lhs->getType() == ASTNodeType::STRING_LITERAL) isLVar = false;
        else if (lhs->getType() == ASTNodeType::VARIABLE_EXPR) {
            auto name = static_cast<VariableExprAST*>(lhs.get())->name;
            if (g.VariableTypes.count(name) && g.VariableTypes[name] == "string") isLVar = false;
        } else if (lhs->getType() == ASTNodeType::CALL_EXPR) {
            auto callee = static_cast<CallExprAST*>(lhs.get())->callee;
            if (callee == "string" || callee == "read_file" || callee == "input_string" || 
                callee == "time_format" || callee == "json_stringify" || 
                callee == "regex_find" || callee == "regex_replace" || callee == "net_recv" ||
                callee == "os_exec" || callee == "exec" || callee == "os_getenv" ||
                callee == "crypto_sha256" || callee == "sha256") {
                isLVar = false;
            }
        } else if (lhs->getType() == ASTNodeType::SLICE_EXPR) {
            auto slice = static_cast<SliceExprAST*>(lhs.get());
            bool containerIsString = false;
            if (slice->container->getType() == ASTNodeType::STRING_LITERAL) containerIsString = true;
            else if (slice->container->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto name = static_cast<VariableExprAST*>(slice->container.get())->name;
                if (g.VariableTypes.count(name) && g.VariableTypes[name] == "string") containerIsString = true;
            }
            if (containerIsString) isLVar = false;
        }
    }
    
    bool isRVar = false;
    if (R->getType()->isPointerTy()) {
        isRVar = true;
        if (rhs->getType() == ASTNodeType::STRING_LITERAL) isRVar = false;
        else if (rhs->getType() == ASTNodeType::VARIABLE_EXPR) {
            auto name = static_cast<VariableExprAST*>(rhs.get())->name;
            if (g.VariableTypes.count(name) && g.VariableTypes[name] == "string") isRVar = false;
        } else if (rhs->getType() == ASTNodeType::CALL_EXPR) {
            auto callee = static_cast<CallExprAST*>(rhs.get())->callee;
            if (callee == "string" || callee == "read_file" || callee == "input_string" || 
                callee == "time_format" || callee == "json_stringify" || 
                callee == "regex_find" || callee == "regex_replace" || callee == "net_recv" ||
                callee == "os_exec" || callee == "exec" || callee == "os_getenv" ||
                callee == "crypto_sha256" || callee == "sha256") {
                isRVar = false;
            }
        } else if (rhs->getType() == ASTNodeType::SLICE_EXPR) {
            auto slice = static_cast<SliceExprAST*>(rhs.get());
            bool containerIsString = false;
            if (slice->container->getType() == ASTNodeType::STRING_LITERAL) containerIsString = true;
            else if (slice->container->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto name = static_cast<VariableExprAST*>(slice->container.get())->name;
                if (g.VariableTypes.count(name) && g.VariableTypes[name] == "string") containerIsString = true;
            }
            if (containerIsString) isRVar = false;
        }
    }
    
    auto vL = isLVar ? L : g.promoteToVar(L, isLBool ? "bool" : (isLInt ? "int" : (isLFloat ? "float" : "string")));
    auto vR = isRVar ? R : g.promoteToVar(R, isRBool ? "bool" : (isRInt ? "int" : (isRFloat ? "float" : "string")));
    
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
