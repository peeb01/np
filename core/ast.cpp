#include "../include/ast.hpp"
#include <iostream>

static void printIndent(int indent) {
    for (int i = 0; i < indent; ++i) std::cout << "  ";
}

void NumberExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[NumberExpr] " << value << " (is_float: " << (is_float ? "true" : "false") << ")\n";
}

void StringExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[StringExpr] \"" << value << "\"\n";
}

void BoolExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[BoolExpr] " << (value ? "true" : "false") << "\n";
}

void VariableExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[VariableExpr] " << name << "\n";
}

void BinaryExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[BinaryExpr] op: " << op << "\n";
    lhs->print(indent + 1);
    rhs->print(indent + 1);
}

void CallExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[CallExpr] callee: " << callee << "\n";
    for (const auto& arg : args) {
        arg->print(indent + 1);
    }
}

void ListExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[ListExpr]\n";
    for (const auto& el : elements) {
        el->print(indent + 1);
    }
}

void DictExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[DictExpr]\n";
    for (const auto& [k, v] : key_values) {
        printIndent(indent + 1);
        std::cout << "Key:\n";
        k->print(indent + 2);
        printIndent(indent + 1);
        std::cout << "Value:\n";
        v->print(indent + 2);
    }
}

void IndexAccessExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[IndexAccessExpr]\n";
    container->print(indent + 1);
    index->print(indent + 1);
}

void SliceExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[SliceExpr]\n";
    container->print(indent + 1);
    if (start) {
        printIndent(indent + 1);
        std::cout << "Start:\n";
        start->print(indent + 2);
    }
    if (end) {
        printIndent(indent + 1);
        std::cout << "End:\n";
        end->print(indent + 2);
    }
}

void DotAccessExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[DotAccessExpr] member: " << member << "\n";
    object->print(indent + 1);
}

void ListCompExprAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[ListCompExpr] item: " << var_name << "\n";
    printIndent(indent + 1);
    std::cout << "Expression:\n";
    expr->print(indent + 2);
    if (is_range) {
        printIndent(indent + 1);
        std::cout << "Range Start:\n";
        range_start->print(indent + 2);
        printIndent(indent + 1);
        std::cout << "Range End:\n";
        range_end->print(indent + 2);
    } else {
        printIndent(indent + 1);
        std::cout << "Collection:\n";
        collection->print(indent + 2);
    }
    if (condition) {
        printIndent(indent + 1);
        std::cout << "Condition:\n";
        condition->print(indent + 2);
    }
}

void VarDeclStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[VarDeclStmt] variables:\n";
    for (const auto& v : vars) {
        printIndent(indent + 1);
        std::cout << v.type_name << " " << v.var_name << "\n";
    }
    if (initializer) {
        printIndent(indent + 1);
        std::cout << "Initializer:\n";
        initializer->print(indent + 2);
    }
}

void VarAssignStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[VarAssignStmt]\n";
    lvalue->print(indent + 1);
    rvalue->print(indent + 1);
}

void BlockStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[BlockStmt] (statements: " << statements.size() << ")\n";
    for (const auto& stmt : statements) {
        stmt->print(indent + 1);
    }
}

void IfStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[IfStmt]\n";
    for (size_t i = 0; i < cases.size(); ++i) {
        printIndent(indent + 1);
        std::cout << (i == 0 ? "If Case:\n" : "Elif Case:\n");
        if (cases[i].cond) cases[i].cond->print(indent + 2);
        cases[i].block->print(indent + 2);
    }
    if (else_block) {
        printIndent(indent + 1);
        std::cout << "Else Block:\n";
        else_block->print(indent + 2);
    }
}

void WhileStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[WhileStmt]\n";
    cond->print(indent + 1);
    body->print(indent + 1);
}

void ForStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[ForStmt] loop_var: " << var_name << "\n";
    if (is_range) {
        printIndent(indent + 1);
        std::cout << "Start:\n";
        range_start->print(indent + 2);
        printIndent(indent + 1);
        std::cout << "End:\n";
        range_end->print(indent + 2);
    } else {
        printIndent(indent + 1);
        std::cout << "Collection:\n";
        collection->print(indent + 2);
    }
    body->print(indent + 1);
}

void ReturnStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[ReturnStmt]\n";
    if (expr) {
        expr->print(indent + 1);
    } else {
        printIndent(indent + 1);
        std::cout << "void\n";
    }
}

void FuncDeclStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[FuncDeclStmt] name: " << name << ", return_type: " << return_type << "\n";
    printIndent(indent + 1);
    std::cout << "Parameters:\n";
    for (const auto& p : params) {
        printIndent(indent + 2);
        std::cout << p.type << " " << p.name << "\n";
    }
    printIndent(indent + 1);
    std::cout << "Body:\n";
    body->print(indent + 2);
}

void StructDeclStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[StructDeclStmt] name: " << name << "\n";
    for (const auto& f : fields) {
        printIndent(indent + 1);
        std::cout << f.type << " " << f.name << "\n";
    }
}

void TryExceptStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[TryExceptStmt]\n";
    printIndent(indent + 1);
    std::cout << "Try:\n";
    try_block->print(indent + 2);
    printIndent(indent + 1);
    std::cout << "Except (var: " << exception_var << "):\n";
    except_block->print(indent + 2);
}

void ThrowStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[ThrowStmt]\n";
    expr->print(indent + 1);
}

void ImportStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[ImportStmt] module: " << module_name << "\n";
}

void ExprStmtAST::print(int indent) const {
    printIndent(indent);
    std::cout << "[ExprStmt]\n";
    expr->print(indent + 1);
}
