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
#include <llvm/TargetParser/SubtargetFeature.h>
#else
#include <llvm/Support/Host.h>
#include <llvm/MC/SubtargetFeature.h>
#endif
#include <llvm/Support/CodeGen.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Verifier.h>
#include <llvm/Passes/PassBuilder.h>
#include <llvm/Passes/StandardInstrumentations.h>
#include <llvm/IR/PassManager.h>
#include <llvm/Analysis/LoopAnalysisManager.h>
#include <llvm/Analysis/CGSCCPassManager.h>
#include <iostream>
#include <optional>
#include <memory>

static std::unique_ptr<llvm::TargetMachine> createHostTargetMachine(int opt_level) {
    llvm::InitializeAllTargetInfos();
    llvm::InitializeAllTargets();
    llvm::InitializeAllTargetMCs();
    llvm::InitializeAllAsmParsers();
    llvm::InitializeAllAsmPrinters();

    std::string TripleStr = llvm::sys::getDefaultTargetTriple();
    std::string Error;
    auto Target = llvm::TargetRegistry::lookupTarget(TripleStr, Error);
    if (!Target) {
        std::cerr << "LLVM Target Error: " << Error << "\n";
        return nullptr;
    }

    std::string CPU = llvm::sys::getHostCPUName().str();
    if (CPU.empty()) CPU = "generic";

    llvm::SubtargetFeatures SubFeatures;
    llvm::StringMap<bool> FeatureMap;
    if (llvm::sys::getHostCPUFeatures(FeatureMap)) {
        for (auto &F : FeatureMap) {
            SubFeatures.AddFeature(F.first(), F.second);
        }
    }
    std::string Features = SubFeatures.getString();

    llvm::TargetOptions opt;
#if LLVM_VERSION_MAJOR >= 16
    std::optional<llvm::Reloc::Model> RM = llvm::Reloc::PIC_;
#else
    llvm::Optional<llvm::Reloc::Model> RM = llvm::Reloc::PIC_;
#endif

#if LLVM_VERSION_MAJOR >= 16
    llvm::CodeGenOptLevel cgo = llvm::CodeGenOptLevel::Default;
    if (opt_level == 0) cgo = llvm::CodeGenOptLevel::None;
    else if (opt_level == 1) cgo = llvm::CodeGenOptLevel::Less;
    else if (opt_level == 2) cgo = llvm::CodeGenOptLevel::Default;
    else if (opt_level >= 3) cgo = llvm::CodeGenOptLevel::Aggressive;
#else
    llvm::CodeGenOpt::Level cgo = llvm::CodeGenOpt::Default;
    if (opt_level == 0) cgo = llvm::CodeGenOpt::None;
    else if (opt_level == 1) cgo = llvm::CodeGenOpt::Less;
    else if (opt_level == 2) cgo = llvm::CodeGenOpt::Default;
    else if (opt_level >= 3) cgo = llvm::CodeGenOpt::Aggressive;
#endif

#ifdef _WIN32
    llvm::Triple Triple(TripleStr);
    return std::unique_ptr<llvm::TargetMachine>(Target->createTargetMachine(Triple, CPU, Features, opt, RM, std::nullopt, cgo));
#else
    return std::unique_ptr<llvm::TargetMachine>(Target->createTargetMachine(TripleStr, CPU, Features, opt, RM, std::nullopt, cgo));
#endif
}

void LLVMCodeGen::optimize(int opt_level) {
    if (opt_level <= 0) {
        return;
    }

    auto TM = createHostTargetMachine(opt_level);
    if (TM) {
        TheModule.setDataLayout(TM->createDataLayout());
        TheModule.setTargetTriple(TM->getTargetTriple().str());
    }

    llvm::LoopAnalysisManager LAM;
    llvm::FunctionAnalysisManager FAM;
    llvm::CGSCCAnalysisManager CGAM;
    llvm::ModuleAnalysisManager MAM;

    llvm::PassInstrumentationCallbacks PIC;
    llvm::StandardInstrumentations SI(Context, false);
    SI.registerCallbacks(PIC, &MAM);

    llvm::PassBuilder PB(TM.get(), llvm::PipelineTuningOptions(), std::nullopt, &PIC);

    PB.registerModuleAnalyses(MAM);
    PB.registerCGSCCAnalyses(CGAM);
    PB.registerFunctionAnalyses(FAM);
    PB.registerLoopAnalyses(LAM);
    PB.crossRegisterProxies(LAM, FAM, CGAM, MAM);

    llvm::OptimizationLevel Level;
    if (opt_level == 1) Level = llvm::OptimizationLevel::O1;
    else if (opt_level == 2) Level = llvm::OptimizationLevel::O2;
    else Level = llvm::OptimizationLevel::O3;

    llvm::ModulePassManager MPM = PB.buildPerModuleDefaultPipeline(Level);
    MPM.run(TheModule, MAM);
}

void LLVMCodeGen::dumpIR() const {
    TheModule.print(llvm::errs(), nullptr);
}

void LLVMCodeGen::writeObjectFile(const std::string& filename) {
    if (llvm::verifyModule(TheModule, &llvm::errs())) {
        std::cerr << "LLVM Verification Error: Module is invalid!" << std::endl;
        exit(1);
    }
    
    auto TargetMachine = createHostTargetMachine(3);
    if (!TargetMachine) {
        std::cerr << "LLVM Target Error: Failed to create TargetMachine\n";
        exit(1);
    }
    
    TheModule.setDataLayout(TargetMachine->createDataLayout());
    TheModule.setTargetTriple(TargetMachine->getTargetTriple().str());
    
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
