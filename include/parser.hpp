#pragma once
#include "common.hpp"
#include <vector>
#include <string>
#include <unordered_map>

class Parser {
public:
    Parser(const std::vector<Token>& tokens);
    void parse();
    std::string getTranslatedCode() const;

private:
    std::vector<Token> tokens;
    size_t pos;
    std::string translated_code;
    std::string global_code;
    std::unordered_map<std::string, std::string> variables;
    std::vector<bool> is_index_stack;

    Token peek(int offset = 0) const;
    Token advance();
    bool check(TokenType type) const;
    std::string getTypeOfToken(const Token& t) const;
    void expect(TokenType type, const std::string& err_msg);
    bool isAtEnd() const;

    std::string parseBlock();
    std::string parseStatement();
    std::string parseExpressionUntil(TokenType stop1, TokenType stop2);
    std::string parseExpression();
};