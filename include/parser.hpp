#pragma once
#include "common.hpp"
#include "ast.hpp"
#include <vector>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <memory>

class Parser {
public:
    Parser(const std::vector<Token>& tokens, const std::string& namespace_prefix = "");
    void parse();
    const std::vector<std::unique_ptr<ASTNode>>& getAST() const { return ast_root; }

    std::unordered_map<std::string, std::string> variables;
    std::unordered_map<std::string, std::vector<std::pair<std::string, std::string>>> structs;

private:
    std::vector<Token> tokens;
    size_t pos;
    std::vector<std::unique_ptr<ASTNode>> ast_root;
    std::unordered_set<std::string> imported_modules;
    std::string namespace_prefix;
    bool is_inside_function;
    std::unordered_set<std::string> local_variables;
    std::unordered_set<std::string> import_aliases;

    bool isBuiltIn(const std::string& name) const;

    Token peek(int offset = 0) const;
    Token advance();
    bool check(TokenType type) const;
    void expect(TokenType type, const std::string& err_msg);
    bool isAtEnd() const;

    std::unique_ptr<BlockStmtAST> parseBlock();
    std::unique_ptr<StmtAST> parseStatement();
    std::unique_ptr<ExprAST> parseExpression();
    std::unique_ptr<ExprAST> parseExpression(int min_precedence);
    std::unique_ptr<ExprAST> parsePrimary();
    std::unique_ptr<ExprAST> parsePostFix(std::unique_ptr<ExprAST> expr);
    
    int getPrecedence(const std::string& op) const;
};