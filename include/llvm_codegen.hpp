#pragma once

#include "ast.hpp"
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Value.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/Instructions.h>
#include <unordered_map>
#include <string>
#include <vector>
#include <memory>

class LLVMCodeGen {
public:
    llvm::LLVMContext Context;
    llvm::Module TheModule;
    llvm::IRBuilder<> Builder;
    std::unordered_map<std::string, llvm::AllocaInst*> NamedValues;
    std::unordered_map<std::string, std::string> VariableTypes; // NP type name
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> Structs;
    
    // Cache for declared runtime functions
    std::unordered_map<std::string, llvm::Function*> RuntimeFunctions;
    
    LLVMCodeGen();
    void compile(const std::vector<std::unique_ptr<ASTNode>>& ast);
    void writeObjectFile(const std::string& filename);
    void dumpIR() const;
    void optimize();
    
    llvm::Type* getLLVMType(const std::string& np_type);
    llvm::Function* getRuntimeFunction(const std::string& name);
    llvm::Value* promoteToVar(llvm::Value* val, const std::string& type);

private:
    void declareRuntime();
};
