#pragma once
#include <string>

enum class TokenType {
    KEYWORD_TYPE,    // int, string, float, bool, uint8, byte, uint16, uint32, uint64, uint, int8, int16, etc.
    KEYWORD_IF,      // if
    KEYWORD_ELIF,    // elif
    KEYWORD_ELSE,    // else
    KEYWORD_WHILE,   // while
    KEYWORD_FOR,     // for
    KEYWORD_IN,      // in
    KEYWORD_FN,      // fn, func, function
    KEYWORD_RETURN,  // return
    KEYWORD_BREAK,   // break
    KEYWORD_CONTINUE,// continue
    KEYWORD_NIL,     // nil
    KEYWORD_DEFER,   // defer
    KEYWORD_GO,      // go
    KEYWORD_VAR,     // var
    KEYWORD_SWITCH,  // switch
    KEYWORD_CASE,    // case
    KEYWORD_DEFAULT, // default
    KEYWORD_ENUM,    // enum
    KEYWORD_AND,     // and
    KEYWORD_OR,      // or
    KEYWORD_NOT,     // not
    KEYWORD_STRUCT,  // struct
    KEYWORD_TRY,     // try
    KEYWORD_EXCEPT,  // except
    KEYWORD_THROW,   // throw
    KEYWORD_IMPORT,  // import
    IDENTIFIER,      // A, B, x, y
    ASSIGN,          // =
    COLON_ASSIGN,    // :=
    PLUS_EQUAL,      // +=
    MINUS_EQUAL,     // -=
    MUL_EQUAL,       // *=
    DIV_EQUAL,       // /=
    MOD_EQUAL,       // %=
    PLUS_PLUS,       // ++
    MINUS_MINUS,     // --
    BIT_AND,         // &
    BIT_OR,          // |
    BIT_XOR,         // ~
    LSHIFT,          // <<
    RSHIFT,          // >>
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
    KEYWORD_ASSERT,  // assert
    FSTRING_LITERAL, // f"..."
    KEYWORD_INTERFACE, // interface
    LARROW,          // <-
    EOF_TOKEN        // End of File
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};