#pragma once

#include "ast.hpp"
#include <string>
#include <vector>
#include <memory>
#include <unordered_map>

namespace llvm {
    class LLVMContext;
    class Module;
    class Function;
    class Value;
    class Type;
}

#include <llvm/IR/IRBuilder.h>

class GPUCodeGen {
public:
    static GPUCodeGen& instance();

    GPUCodeGen();
    ~GPUCodeGen();

    void reset();
    bool hasKernels() const { return !kernel_names.empty(); }
    const std::vector<std::string>& getKernelNames() const { return kernel_names; }

    // Compile a kernel function AST node into the device LLVM Module
    void compileKernel(FuncDeclStmtAST* funcAst);

    // Compile the entire device module to PTX assembly string
    std::string compileToPTX(const std::string& sm_arch = "sm_50");

private:
    std::unique_ptr<llvm::LLVMContext> ctx;
    std::unique_ptr<llvm::Module> mod;
    std::unique_ptr<llvm::IRBuilder<>> builder;
    std::vector<std::string> kernel_names;

    // Helper functions for kernel code generation
    llvm::Type* getGpuType(const std::string& type_name, bool is_param_pointer);
    llvm::Value* codegenExpr(ExprAST* expr, llvm::Function* kernelFunc,
                             std::unordered_map<std::string, llvm::Value*>& namedValues,
                             std::unordered_map<std::string, llvm::Type*>& valueTypes);
    void codegenStmt(ASTNode* node, llvm::Function* kernelFunc,
                     std::unordered_map<std::string, llvm::Value*>& namedValues,
                     std::unordered_map<std::string, llvm::Type*>& valueTypes);
};
