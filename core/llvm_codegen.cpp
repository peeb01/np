#include "../include/llvm_codegen.hpp"
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/TargetParser/Host.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/Transforms/InstCombine/InstCombine.h>
#include <llvm/Transforms/Scalar.h>
#include <llvm/Transforms/Scalar/GVN.h>
#include <iostream>
#include <optional>
#include <llvm/IR/Verifier.h>

LLVMCodeGen::LLVMCodeGen() 
    : TheModule("np_module", Context), Builder(Context) {
    declareRuntime();
}

void LLVMCodeGen::declareRuntime() {
    auto i8PtrTy = llvm::PointerType::getUnqual(Context);
    auto i64Ty = llvm::Type::getInt64Ty(Context);
    auto doubleTy = llvm::Type::getDoubleTy(Context);
    auto i1Ty = llvm::Type::getInt1Ty(Context);
    auto voidTy = llvm::Type::getVoidTy(Context);
    
    // Helper to declare extern "C" functions
    auto declareFunc = [&](const std::string& name, llvm::Type* retTy, const std::vector<llvm::Type*>& argTys) {
        auto ft = llvm::FunctionType::get(retTy, argTys, false);
        auto f = llvm::Function::Create(ft, llvm::Function::ExternalLinkage, name, TheModule);
        RuntimeFunctions[name] = f;
    };
    
    // String API
    declareFunc("np_rt_string_create", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_string_destroy", voidTy, {i8PtrTy});
    declareFunc("np_rt_string_concat", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_string_concat_char", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_string_concat_char_lhs", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_string_c_str", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_string_len", i64Ty, {i8PtrTy});
    declareFunc("np_rt_string_slice", i8PtrTy, {i8PtrTy, i64Ty, i64Ty});
    declareFunc("np_rt_string_eq", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_string_lt", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_string_contains", i1Ty, {i8PtrTy, i8PtrTy});
    
    // np_var API
    declareFunc("np_rt_var_create_int", i8PtrTy, {i64Ty});
    declareFunc("np_rt_var_create_float", i8PtrTy, {doubleTy});
    declareFunc("np_rt_var_create_string", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_var_create_bool", i8PtrTy, {i1Ty});
    declareFunc("np_rt_var_create_list", i8PtrTy, {});
    declareFunc("np_rt_var_create_dict", i8PtrTy, {});
    declareFunc("np_rt_var_create_int128", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_var_create_int256", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_var_destroy", voidTy, {i8PtrTy});
    
    declareFunc("np_rt_var_append", voidTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_pop", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_var_get_index", i8PtrTy, {i8PtrTy, i64Ty});
    declareFunc("np_rt_var_set_index", voidTy, {i8PtrTy, i64Ty, i8PtrTy});
    declareFunc("np_rt_var_get_key", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_set_key", voidTy, {i8PtrTy, i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_len", i64Ty, {i8PtrTy});
    declareFunc("np_rt_var_slice", i8PtrTy, {i8PtrTy, i64Ty, i64Ty});
    
    // np_var Operators
    declareFunc("np_rt_var_add", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_sub", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_mul", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_div", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_mod", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_pow", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_gt", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_lt", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_ge", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_le", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_eq", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_var_ne", i1Ty, {i8PtrTy, i8PtrTy});
    
    // Printing API
    declareFunc("np_rt_print_int", voidTy, {i64Ty});
    declareFunc("np_rt_print_float", voidTy, {doubleTy});
    declareFunc("np_rt_print_bool", voidTy, {i1Ty});
    declareFunc("np_rt_print_string", voidTy, {i8PtrTy});
    declareFunc("np_rt_print_var", voidTy, {i8PtrTy});
    
    // Conversion API
    declareFunc("np_rt_to_int_string", i64Ty, {i8PtrTy});
    declareFunc("np_rt_to_int_var", i64Ty, {i8PtrTy});
    declareFunc("np_rt_to_float_string", doubleTy, {i8PtrTy});
    declareFunc("np_rt_to_float_var", doubleTy, {i8PtrTy});
    declareFunc("np_rt_to_string_int", i8PtrTy, {i64Ty});
    declareFunc("np_rt_to_string_float", i8PtrTy, {doubleTy});
    declareFunc("np_rt_to_string_var", i8PtrTy, {i8PtrTy});
    
    // Input API
    declareFunc("np_rt_input_int", i64Ty, {});
    declareFunc("np_rt_input_float", doubleTy, {});
    declareFunc("np_rt_input_string", i8PtrTy, {});
    
    // Modules
    declareFunc("np_rt_sys_init_args", voidTy, {llvm::Type::getInt32Ty(Context), i8PtrTy});
    declareFunc("np_rt_sys_get_argv", i8PtrTy, {});
    declareFunc("np_rt_time_now", doubleTy, {});
    declareFunc("np_rt_time_sleep", voidTy, {doubleTy});
    declareFunc("np_rt_time_format", i8PtrTy, {doubleTy, i8PtrTy});
    declareFunc("np_rt_json_stringify", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_json_parse", i8PtrTy, {i8PtrTy});
    declareFunc("np_rt_regex_match", i1Ty, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_regex_find", i8PtrTy, {i8PtrTy, i8PtrTy});
    declareFunc("np_rt_regex_replace", i8PtrTy, {i8PtrTy, i8PtrTy, i8PtrTy});
    declareFunc("np_rt_type_var", i8PtrTy, {i8PtrTy});

    // Networking Socket API
    declareFunc("np_rt_net_listen", i64Ty, {i64Ty});
    declareFunc("np_rt_net_accept", i64Ty, {i64Ty});
    declareFunc("np_rt_net_connect", i64Ty, {i8PtrTy, i64Ty});
    declareFunc("np_rt_net_send", i64Ty, {i64Ty, i8PtrTy});
    declareFunc("np_rt_net_recv", i8PtrTy, {i64Ty, i64Ty});
    declareFunc("np_rt_net_close", voidTy, {i64Ty});
}

llvm::Type* LLVMCodeGen::getLLVMType(const std::string& np_type) {
    if (np_type == "void" || np_type == "") {
        return llvm::Type::getVoidTy(Context);
    }
    if (np_type == "int" || np_type == "int32" || np_type == "int64") {
        return llvm::Type::getInt64Ty(Context);
    }
    if (np_type == "float" || np_type == "float32" || np_type == "float64") {
        return llvm::Type::getDoubleTy(Context);
    }
    if (np_type == "bool") {
        return llvm::Type::getInt1Ty(Context);
    }
    // String, array, dict, and structs map to pointers in our C runtime
    return llvm::PointerType::getUnqual(Context);
}

llvm::Function* LLVMCodeGen::getRuntimeFunction(const std::string& name) {
    return RuntimeFunctions[name];
}

llvm::Value* LLVMCodeGen::promoteToVar(llvm::Value* val, const std::string& type) {
    if (type == "int" || type == "int32" || type == "int64") {
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

void LLVMCodeGen::compile(const std::vector<std::unique_ptr<ASTNode>>& ast) {
    // 1. Declare and setup the main function
    auto i32Ty = llvm::Type::getInt32Ty(Context);
    auto i8PtrPtrTy = llvm::PointerType::getUnqual(Context); // char**
    auto mainFuncType = llvm::FunctionType::get(i32Ty, {i32Ty, i8PtrPtrTy}, false);
    auto mainFunc = llvm::Function::Create(mainFuncType, llvm::Function::ExternalLinkage, "main", TheModule);
    
    auto mainBB = llvm::BasicBlock::Create(Context, "entry", mainFunc);
    Builder.SetInsertPoint(mainBB);
    
    // Call np_rt_sys_init_args(argc, argv)
    auto mainArgs = mainFunc->arg_begin();
    llvm::Value* argcVal = &(*mainArgs);
    llvm::Value* argvVal = &(*++mainArgs);
    Builder.CreateCall(getRuntimeFunction("np_rt_sys_init_args"), {argcVal, argvVal});
    
    // 2. Generate code for all AST nodes
    for (const auto& node : ast) {
        if (node->getType() == ASTNodeType::FUNC_DECL) {
            // Save insert point
            auto savedBB = Builder.GetInsertBlock();
            node->codegen(*this);
            // Restore insert point
            Builder.SetInsertPoint(savedBB);
        } else if (node->getType() == ASTNodeType::STRUCT_DECL) {
            node->codegen(*this);
        } else {
            // Regular global statement compiled inside main
            node->codegen(*this);
        }
    }
    
    // 3. Close main function with return 0
    if (!Builder.GetInsertBlock()->getTerminator()) {
        Builder.CreateRet(llvm::ConstantInt::get(Context, llvm::APInt(32, 0, true)));
    }
}

void LLVMCodeGen::optimize() {
    // LLVM 18 optimization passes can be implemented with the new PassBuilder if needed.
    // Legacy PassManager is bypassed here to prevent double-free crashes.
}

void LLVMCodeGen::dumpIR() const {
    TheModule.print(llvm::errs(), nullptr);
}

void LLVMCodeGen::writeObjectFile(const std::string& filename) {
    if (llvm::verifyModule(TheModule, &llvm::errs())) {
        std::cerr << "LLVM Verification Error: Module is invalid!" << std::endl;
        exit(1);
    }
    
    std::string Triple = llvm::sys::getDefaultTargetTriple();
    
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    
    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(Triple, Error);
    if (!Target) {
        std::cerr << "LLVM Target Error: " << Error << "\n";
        exit(1);
    }
    
    std::string CPU = "generic";
    std::string Features = "";
    llvm::TargetOptions opt;
    std::optional<llvm::Reloc::Model> RM = llvm::Reloc::PIC_;
    auto TargetMachine = Target->createTargetMachine(Triple, CPU, Features, opt, RM);
    
    TheModule.setDataLayout(TargetMachine->createDataLayout());
    TheModule.setTargetTriple(Triple);
    
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << "\n";
        exit(1);
    }
    
    llvm::legacy::PassManager pass;
    auto FileType = llvm::CodeGenFileType::ObjectFile;
    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        std::cerr << "TargetMachine can't emit a file of this type\n";
        exit(1);
    }
    
    pass.run(TheModule);
    dest.flush();
}

// Note: AST Node codegen implementations have been split into core/codegen_expr.cpp and core/codegen_stmt.cpp for maintainability.
