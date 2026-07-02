#include "../include/lexer.hpp"
#include <cctype>

Lexer::Lexer(const std::string& source) : source(source), pos(0), line(1), is_at_line_start(true) {
    indent_stack.push_back(0);
}

char Lexer::peek() {
    if (pos >= source.length()) return '\0';
    return source[pos];
}

char Lexer::advance() {
    if (pos >= source.length()) return '\0';
    return source[pos++];
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;
    int bracket_nesting = 0;
    while (pos < source.length()) {
        if (is_at_line_start) {
            int spaces = 0;
            while (peek() == ' ') { advance(); spaces++; }
            
            if (bracket_nesting > 0) {
                is_at_line_start = false;
            } else if (peek() == '\n' || peek() == '\r' || peek() == '#' || peek() == '\0') {
                // Ignore indentation on empty lines
            } else {
                if (spaces > indent_stack.back()) {
                    indent_stack.push_back(spaces);
                    tokens.push_back({TokenType::INDENT, "", line});
                } else if (spaces < indent_stack.back()) {
                    while (indent_stack.size() > 1 && spaces < indent_stack.back()) {
                        indent_stack.pop_back();
                        tokens.push_back({TokenType::DEDENT, "", line});
                    }
                }
                is_at_line_start = false;
            }
        }

        if (pos >= source.length()) break;

        char c = peek();

        if (c == '\n') {
            advance();
            line++;
            is_at_line_start = true;
            if (bracket_nesting == 0) {
                tokens.push_back({TokenType::NEWLINE, "", line});
            }
            continue;
        }

        if (c == '\r' || c == ' ' || c == '\t') {
            advance();
            continue;
        }

        if (c == '#') {
            while (peek() != '\n' && peek() != '\0') advance();
            continue;
        }

        if (std::isalpha(c)) {
            std::string id = "";
            while (std::isalnum(peek()) || peek() == '_') {
                id += advance();
            }
            
            if (id == "int" || id == "int32" || id == "int64" || id == "int128" || id == "int256" || id == "string" || id == "float" || id == "float32" || id == "float64" || id == "bool" || id == "array" || id == "dict") tokens.push_back({TokenType::KEYWORD_TYPE, id, line});
            else if (id == "if") tokens.push_back({TokenType::KEYWORD_IF, id, line});
            else if (id == "elif") tokens.push_back({TokenType::KEYWORD_ELIF, id, line});
            else if (id == "else") tokens.push_back({TokenType::KEYWORD_ELSE, id, line});
            else if (id == "while") tokens.push_back({TokenType::KEYWORD_WHILE, id, line});
            else if (id == "for") tokens.push_back({TokenType::KEYWORD_FOR, id, line});
            else if (id == "in") tokens.push_back({TokenType::KEYWORD_IN, id, line});
            else if (id == "fn") tokens.push_back({TokenType::KEYWORD_FN, id, line});
            else if (id == "return") tokens.push_back({TokenType::KEYWORD_RETURN, id, line});
            else if (id == "and") tokens.push_back({TokenType::KEYWORD_AND, id, line});
            else if (id == "or") tokens.push_back({TokenType::KEYWORD_OR, id, line});
            else if (id == "not") tokens.push_back({TokenType::KEYWORD_NOT, id, line});
            else if (id == "struct") tokens.push_back({TokenType::KEYWORD_STRUCT, id, line});
            else if (id == "try") tokens.push_back({TokenType::KEYWORD_TRY, id, line});
            else if (id == "except") tokens.push_back({TokenType::KEYWORD_EXCEPT, id, line});
            else if (id == "throw") tokens.push_back({TokenType::KEYWORD_THROW, id, line});
            else if (id == "import") tokens.push_back({TokenType::KEYWORD_IMPORT, id, line});
            else if (id == "true" || id == "false") tokens.push_back({TokenType::BOOL_LITERAL, id, line});
            else tokens.push_back({TokenType::IDENTIFIER, id, line});
            continue;
        }

        if (std::isdigit(c)) {
            std::string num = "";
            bool is_float = false;
            while (std::isdigit(peek()) || peek() == '.') {
                if (peek() == '.') is_float = true;
                num += advance();
            }
            if (is_float) tokens.push_back({TokenType::FLOAT_LITERAL, num, line});
            else tokens.push_back({TokenType::INT_LITERAL, num, line});
            continue;
        }

        if (c == '"') {
            advance(); // skip opening quote
            std::string str = "";
            while (peek() != '"' && peek() != '\0') {
                if (peek() == '\\') {
                    advance(); // skip '\\'
                    if (peek() == 'n') {
                        str += '\n';
                        advance();
                    } else if (peek() == 't') {
                        str += '\t';
                        advance();
                    } else if (peek() == 'r') {
                        str += '\r';
                        advance();
                    } else if (peek() == '"') {
                        str += '"';
                        advance();
                    } else if (peek() == '\\') {
                        str += '\\';
                        advance();
                    } else {
                        str += '\\';
                    }
                } else {
                    str += advance();
                }
            }
            advance(); // skip closing quote
            tokens.push_back({TokenType::STRING_LITERAL, str, line});
            continue;
        }

        // Two-character operators
        if (c == '=' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::OPERATOR, "==", line}); continue; }
        if (c == '!' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::OPERATOR, "!=", line}); continue; }
        if (c == '-' && pos + 1 < source.length() && source[pos + 1] == '>') { advance(); advance(); tokens.push_back({TokenType::ARROW, "->", line}); continue; }
        if (c == '>' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::OPERATOR, ">=", line}); continue; }
        if (c == '<' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::OPERATOR, "<=", line}); continue; }

        // Single-character operators and symbols
        if (c == '[' || c == '{' || c == '(') {
            advance();
            bracket_nesting++;
            if (c == '[') tokens.push_back({TokenType::LBRACKET, "[", line});
            else if (c == '{') tokens.push_back({TokenType::LBRACE, "{", line});
            else tokens.push_back({TokenType::LPAREN, "(", line});
            continue;
        }
        if (c == ']' || c == '}' || c == ')') {
            advance();
            if (bracket_nesting > 0) bracket_nesting--;
            if (c == ']') tokens.push_back({TokenType::RBRACKET, "]", line});
            else if (c == '}') tokens.push_back({TokenType::RBRACE, "}", line});
            else tokens.push_back({TokenType::RPAREN, ")", line});
            continue;
        }
        if (c == '=' || c == '+' || c == '-' || c == '*' || c == '/' || c == '>' || c == '<' || c == '%' || c == '^') {
            advance();
            if (c == '=') tokens.push_back({TokenType::ASSIGN, "=", line});
            else tokens.push_back({TokenType::OPERATOR, std::string(1, c), line});
            continue;
        }
        if (c == '.') { advance(); tokens.push_back({TokenType::DOT, ".", line}); continue; }

        if (c == ':') { advance(); tokens.push_back({TokenType::COLON, ":", line}); continue; }
        if (c == ',') { advance(); tokens.push_back({TokenType::COMMA, ",", line}); continue; }

        // Skip unsupported tokens
        advance();
    }
    
    while (indent_stack.size() > 1) {
        indent_stack.pop_back();
        tokens.push_back({TokenType::DEDENT, "", line});
    }
    tokens.push_back({TokenType::EOF_TOKEN, "", line});
    return tokens;
}