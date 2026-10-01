#pragma once

#include <string>
#include <vector>
#include <memory>
#include <utility>

// Forward declare LLVM classes to avoid polluting this header with heavy LLVM headers
namespace llvm {
    class Value;
    class Type;
    class Function;
}

class LLVMCodeGen;

enum class ASTNodeType {
    NUMBER_LITERAL,
    STRING_LITERAL,
    BOOL_LITERAL,
    VARIABLE_EXPR,
    BINARY_EXPR,
    CALL_EXPR,
    LIST_LITERAL,
    DICT_LITERAL,
    INDEX_ACCESS,
    DOT_ACCESS,
    LIST_COMPREHENSION,
    
    VAR_DECL,
    VAR_ASSIGN,
    BLOCK_STMT,
    IF_STMT,
    WHILE_STMT,
    FOR_STMT,
    RETURN_STMT,
    BREAK_STMT,
    CONTINUE_STMT,
    DEFER_STMT,
    GO_STMT,
    FUNC_DECL,
    STRUCT_DECL,
    TRY_EXCEPT_STMT,
    THROW_STMT,
    IMPORT_STMT,
    EXPR_STMT,
    SLICE_EXPR,
    NIL_LITERAL,
    SWITCH_STMT,
    ENUM_DECL,
    ASSERT_STMT,
    INTERFACE_DECL,
    NAMED_ARG_EXPR
};

// Base AST Node
class ASTNode {
public:
    virtual ~ASTNode() = default;
    virtual ASTNodeType getType() const = 0;
    virtual llvm::Value* codegen(LLVMCodeGen& g) = 0;
    virtual void print(int indent = 0) const = 0;
};

// Base Expression Node
class ExprAST : public ASTNode {};

// Base Statement Node
class StmtAST : public ASTNode {};

// Literal Nodes
class NumberExprAST : public ExprAST {
public:
    std::string value;
    bool is_float;
    
