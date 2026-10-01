#include "llvm_codegen.hpp"

llvm::Type* LLVMCodeGen::getLLVMType(const std::string& np_type) {
    if (np_type == "void" || np_type == "") {
        return llvm::Type::getVoidTy(Context);
    }
    if (np_type == "uint8" || np_type == "byte" || np_type == "int8") {
        return llvm::Type::getInt8Ty(Context);
    }
    if (np_type == "uint16" || np_type == "int16") {
        return llvm::Type::getInt16Ty(Context);
    }
    if (np_type == "uint32" || np_type == "int32") {
        return llvm::Type::getInt32Ty(Context);
    }
    if (np_type == "int" || np_type == "int64" || np_type == "uint64" || np_type == "uint") {
        return llvm::Type::getInt64Ty(Context);
    }
    if (np_type == "uint128") {
        return llvm::Type::getInt128Ty(Context);
    }
    if (np_type == "float" || np_type == "float32" || np_type == "float64") {
        return llvm::Type::getDoubleTy(Context);
    }
    if (np_type == "bool") {
        return llvm::Type::getInt1Ty(Context);
    }
    // String, array, dict, function pointers, and structs map to pointers
    return llvm::PointerType::get(llvm::Type::getInt8Ty(Context), 0);
}

llvm::Function* LLVMCodeGen::getRuntimeFunction(const std::string& name) {
    return RuntimeFunctions[name];
}

llvm::Value* LLVMCodeGen::promoteToVar(llvm::Value* val, const std::string& type) {
    if (type == "int" || type == "int32" || type == "int64" ||
        type == "uint8" || type == "byte" || type == "int8" ||
        type == "uint16" || type == "int16" || type == "uint32" ||
        type == "uint64" || type == "uint") {
        if (val->getType()->isIntegerTy() && !val->getType()->isIntegerTy(64)) {
            val = Builder.CreateZExtOrTrunc(val, llvm::Type::getInt64Ty(Context));
        }
        return Builder.CreateCall(getRuntimeFunction("np_rt_var_create_int"), {val});
    }
    if (type == "float" || type == "float32" || type == "float64") {
        return Builder.CreateCall(getRuntimeFunction("np_rt_var_create_float"), {val});
    }
    if (type == "bool") {
        return Builder.CreateCall(getRuntimeFunction("np_rt_var_create_bool"), {val});
    }
    if (type == "string") {
        return Builder.CreateCall(getRuntimeFunction("np_rt_var_create_string"), {val});
    }
    return val; // Already a var/struct
}
