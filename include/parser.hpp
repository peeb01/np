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

    // Token inspection and navigation
    Token peek(int offset = 0) const;
    Token advance();
    bool check(TokenType type) const;
    void expect(TokenType type, const std::string& err_msg);
    bool isAtEnd() const;
    bool isBuiltIn(const std::string& name) const;
    int getPrecedence(const std::string& op) const;

    // Expression parsing
    std::unique_ptr<ExprAST> parseExpression();
    std::unique_ptr<ExprAST> parseExpression(int min_precedence);
    std::unique_ptr<ExprAST> parsePrimary();
    std::unique_ptr<ExprAST> parsePostFix(std::unique_ptr<ExprAST> expr);

    // Block & Statement parsing
    std::unique_ptr<BlockStmtAST> parseBlock();
    std::unique_ptr<StmtAST> parseStatement();

    // Specific statement parsers
    std::unique_ptr<StmtAST> parseFunction();
    std::unique_ptr<StmtAST> parseStruct();
    std::unique_ptr<StmtAST> parseIf();
    std::unique_ptr<StmtAST> parseWhile();
    std::unique_ptr<StmtAST> parseFor();
    std::unique_ptr<StmtAST> parseReturn();
    std::unique_ptr<StmtAST> parseBreak();
    std::unique_ptr<StmtAST> parseContinue();
    std::unique_ptr<StmtAST> parseDefer();
    std::unique_ptr<StmtAST> parseGo();
    std::unique_ptr<StmtAST> parseSwitch();
    std::unique_ptr<StmtAST> parseEnum();
    std::unique_ptr<StmtAST> parseInterface();
    std::unique_ptr<StmtAST> parseAssert();
    std::unique_ptr<StmtAST> parseVarDecl();
    std::unique_ptr<StmtAST> parseTryExcept();
    std::unique_ptr<StmtAST> parseThrow();
    std::unique_ptr<StmtAST> parseImport();
    std::unique_ptr<StmtAST> parseSingleImport();
    std::string parseType();

    std::unordered_map<std::string, std::unordered_set<std::string>> struct_methods;
    std::unordered_map<std::string, std::unordered_map<std::string, int>> enums;
    std::unordered_map<std::string, std::unordered_set<std::string>> interfaces;

    // Parser State
    std::vector<Token> tokens;
    size_t pos;
    std::vector<std::unique_ptr<ASTNode>> ast_root;
    std::unordered_set<std::string> imported_modules;
    std::string namespace_prefix;
    bool is_inside_function;
    std::unordered_set<std::string> local_variables;
    std::unordered_set<std::string> import_aliases;
};