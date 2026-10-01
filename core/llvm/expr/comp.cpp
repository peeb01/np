#include "llvm_codegen.hpp"

llvm::Value* ListCompExprAST::codegen(LLVMCodeGen& g) {
    auto list = g.Builder.CreateCall(g.getRuntimeFunction("np_rt_var_create_list"), {});
    return list;
}
