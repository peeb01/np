#include "lexer.hpp"
#include <cctype>

Lexer::Lexer(const std::string& source) : source(source), pos(0), line(1), bracket_nesting(0) {
    indent_stack.push_back(0);
}

char Lexer::peek() const {
    if (pos >= source.length()) return '\0';
    return source[pos];
}

char Lexer::advance() {
    if (pos >= source.length()) return '\0';
    char c = source[pos++];
    if (c == '\n') line++;
    return c;
}

std::vector<Token> Lexer::tokenize() {
    std::vector<Token> tokens;

    while (pos < source.length()) {
        char c = peek();

        // Handle Indentation & Newlines
        if (c == '\n') {
            advance();
            // Don't emit newlines or handle indentation if we are inside brackets [] or {} or ()
            if (bracket_nesting > 0) {
                continue;
            }

            tokens.push_back({TokenType::NEWLINE, "\\n", line - 1});

            // Calculate current line indentation
            int spaces = 0;
            while (peek() == ' ' || peek() == '\t') {
                if (peek() == '\t') spaces += 4;
                else spaces += 1;
                advance();
            }

            // Ignore blank lines or comment-only lines
            if (peek() == '\n' || peek() == '\r' || peek() == '\0' || peek() == '#') {
                continue;
            }

            int current_indent = indent_stack.back();
            if (spaces > current_indent) {
                indent_stack.push_back(spaces);
                tokens.push_back({TokenType::INDENT, "", line});
            } else if (spaces < current_indent) {
                while (indent_stack.size() > 1 && indent_stack.back() > spaces) {
                    indent_stack.pop_back();
                    tokens.push_back({TokenType::DEDENT, "", line});
                }
            }
            continue;
        }

        if (c == ' ' || c == '\t' || c == '\r') {
            advance();
            continue;
        }

        if (c == '#') {
            while (peek() != '\n' && peek() != '\0') advance();
            continue;
        }

        // F-String literal: f"..." or F"..."
        if ((c == 'f' || c == 'F') && pos + 1 < source.length() && source[pos + 1] == '"') {
            advance(); // skip 'f'
            advance(); // skip '"'
            std::string str = "";
            while (peek() != '"' && peek() != '\0') {
                if (peek() == '\\') {
                    advance();
                    if (peek() == 'n') { str += '\n'; advance(); }
                    else if (peek() == 't') { str += '\t'; advance(); }
                    else if (peek() == 'r') { str += '\r'; advance(); }
                    else if (peek() == '"') { str += '"'; advance(); }
                    else if (peek() == '\\') { str += '\\'; advance(); }
                    else { str += '\\'; }
                } else {
                    str += advance();
                }
            }
            advance(); // skip closing quote
            tokens.push_back({TokenType::FSTRING_LITERAL, str, line});
            continue;
        }

        if (std::isalpha(c) || c == '_') {
            std::string id = "";
            while (std::isalnum(peek()) || peek() == '_') {
                id += advance();
            }
            
            // Types
            if (id == "int" || id == "int8" || id == "int16" || id == "int32" || id == "int64" || 
                id == "int128" || id == "int256" || id == "uint" || id == "uint8" || id == "byte" || 
                id == "uint16" || id == "uint32" || id == "uint64" || id == "uint128" || id == "uint256" || 
                id == "string" || id == "float" || id == "float32" || id == "float64" || id == "bool" || 
                id == "array" || id == "dict") {
                tokens.push_back({TokenType::KEYWORD_TYPE, id, line});
            }
            else if (id == "if") tokens.push_back({TokenType::KEYWORD_IF, id, line});
            else if (id == "elif") tokens.push_back({TokenType::KEYWORD_ELIF, id, line});
            else if (id == "else") tokens.push_back({TokenType::KEYWORD_ELSE, id, line});
            else if (id == "while") tokens.push_back({TokenType::KEYWORD_WHILE, id, line});
            else if (id == "for") tokens.push_back({TokenType::KEYWORD_FOR, id, line});
            else if (id == "in") tokens.push_back({TokenType::KEYWORD_IN, id, line});
            else if (id == "fn" || id == "func" || id == "function") tokens.push_back({TokenType::KEYWORD_FN, id, line});
            else if (id == "return") tokens.push_back({TokenType::KEYWORD_RETURN, id, line});
            else if (id == "break") tokens.push_back({TokenType::KEYWORD_BREAK, id, line});
            else if (id == "continue") tokens.push_back({TokenType::KEYWORD_CONTINUE, id, line});
            else if (id == "nil") tokens.push_back({TokenType::KEYWORD_NIL, id, line});
            else if (id == "defer") tokens.push_back({TokenType::KEYWORD_DEFER, id, line});
            else if (id == "go") tokens.push_back({TokenType::KEYWORD_GO, id, line});
            else if (id == "var") tokens.push_back({TokenType::KEYWORD_VAR, id, line});
            else if (id == "switch") tokens.push_back({TokenType::KEYWORD_SWITCH, id, line});
            else if (id == "case") tokens.push_back({TokenType::KEYWORD_CASE, id, line});
            else if (id == "default") tokens.push_back({TokenType::KEYWORD_DEFAULT, id, line});
            else if (id == "enum") tokens.push_back({TokenType::KEYWORD_ENUM, id, line});
            else if (id == "and") tokens.push_back({TokenType::KEYWORD_AND, id, line});
            else if (id == "or") tokens.push_back({TokenType::KEYWORD_OR, id, line});
            else if (id == "not") tokens.push_back({TokenType::KEYWORD_NOT, id, line});
            else if (id == "struct") tokens.push_back({TokenType::KEYWORD_STRUCT, id, line});
            else if (id == "try") tokens.push_back({TokenType::KEYWORD_TRY, id, line});
            else if (id == "except") tokens.push_back({TokenType::KEYWORD_EXCEPT, id, line});
            else if (id == "throw") tokens.push_back({TokenType::KEYWORD_THROW, id, line});
            else if (id == "import") tokens.push_back({TokenType::KEYWORD_IMPORT, id, line});
            else if (id == "assert") tokens.push_back({TokenType::KEYWORD_ASSERT, id, line});
            else if (id == "interface") tokens.push_back({TokenType::KEYWORD_INTERFACE, id, line});
            else if (id == "kernel") tokens.push_back({TokenType::KEYWORD_KERNEL, id, line});
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

        // Two-character operators and symbols
        if (c == ':' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::COLON_ASSIGN, ":=", line}); continue; }
        if (c == '+' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::PLUS_EQUAL, "+=", line}); continue; }
        if (c == '-' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::MINUS_EQUAL, "-=", line}); continue; }
        if (c == '*' && pos + 1 < source.length() && source[pos + 1] == '*') { advance(); advance(); tokens.push_back({TokenType::OPERATOR, "**", line}); continue; }
        if (c == '*' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::MUL_EQUAL, "*=", line}); continue; }
        if (c == '/' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::DIV_EQUAL, "/=", line}); continue; }
        if (c == '%' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::MOD_EQUAL, "%=", line}); continue; }
        if (c == '+' && pos + 1 < source.length() && source[pos + 1] == '+') { advance(); advance(); tokens.push_back({TokenType::PLUS_PLUS, "++", line}); continue; }
        if (c == '-' && pos + 1 < source.length() && source[pos + 1] == '-') { advance(); advance(); tokens.push_back({TokenType::MINUS_MINUS, "--", line}); continue; }
        if (c == '<' && pos + 1 < source.length() && source[pos + 1] == '<') { advance(); advance(); tokens.push_back({TokenType::LSHIFT, "<<", line}); continue; }
        if (c == '>' && pos + 1 < source.length() && source[pos + 1] == '>') { advance(); advance(); tokens.push_back({TokenType::RSHIFT, ">>", line}); continue; }
        if (c == '=' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::OPERATOR, "==", line}); continue; }
        if (c == '!' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::OPERATOR, "!=", line}); continue; }
        if (c == '-' && pos + 1 < source.length() && source[pos + 1] == '>') { advance(); advance(); tokens.push_back({TokenType::ARROW, "->", line}); continue; }
        if (c == '>' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::OPERATOR, ">=", line}); continue; }
        if (c == '<' && pos + 1 < source.length() && source[pos + 1] == '=') { advance(); advance(); tokens.push_back({TokenType::OPERATOR, "<=", line}); continue; }
        if (c == '<' && pos + 1 < source.length() && source[pos + 1] == '-') { advance(); advance(); tokens.push_back({TokenType::LARROW, "<-", line}); continue; }

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
        if (c == '&') { advance(); tokens.push_back({TokenType::BIT_AND, "&", line}); continue; }
        if (c == '|') { advance(); tokens.push_back({TokenType::BIT_OR, "|", line}); continue; }
        if (c == '~') { advance(); tokens.push_back({TokenType::BIT_XOR, "~", line}); continue; }
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