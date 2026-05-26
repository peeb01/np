#pragma once
#include "common.hpp"
#include <string>
#include <vector>

class Lexer {
public:
    Lexer(const std::string& source);
    std::vector<Token> tokenize();

private:
    std::string source;
    size_t pos;
    int line;

    char peek();
    char advance();
    std::vector<int> indent_stack;
    bool is_at_line_start;
};