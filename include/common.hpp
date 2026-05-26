#pragma once
#include <string>

enum class TokenType {
    KEYWORD_TYPE,    // int, string, float, bool
    KEYWORD_IF,      // if
    KEYWORD_ELIF,    // elif
    KEYWORD_ELSE,    // else
    KEYWORD_WHILE,   // while
    KEYWORD_FOR,     // for
    KEYWORD_IN,      // in
    KEYWORD_FN,      // fn
    KEYWORD_RETURN,  // return
    KEYWORD_AND,     // and
    KEYWORD_OR,      // or
    KEYWORD_NOT,     // not
    IDENTIFIER,      // A, B, x, y
    ASSIGN,          // =
    STRING_LITERAL,  // "123"
    INT_LITERAL,     // 123
    FLOAT_LITERAL,   // 3.14
    BOOL_LITERAL,    // true, false
    LPAREN,          // (
    RPAREN,          // )
    LBRACKET,        // [
    RBRACKET,        // ]
    LBRACE,          // {
    RBRACE,          // }
    DOT,             // .
    COLON,           // :
    ARROW,           // ->
    COMMA,           // ,
    OPERATOR,        // +, -, *, /, %, ==, !=, >, <, >=, <=, ^
    NEWLINE,         // \n
    INDENT,          // Start of block
    DEDENT,          // End of block
    EOF_TOKEN        // End of File
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};