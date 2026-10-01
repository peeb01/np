#include "llvm_codegen.hpp"
#include <llvm/Support/TargetSelect.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/raw_ostream.h>
#include <llvm/Config/llvm-config.h>
#if LLVM_VERSION_MAJOR >= 17
#include <llvm/TargetParser/Host.h>
#else
#include <llvm/Support/Host.h>
#endif
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <iostream>
#include <optional>

void LLVMCodeGen::optimize() {
    // LLVM optimization passes can be customized here if needed.
}

void LLVMCodeGen::dumpIR() const {
    TheModule.print(llvm::errs(), nullptr);
}

void LLVMCodeGen::writeObjectFile(const std::string& filename) {
    if (llvm::verifyModule(TheModule, &llvm::errs())) {
        std::cerr << "LLVM Verification Error: Module is invalid!" << std::endl;
        exit(1);
    }
    
    std::string TripleStr = llvm::sys::getDefaultTargetTriple();
    llvm::Triple Triple(TripleStr);
    
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();
    
    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TripleStr, Error);
    if (!Target) {
        std::cerr << "LLVM Target Error: " << Error << "\n";
        exit(1);
    }
    
    std::string CPU = "generic";
    std::string Features = "";
    llvm::TargetOptions opt;
#if LLVM_VERSION_MAJOR >= 16
    std::optional<llvm::Reloc::Model> RM = llvm::Reloc::PIC_;
#else
    llvm::Optional<llvm::Reloc::Model> RM = llvm::Reloc::PIC_;
#endif

#ifdef _WIN32
    auto TargetMachine = Target->createTargetMachine(Triple, CPU, Features, opt, RM);
#else
    auto TargetMachine = Target->createTargetMachine(TripleStr, CPU, Features, opt, RM);
#endif
    
    TheModule.setDataLayout(TargetMachine->createDataLayout());
#ifdef _WIN32
    TheModule.setTargetTriple(Triple);
#else
    TheModule.setTargetTriple(TripleStr);
#endif
    
    std::error_code EC;
    llvm::raw_fd_ostream dest(filename, EC, llvm::sys::fs::OF_None);
    if (EC) {
        std::cerr << "Could not open file: " << EC.message() << "\n";
        exit(1);
    }
    
    llvm::legacy::PassManager pass;
#if LLVM_VERSION_MAJOR >= 16
    auto FileType = llvm::CodeGenFileType::ObjectFile;
#else
    auto FileType = llvm::CGFT_ObjectFile;
#endif
    if (TargetMachine->addPassesToEmitFile(pass, dest, nullptr, FileType)) {
        std::cerr << "TargetMachine can't emit a file of this type\n";
        exit(1);
    }
    
    pass.run(TheModule);
    dest.flush();
}
