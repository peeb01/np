#include "parser.hpp"

void Parser::parse() {
    while (!isAtEnd()) {
        if (check(TokenType::NEWLINE)) { 
            advance(); 
            continue; 
        }
        auto stmt = parseStatement();
        if (stmt) {
            ast_root.push_back(std::move(stmt));
        }
    }
}