    NumberExprAST(const std::string& val, bool is_float) : value(val), is_float(is_float) {}
    ASTNodeType getType() const override { return ASTNodeType::NUMBER_LITERAL; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class StringExprAST : public ExprAST {
public:
    std::string value;
    
    StringExprAST(const std::string& val) : value(val) {}
    ASTNodeType getType() const override { return ASTNodeType::STRING_LITERAL; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class BoolExprAST : public ExprAST {
public:
    bool value;
    
    BoolExprAST(bool val) : value(val) {}
    ASTNodeType getType() const override { return ASTNodeType::BOOL_LITERAL; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class VariableExprAST : public ExprAST {
public:
    std::string name;
    
    VariableExprAST(const std::string& name) : name(name) {}
    ASTNodeType getType() const override { return ASTNodeType::VARIABLE_EXPR; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class BinaryExprAST : public ExprAST {
public:
    std::string op;
    std::unique_ptr<ExprAST> lhs;
    std::unique_ptr<ExprAST> rhs;
    
    BinaryExprAST(const std::string& op, std::unique_ptr<ExprAST> lhs, std::unique_ptr<ExprAST> rhs)
        : op(op), lhs(std::move(lhs)), rhs(std::move(rhs)) {}
    ASTNodeType getType() const override { return ASTNodeType::BINARY_EXPR; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class CallExprAST : public ExprAST {
public:
    std::string callee;
    std::unique_ptr<ExprAST> callee_expr;
    std::vector<std::unique_ptr<ExprAST>> args;
    
    CallExprAST(const std::string& callee, std::vector<std::unique_ptr<ExprAST>> args)
        : callee(callee), callee_expr(nullptr), args(std::move(args)) {}
    CallExprAST(std::unique_ptr<ExprAST> callee_expr, std::vector<std::unique_ptr<ExprAST>> args)
        : callee(""), callee_expr(std::move(callee_expr)), args(std::move(args)) {}
    ASTNodeType getType() const override { return ASTNodeType::CALL_EXPR; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class NamedArgExprAST : public ExprAST {
public:
    std::string name;
    std::unique_ptr<ExprAST> value;
    
    NamedArgExprAST(const std::string& name, std::unique_ptr<ExprAST> value)
        : name(name), value(std::move(value)) {}
    ASTNodeType getType() const override { return ASTNodeType::NAMED_ARG_EXPR; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class NilExprAST : public ExprAST {
public:
    NilExprAST() {}
    ASTNodeType getType() const override { return ASTNodeType::NIL_LITERAL; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class ListExprAST : public ExprAST {
public:
    std::vector<std::unique_ptr<ExprAST>> elements;
    
    ListExprAST(std::vector<std::unique_ptr<ExprAST>> elements) : elements(std::move(elements)) {}
    ASTNodeType getType() const override { return ASTNodeType::LIST_LITERAL; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class DictExprAST : public ExprAST {
public:
    std::vector<std::pair<std::unique_ptr<ExprAST>, std::unique_ptr<ExprAST>>> key_values;
    
    DictExprAST(std::vector<std::pair<std::unique_ptr<ExprAST>, std::unique_ptr<ExprAST>>> key_values)
        : key_values(std::move(key_values)) {}
    ASTNodeType getType() const override { return ASTNodeType::DICT_LITERAL; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class IndexAccessExprAST : public ExprAST {
public:
    std::unique_ptr<ExprAST> container;
    std::unique_ptr<ExprAST> index;
    
    IndexAccessExprAST(std::unique_ptr<ExprAST> container, std::unique_ptr<ExprAST> index)
        : container(std::move(container)), index(std::move(index)) {}
    ASTNodeType getType() const override { return ASTNodeType::INDEX_ACCESS; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class SliceExprAST : public ExprAST {
public:
    std::unique_ptr<ExprAST> container;
    std::unique_ptr<ExprAST> start;
    std::unique_ptr<ExprAST> end;
    
    SliceExprAST(std::unique_ptr<ExprAST> container, std::unique_ptr<ExprAST> start, std::unique_ptr<ExprAST> end)
        : container(std::move(container)), start(std::move(start)), end(std::move(end)) {}
    ASTNodeType getType() const override { return ASTNodeType::SLICE_EXPR; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class DotAccessExprAST : public ExprAST {
public:
    std::unique_ptr<ExprAST> object;
    std::string member;
    
    DotAccessExprAST(std::unique_ptr<ExprAST> object, const std::string& member)
        : object(std::move(object)), member(member) {}
    ASTNodeType getType() const override { return ASTNodeType::DOT_ACCESS; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class ListCompExprAST : public ExprAST {
public:
    std::unique_ptr<ExprAST> expr;
    std::string var_name;
    bool is_range;
    std::unique_ptr<ExprAST> range_start; // Used if is_range = true
    std::unique_ptr<ExprAST> range_end;   // Used if is_range = true
    std::unique_ptr<ExprAST> collection;  // Used if is_range = false
    std::unique_ptr<ExprAST> condition;   // Optional condition filter
    
    ListCompExprAST(std::unique_ptr<ExprAST> expr, const std::string& var_name,
                     std::unique_ptr<ExprAST> start, std::unique_ptr<ExprAST> end,
                     std::unique_ptr<ExprAST> cond)
        : expr(std::move(expr)), var_name(var_name), is_range(true),
          range_start(std::move(start)), range_end(std::move(end)), collection(nullptr), condition(std::move(cond)) {}

    ListCompExprAST(std::unique_ptr<ExprAST> expr, const std::string& var_name,
                     std::unique_ptr<ExprAST> coll, std::unique_ptr<ExprAST> cond)
        : expr(std::move(expr)), var_name(var_name), is_range(false),
          range_start(nullptr), range_end(nullptr), collection(std::move(coll)), condition(std::move(cond)) {}
          
    ASTNodeType getType() const override { return ASTNodeType::LIST_COMPREHENSION; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

struct VarDeclItem {
    std::string type_name;
    std::string var_name;
    std::unique_ptr<ExprAST> size_expr;

    VarDeclItem(std::string type_name, std::string var_name, std::unique_ptr<ExprAST> size_expr = nullptr)
        : type_name(std::move(type_name)), var_name(std::move(var_name)), size_expr(std::move(size_expr)) {}

    VarDeclItem(VarDeclItem&& o) noexcept
        : type_name(std::move(o.type_name)), var_name(std::move(o.var_name)), size_expr(std::move(o.size_expr)) {}

    VarDeclItem& operator=(VarDeclItem&& o) noexcept {
        type_name = std::move(o.type_name);
        var_name = std::move(o.var_name);
        size_expr = std::move(o.size_expr);
        return *this;
    }
};

// Statement Nodes
class VarDeclStmtAST : public StmtAST {
public:
    std::vector<VarDeclItem> vars;
    std::unique_ptr<ExprAST> initializer;
    
    VarDeclStmtAST(std::vector<VarDeclItem> vars, std::unique_ptr<ExprAST> init)
        : vars(std::move(vars)), initializer(std::move(init)) {}
    ASTNodeType getType() const override { return ASTNodeType::VAR_DECL; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class VarAssignStmtAST : public StmtAST {
public:
    std::unique_ptr<ExprAST> lvalue; // Usually VariableExprAST or IndexAccessExprAST
    std::unique_ptr<ExprAST> rvalue;
    
    VarAssignStmtAST(std::unique_ptr<ExprAST> lvalue, std::unique_ptr<ExprAST> rvalue)
        : lvalue(std::move(lvalue)), rvalue(std::move(rvalue)) {}
    ASTNodeType getType() const override { return ASTNodeType::VAR_ASSIGN; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class BlockStmtAST : public StmtAST {
public:
    std::vector<std::unique_ptr<ASTNode>> statements;
    
    BlockStmtAST(std::vector<std::unique_ptr<ASTNode>> stmts) : statements(std::move(stmts)) {}
    ASTNodeType getType() const override { return ASTNodeType::BLOCK_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

struct IfCondBlock {
    std::unique_ptr<ExprAST> cond;
    std::unique_ptr<BlockStmtAST> block;
};

class IfStmtAST : public StmtAST {
public:
    std::vector<IfCondBlock> cases; // cases[0] is the main "if", cases[1..] are "elif"s
    std::unique_ptr<BlockStmtAST> else_block;
    
    IfStmtAST(std::vector<IfCondBlock> cases, std::unique_ptr<BlockStmtAST> else_block)
        : cases(std::move(cases)), else_block(std::move(else_block)) {}
    ASTNodeType getType() const override { return ASTNodeType::IF_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class WhileStmtAST : public StmtAST {
public:
    std::unique_ptr<ExprAST> cond;
    std::unique_ptr<BlockStmtAST> body;
    
    WhileStmtAST(std::unique_ptr<ExprAST> cond, std::unique_ptr<BlockStmtAST> body)
        : cond(std::move(cond)), body(std::move(body)) {}
    ASTNodeType getType() const override { return ASTNodeType::WHILE_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class ForStmtAST : public StmtAST {
public:
    std::string var_name;
    bool is_range;
    std::unique_ptr<ExprAST> range_start;
    std::unique_ptr<ExprAST> range_end;
    std::unique_ptr<ExprAST> collection;
    std::unique_ptr<BlockStmtAST> body;
    
    ForStmtAST(const std::string& var_name, std::unique_ptr<ExprAST> start, std::unique_ptr<ExprAST> end, std::unique_ptr<BlockStmtAST> body)
        : var_name(var_name), is_range(true), range_start(std::move(start)), range_end(std::move(end)), collection(nullptr), body(std::move(body)) {}
        
    ForStmtAST(const std::string& var_name, std::unique_ptr<ExprAST> coll, std::unique_ptr<BlockStmtAST> body)
        : var_name(var_name), is_range(false), range_start(nullptr), range_end(nullptr), collection(std::move(coll)), body(std::move(body)) {}
        
    ASTNodeType getType() const override { return ASTNodeType::FOR_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class ReturnStmtAST : public StmtAST {
public:
    std::unique_ptr<ExprAST> expr; // nullptr if void return
    
    ReturnStmtAST(std::unique_ptr<ExprAST> expr) : expr(std::move(expr)) {}
    ASTNodeType getType() const override { return ASTNodeType::RETURN_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class BreakStmtAST : public StmtAST {
public:
    BreakStmtAST() {}
    ASTNodeType getType() const override { return ASTNodeType::BREAK_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class ContinueStmtAST : public StmtAST {
public:
    ContinueStmtAST() {}
    ASTNodeType getType() const override { return ASTNodeType::CONTINUE_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class DeferStmtAST : public StmtAST {
public:
    std::unique_ptr<StmtAST> stmt;
    DeferStmtAST(std::unique_ptr<StmtAST> stmt) : stmt(std::move(stmt)) {}
    ASTNodeType getType() const override { return ASTNodeType::DEFER_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class GoStmtAST : public StmtAST {
public:
    std::unique_ptr<ExprAST> call;
    GoStmtAST(std::unique_ptr<ExprAST> call) : call(std::move(call)) {}
    ASTNodeType getType() const override { return ASTNodeType::GO_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

struct SwitchCase {
    std::unique_ptr<ExprAST> val;
    std::unique_ptr<BlockStmtAST> body;
};

class SwitchStmtAST : public StmtAST {
public:
    std::unique_ptr<ExprAST> cond;
    std::vector<SwitchCase> cases;
    std::unique_ptr<BlockStmtAST> default_block;
    
    SwitchStmtAST(std::unique_ptr<ExprAST> cond, std::vector<SwitchCase> cases, std::unique_ptr<BlockStmtAST> def_block)
        : cond(std::move(cond)), cases(std::move(cases)), default_block(std::move(def_block)) {}
    ASTNodeType getType() const override { return ASTNodeType::SWITCH_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class EnumDeclStmtAST : public StmtAST {
public:
    std::string name;
    std::vector<std::pair<std::string, int>> members;
    
    EnumDeclStmtAST(const std::string& name, std::vector<std::pair<std::string, int>> members)
        : name(name), members(std::move(members)) {}
    ASTNodeType getType() const override { return ASTNodeType::ENUM_DECL; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

struct FuncParam {
    std::string type;
    std::string name;
};

class FuncDeclStmtAST : public StmtAST {
public:
    std::string name;
    std::vector<FuncParam> params;
    std::string return_type;
    std::unique_ptr<BlockStmtAST> body;
    bool is_kernel;
    
    FuncDeclStmtAST(const std::string& name, std::vector<FuncParam> params, const std::string& ret_type, std::unique_ptr<BlockStmtAST> body, bool is_kernel = false)
        : name(name), params(std::move(params)), return_type(ret_type), body(std::move(body)), is_kernel(is_kernel) {}
    ASTNodeType getType() const override { return ASTNodeType::FUNC_DECL; }
    llvm::Function* declarePrototype(LLVMCodeGen& g);
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

struct StructField {
    std::string type;
    std::string name;
};

class StructDeclStmtAST : public StmtAST {
public:
    std::string name;
    std::vector<StructField> fields;
    
    StructDeclStmtAST(const std::string& name, std::vector<StructField> fields)
        : name(name), fields(std::move(fields)) {}
    ASTNodeType getType() const override { return ASTNodeType::STRUCT_DECL; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class TryExceptStmtAST : public StmtAST {
public:
    std::unique_ptr<BlockStmtAST> try_block;
    std::string exception_var;
    std::unique_ptr<BlockStmtAST> except_block;
    
    TryExceptStmtAST(std::unique_ptr<BlockStmtAST> try_b, const std::string& exc_v, std::unique_ptr<BlockStmtAST> except_b)
        : try_block(std::move(try_b)), exception_var(exc_v), except_block(std::move(except_b)) {}
    ASTNodeType getType() const override { return ASTNodeType::TRY_EXCEPT_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class ThrowStmtAST : public StmtAST {
public:
    std::unique_ptr<ExprAST> expr;
    
    ThrowStmtAST(std::unique_ptr<ExprAST> expr) : expr(std::move(expr)) {}
    ASTNodeType getType() const override { return ASTNodeType::THROW_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class ImportStmtAST : public StmtAST {
public:
    std::string module_name;
    
    ImportStmtAST(const std::string& module_name) : module_name(module_name) {}
    ASTNodeType getType() const override { return ASTNodeType::IMPORT_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class ExprStmtAST : public StmtAST {
public:
    std::unique_ptr<ExprAST> expr;
    
    ExprStmtAST(std::unique_ptr<ExprAST> expr) : expr(std::move(expr)) {}
    ASTNodeType getType() const override { return ASTNodeType::EXPR_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class AssertStmtAST : public StmtAST {
public:
    std::unique_ptr<ExprAST> cond;
    std::unique_ptr<ExprAST> message;
    int line;
    
    AssertStmtAST(std::unique_ptr<ExprAST> cond, std::unique_ptr<ExprAST> message, int line)
        : cond(std::move(cond)), message(std::move(message)), line(line) {}
    ASTNodeType getType() const override { return ASTNodeType::ASSERT_STMT; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};

class InterfaceDeclStmtAST : public StmtAST {
public:
    std::string name;
    std::vector<std::string> methods;
    
    InterfaceDeclStmtAST(const std::string& name, std::vector<std::string> methods)
        : name(name), methods(std::move(methods)) {}
    ASTNodeType getType() const override { return ASTNodeType::INTERFACE_DECL; }
    llvm::Value* codegen(LLVMCodeGen& g) override;
    void print(int indent) const override;
};
