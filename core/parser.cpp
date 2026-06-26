#include "../include/parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include "../include/lexer.hpp"

static std::unordered_set<std::string> imported_files;

Parser::Parser(const std::vector<Token>& tokens) : tokens(tokens), pos(0) {}

Token Parser::peek(int offset) const {
    if (pos + offset >= tokens.size()) return tokens.back();
    return tokens[pos + offset];
}

Token Parser::advance() {
    if (!isAtEnd()) pos++;
    return tokens[pos - 1];
}

bool Parser::check(TokenType type) const { return peek().type == type; }
bool Parser::isAtEnd() const { return check(TokenType::EOF_TOKEN); }

void Parser::expect(TokenType type, const std::string& err_msg) {
    if (check(type)) {
        advance();
    } else {
        std::cerr << "Syntax Error on line " << peek().line << ": " << err_msg << "\n";
        exit(1);
    }
}

std::string Parser::getTypeOfToken(const Token& t) const {
    if (t.type == TokenType::INT_LITERAL) return "int";
    if (t.type == TokenType::FLOAT_LITERAL) return "float";
    if (t.type == TokenType::STRING_LITERAL) return "string";
    if (t.type == TokenType::BOOL_LITERAL) return "bool";
    if (t.type == TokenType::LBRACKET) return "array";
    if (t.type == TokenType::LBRACE) return "dict";
    if (t.type == TokenType::IDENTIFIER) {
        if (variables.count(t.value)) return variables.at(t.value);
    }
    return "";
}

void Parser::parse() {
    while (!isAtEnd()) {
        if (check(TokenType::NEWLINE)) { advance(); continue; }
        bool is_function = check(TokenType::KEYWORD_FN);
        size_t start_pos = pos;
        std::string stmt = parseStatement();
        
        if (pos == start_pos) {
            std::cerr << "Syntax Error on line " << peek().line << ": Unexpected token '" << peek().value << "'\n";
            exit(1);
        }

        if (is_function) {
             global_code += stmt;
        } else {
             translated_code += stmt;
        }
    }
}

std::string Parser::getTranslatedCode() const {
    return global_code + "\nint main(int argc, char* argv[]) {\n    np_init_args(argc, argv);\n" + translated_code + "\nreturn 0;\n}\n";
}

std::string Parser::parseBlock() {
    std::string block = "";
    while (check(TokenType::NEWLINE)) advance();
    if (check(TokenType::INDENT)) {
        advance();
        while (!check(TokenType::DEDENT) && !isAtEnd()) {
            if (check(TokenType::NEWLINE)) { advance(); continue; }
            block += parseStatement();
        }
        if (check(TokenType::DEDENT)) advance();
    }
    return block;
}

std::string Parser::parseExpressionUntil(TokenType stop1, TokenType stop2) {
    std::string expr = "";
    Token prev_token = {TokenType::EOF_TOKEN, "", 0};
    Token op_token = {TokenType::EOF_TOKEN, "", 0};
    int nesting = 0;

    while (!isAtEnd()) {
        if (nesting <= 0) {
            if (check(stop1) || check(stop2) || check(TokenType::NEWLINE)) {
                break;
            }
        }
        
        Token t = advance();
        
        if (t.type == TokenType::LPAREN) nesting++;
        if (t.type == TokenType::RPAREN) nesting--;

        if (t.type == TokenType::IDENTIFIER && (t.value == "sys" || t.value == "json" || t.value == "time" || t.value == "regex")) {
            std::string mod_name = t.value;
            if (!imported_modules.count(mod_name)) {
                std::cerr << "Syntax Error on line " << t.line << ": Module '" << mod_name << "' must be imported before use\n";
                exit(1);
            }
            expect(TokenType::DOT, "Expected '.' after module name");
            Token member = advance();
            if (member.type != TokenType::IDENTIFIER) {
                std::cerr << "Syntax Error on line " << member.line << ": Expected identifier after '.' on module '" << mod_name << "'\n";
                exit(1);
            }
            if (mod_name == "sys") {
                if (member.value == "argv" || member.value == "args") {
                    expr += "np_sys_argv";
                } else {
                    std::cerr << "Syntax Error on line " << member.line << ": Unknown member '" << member.value << "' on sys module\n";
                    exit(1);
                }
            } else if (mod_name == "json") {
                if (member.value == "parse" || member.value == "unmarshal") {
                    expr += "np_json_parse";
                } else if (member.value == "stringify" || member.value == "marshal") {
                    expr += "np_json_stringify";
                } else {
                    std::cerr << "Syntax Error on line " << member.line << ": Unknown member '" << member.value << "' on json module\n";
                    exit(1);
                }
            } else if (mod_name == "time") {
                if (member.value == "now") {
                    expr += "np_time_now";
                } else if (member.value == "sleep") {
                    expr += "np_time_sleep";
                } else if (member.value == "format") {
                    expr += "np_time_format";
                } else {
                    std::cerr << "Syntax Error on line " << member.line << ": Unknown member '" << member.value << "' on time module\n";
                    exit(1);
                }
            } else if (mod_name == "regex") {
                if (member.value == "match") {
                    expr += "np_regex_match";
                } else if (member.value == "find") {
                    expr += "np_regex_find";
                } else if (member.value == "replace") {
                    expr += "np_regex_replace";
                } else {
                    std::cerr << "Syntax Error on line " << member.line << ": Unknown member '" << member.value << "' on regex module\n";
                    exit(1);
                }
            }
            prev_token = member;
            continue;
        }

        if (t.type == TokenType::IDENTIFIER && t.value == "input") {
            if (check(TokenType::LPAREN)) { advance(); if (check(TokenType::RPAREN)) advance(); }
            expr += "np_input()"; prev_token = t; continue;
        }
        if (t.type == TokenType::IDENTIFIER && t.value == "print") { expr += "np_print"; prev_token = t; continue; }
        if (t.type == TokenType::IDENTIFIER && t.value == "type") { expr += "np_type"; prev_token = t; continue; }
        if (t.type == TokenType::IDENTIFIER && t.value == "sqrt") { expr += "np_sqrt"; prev_token = t; continue; }
        if (t.type == TokenType::IDENTIFIER && t.value == "abs") { expr += "np_abs"; prev_token = t; continue; }
        if (t.type == TokenType::IDENTIFIER && t.value == "min") { expr += "np_min"; prev_token = t; continue; }
        if (t.type == TokenType::IDENTIFIER && t.value == "max") { expr += "np_max"; prev_token = t; continue; }
        if (t.type == TokenType::IDENTIFIER && t.value == "round") { expr += "np_round"; prev_token = t; continue; }
        if (t.type == TokenType::IDENTIFIER && t.value == "len") { expr += "np_len"; prev_token = t; continue; }
        if (t.type == TokenType::IDENTIFIER && t.value == "read_file") { expr += "np_read_file"; prev_token = t; continue; }
        if (t.type == TokenType::IDENTIFIER && t.value == "write_file") { expr += "np_write_file"; prev_token = t; continue; }

        // Struct Constructor call
        if (t.type == TokenType::IDENTIFIER && structs.count(t.value)) {
            std::string struct_name = t.value;
            expect(TokenType::LPAREN, "Expected '(' for struct constructor");
            std::vector<std::string> args;
            while (!check(TokenType::RPAREN) && !isAtEnd()) {
                args.push_back(parseExpressionUntil(TokenType::COMMA, TokenType::RPAREN));
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN, "Expected ')' after constructor arguments");
            
            std::string construct_expr = "np_var(std::map<std::string, np_var>{\n";
            auto& fields = structs[struct_name];
            for (size_t i = 0; i < fields.size(); ++i) {
                construct_expr += "{\"" + fields[i].first + "\", ";
                if (i < args.size()) {
                    construct_expr += "np_var(" + args[i] + ")";
                } else {
                    std::string ftype = fields[i].second;
                    if (ftype == "int" || ftype == "int32" || ftype == "int64" || ftype == "int128" || ftype == "int256" || ftype == "float" || ftype == "float32" || ftype == "float64") {
                        construct_expr += "0";
                    } else if (ftype == "string") {
                        construct_expr += "np_string(\"\")";
                    } else if (ftype == "bool") {
                        construct_expr += "false";
                    } else if (ftype == "array") {
                        construct_expr += "std::vector<np_var>{}";
                    } else if (ftype == "dict") {
                        construct_expr += "std::map<std::string, np_var>{}";
                    } else {
                        construct_expr += "np_var()";
                    }
                }
                construct_expr += "}";
                if (i < fields.size() - 1) construct_expr += ", ";
            }
            construct_expr += "})";
            expr += construct_expr;
            prev_token = {TokenType::RPAREN, ")", t.line};
            continue;
        }

        // Auto-binding for Array properties
        if (t.type == TokenType::IDENTIFIER && (t.value == "length" || t.value == "shape")) {
            if (prev_token.type == TokenType::DOT) {
                expr += t.value;
                if (!check(TokenType::LPAREN)) expr += "()";
                prev_token = t;
                continue;
            }
        }

        // Dot member access translation to index access
        if (t.type == TokenType::DOT) {
            Token next = peek();
            if (next.type == TokenType::IDENTIFIER) {
                std::string method = next.value;
                if (method == "length" || method == "shape" || method == "sort" || method == "reverse" || 
                    method == "contains" || method == "split" || method == "join" || method == "trim" ||
                    method == "keys" || method == "values" || method == "append" || method == "pop" ||
                    method == "clear" || method == "has_key") {
                    expr += ".";
                    prev_token = t;
                    continue;
                } else {
                    advance(); // consume the field name
                    expr += "[\"" + method + "\"]";
                    prev_token = {TokenType::RBRACKET, "]", t.line};
                    continue;
                }
            } else {
                expr += ".";
                prev_token = t;
                continue;
            }
        }

        // Array Literal & Indexing Tracker & List Comprehension
        if (t.type == TokenType::LBRACKET) {
            nesting++;
            
            // Check for list comprehension
            int depth = 1;
            bool is_comp = false;
            size_t scan_pos = pos;
            while (scan_pos < tokens.size()) {
                Token st = tokens[scan_pos];
                if (st.type == TokenType::LBRACKET) depth++;
                else if (st.type == TokenType::RBRACKET) {
                    depth--;
                    if (depth == 0) break;
                }
                else if (st.type == TokenType::KEYWORD_FOR && depth == 1) {
                    is_comp = true;
                    break;
                }
                scan_pos++;
            }

            if (is_comp) {
                std::string comp_expr = parseExpressionUntil(TokenType::KEYWORD_FOR, TokenType::KEYWORD_FOR);
                expect(TokenType::KEYWORD_FOR, "Expected 'for' in list comprehension");
                std::string var = advance().value;
                expect(TokenType::KEYWORD_IN, "Expected 'in' in list comprehension");
                
                std::string loop_header = "";
                if (check(TokenType::IDENTIFIER) && peek().value == "range") {
                    advance(); // range
                    expect(TokenType::LPAREN, "Expected '(' after range");
                    std::string start = parseExpressionUntil(TokenType::COMMA, TokenType::COMMA);
                    expect(TokenType::COMMA, "Expected ',' in range");
                    std::string end = parseExpressionUntil(TokenType::RPAREN, TokenType::RPAREN);
                    expect(TokenType::RPAREN, "Expected ')' after range");
                    loop_header = "for (int " + var + " = " + start + "; " + var + " < " + end + "; ++" + var + ")";
                } else {
                    std::string coll = parseExpressionUntil(TokenType::KEYWORD_IF, TokenType::RBRACKET);
                    loop_header = "for (const np_var& " + var + " : " + coll + ")";
                }
                
                bool has_if = false;
                std::string cond = "";
                if (check(TokenType::KEYWORD_IF)) {
                    advance(); // if
                    cond = parseExpressionUntil(TokenType::RBRACKET, TokenType::RBRACKET);
                    has_if = true;
                }
                expect(TokenType::RBRACKET, "Expected ']' at end of list comprehension");
                
                std::string iife = "([&]() { std::vector<np_var> _res; " + loop_header + " { " +
                                   (has_if ? "if (" + cond + ") { _res.push_back(" + comp_expr + "); }" : "_res.push_back(" + comp_expr + ");") +
                                   " } return np_var(_res); })()";
                expr += iife;
                nesting--;
                prev_token = {TokenType::RBRACKET, "]", t.line};
                continue;
            }
            
            Token prev_actual = (pos >= 2) ? tokens[pos - 2] : Token{TokenType::EOF_TOKEN, "", 0};
            if (prev_actual.type == TokenType::IDENTIFIER || 
                prev_actual.type == TokenType::RBRACKET || 
                prev_actual.type == TokenType::RPAREN || 
                prev_actual.type == TokenType::RBRACE) {
                
                int sdepth = 1;
                bool is_slice = false;
                size_t scan_pos = pos;
                while (scan_pos < tokens.size()) {
                    Token st = tokens[scan_pos];
                    if (st.type == TokenType::LBRACKET) sdepth++;
                    else if (st.type == TokenType::RBRACKET) {
                        sdepth--;
                        if (sdepth == 0) break;
                    }
                    else if (st.type == TokenType::COLON && sdepth == 1) {
                        is_slice = true;
                    }
                    scan_pos++;
                }

                if (is_slice) {
                    expr += ".slice(";
                    std::string start_expr = "";
                    if (check(TokenType::COLON)) {
                        start_expr = "0";
                    } else {
                        start_expr = parseExpressionUntil(TokenType::COLON, TokenType::COLON);
                    }
                    expr += start_expr;
                    expect(TokenType::COLON, "Expected ':' in slice");
                    std::string end_expr = "";
                    if (check(TokenType::RBRACKET)) {
                        end_expr = "-999999";
                    } else {
                        end_expr = parseExpressionUntil(TokenType::RBRACKET, TokenType::RBRACKET);
                    }
                    expr += ", " + end_expr + ")";
                    expect(TokenType::RBRACKET, "Expected ']' at end of slice");
                    nesting--;
                    prev_token = {TokenType::RBRACKET, "]", t.line};
                    continue;
                } else {
                    expr += "[";
                    is_index_stack.push_back(true);
                }
            } else {
                expr += "std::vector<np_var>{";
                is_index_stack.push_back(false);
            }
            prev_token = t;
            continue;
        }
        if (t.type == TokenType::RBRACKET) {
            nesting--;
            if (!is_index_stack.empty()) {
                if (is_index_stack.back()) expr += "]";
                else expr += "}";
                is_index_stack.pop_back();
            } else {
                expr += "]";
            }
            prev_token = t;
            continue;
        }
        
        // Dictionary Dictionary Tracker & Dict Comprehension
        if (t.type == TokenType::LBRACE) {
            nesting++;
            
            // Check for dict comprehension
            int depth = 1;
            bool is_comp = false;
            size_t scan_pos = pos;
            while (scan_pos < tokens.size()) {
                Token st = tokens[scan_pos];
                if (st.type == TokenType::LBRACE) depth++;
                else if (st.type == TokenType::RBRACE) {
                    depth--;
                    if (depth == 0) break;
                }
                else if (st.type == TokenType::KEYWORD_FOR && depth == 1) {
                    is_comp = true;
                    break;
                }
                scan_pos++;
            }

            if (is_comp) {
                std::string key_expr = parseExpressionUntil(TokenType::COLON, TokenType::COLON);
                expect(TokenType::COLON, "Expected ':' in dict comprehension");
                std::string val_expr = parseExpressionUntil(TokenType::KEYWORD_FOR, TokenType::KEYWORD_FOR);
                expect(TokenType::KEYWORD_FOR, "Expected 'for' in dict comprehension");
                
                std::string loop_header = "";
                std::string var1 = advance().value;
                if (check(TokenType::COMMA)) {
                    advance(); // ,
                    std::string var2 = advance().value;
                    expect(TokenType::KEYWORD_IN, "Expected 'in' in dict comprehension");
                    std::string coll = parseExpressionUntil(TokenType::KEYWORD_IF, TokenType::RBRACE);
                    loop_header = "auto&& _coll = " + coll + "; if (std::holds_alternative<DictPtr>(_coll.v) && std::get<DictPtr>(_coll.v)) for (const auto& [" + var1 + ", " + var2 + "] : std::get<DictPtr>(_coll.v)->map)";
                } else {
                    expect(TokenType::KEYWORD_IN, "Expected 'in' in dict comprehension");
                    if (check(TokenType::IDENTIFIER) && peek().value == "range") {
                        advance(); // range
                        expect(TokenType::LPAREN, "Expected '(' after range");
                        std::string start = parseExpressionUntil(TokenType::COMMA, TokenType::COMMA);
                        expect(TokenType::COMMA, "Expected ',' in range");
                        std::string end = parseExpressionUntil(TokenType::RPAREN, TokenType::RPAREN);
                        expect(TokenType::RPAREN, "Expected ')' after range");
                        loop_header = "for (int " + var1 + " = " + start + "; " + var1 + " < " + end + "; ++" + var1 + ")";
                    } else {
                        std::string coll = parseExpressionUntil(TokenType::KEYWORD_IF, TokenType::RBRACE);
                        loop_header = "for (const np_var& " + var1 + " : " + coll + ")";
                    }
                }
                
                bool has_if = false;
                std::string cond = "";
                if (check(TokenType::KEYWORD_IF)) {
                    advance(); // if
                    cond = parseExpressionUntil(TokenType::RBRACE, TokenType::RBRACE);
                    has_if = true;
                }
                expect(TokenType::RBRACE, "Expected '}' at end of dict comprehension");
                
                std::string iife = "([&]() { std::map<std::string, np_var> _res; " + loop_header + " { " +
                                   (has_if ? "if (" + cond + ") { _res[np_to_string(" + key_expr + ")] = " + val_expr + "; }" : "_res[np_to_string(" + key_expr + ")] = " + val_expr + ";") +
                                   " } return np_var(_res); })()";
                expr += iife;
                nesting--;
                prev_token = {TokenType::RBRACE, "}", t.line};
                continue;
            }
            
            expr += "std::map<std::string, np_var>{";
            while (!check(TokenType::RBRACE) && !check(TokenType::NEWLINE) && !isAtEnd()) {
                expr += "{";
                expr += parseExpressionUntil(TokenType::COLON, TokenType::COLON);
                if (check(TokenType::COLON)) advance();
                expr += ", ";
                expr += parseExpressionUntil(TokenType::COMMA, TokenType::RBRACE);
                expr += "}";
                if (check(TokenType::COMMA)) { expr += ", "; advance(); }
            }
            if (check(TokenType::RBRACE)) { t = advance(); nesting--; }
            expr += "}";
            prev_token = t;
            continue;
        }
        if (t.type == TokenType::RBRACE) { nesting--; expr += "}"; prev_token = t; continue; }

        // Semantic Type Checking for String Concatenation
        if (t.type == TokenType::OPERATOR && t.value == "+") {
            op_token = t;
        } else if (op_token.value == "+") {
            std::string left_type = getTypeOfToken(prev_token);
            std::string right_type = getTypeOfToken(t);
            
            if (!left_type.empty() && !right_type.empty() && left_type != right_type) {
                if ((left_type == "string" && (right_type == "int" || right_type == "float")) ||
                    (right_type == "string" && (left_type == "int" || left_type == "float"))) {
                    std::string err_msg = "TypeError on line " + std::to_string(t.line) + ": Cannot implicitly concatenate '" + (left_type == "string" ? right_type : left_type) + "' and 'string'.";
                    expr += " + np_throw_error(\"" + err_msg + "\")";
                    op_token = {TokenType::EOF_TOKEN, "", 0};
                    continue;
                }
            }
            op_token = {TokenType::EOF_TOKEN, "", 0};
        }
        
        if (t.type == TokenType::KEYWORD_AND) expr += " && ";
        else if (t.type == TokenType::KEYWORD_OR) expr += " || ";
        else if (t.type == TokenType::KEYWORD_NOT) expr += " ! ";
        else if (t.type == TokenType::OPERATOR && t.value == "^") expr += " *np_pow_helper* ";
        else if (t.type == TokenType::KEYWORD_TYPE) {
            if (t.value == "int") expr += "np_to_int";
            else if (t.value == "float") expr += "np_to_float";
            else if (t.value == "string") expr += "np_to_string";
            else expr += t.value;
        } else if (t.type == TokenType::INT_LITERAL) {
            if (t.value.length() > 18) {
                if (t.value.length() <= 38) {
                    expr += "np_int128(\"" + t.value + "\")";
                } else {
                    expr += "np_int256(\"" + t.value + "\")";
                }
            } else {
                expr += t.value;
            }
        } else {
            if (t.type == TokenType::STRING_LITERAL) expr += "np_string(\"" + t.value + "\")";
            else expr += t.value;
        }

        if (t.type != TokenType::OPERATOR && t.type != TokenType::COMMA) {
            prev_token = t;
        }
    }
    return expr;
}

std::string Parser::parseExpression() {
    std::string expr = parseExpressionUntil(TokenType::COLON, TokenType::COLON);
    if (check(TokenType::NEWLINE)) advance();
    return expr;
}

std::string Parser::parseStatement() {
    if (check(TokenType::NEWLINE)) { advance(); return ""; }

    if (check(TokenType::KEYWORD_FN)) {
        advance(); // fn
        std::string name = advance().value;
        expect(TokenType::LPAREN, "Expected '(' after function name");
        std::string params = "";
        while (!check(TokenType::RPAREN) && !isAtEnd()) {
            if (check(TokenType::KEYWORD_TYPE)) {
                std::string type = advance().value;
                std::string np_type = type;
                if (type == "int") type = "int64_t";
                else if (type == "int32") type = "int32_t";
                else if (type == "int64") type = "int64_t";
                else if (type == "int128") type = "np_int128";
                else if (type == "int256") type = "np_int256";
                else if (type == "float") type = "double";
                else if (type == "float32") type = "float";
                else if (type == "float64") type = "double";
                else if (type == "string") type = "np_string";
                else if (type == "array" || type == "dict") type = "np_var";
                std::string param_name = advance().value;
                params += type + " " + param_name;
                variables[param_name] = np_type; // Track parameter type
            } else if (check(TokenType::COMMA)) { 
                params += ", "; advance(); 
            } else {
                std::cerr << "Syntax Error on line " << peek().line << ": Unexpected token in function parameters\n";
                exit(1);
            }
        }
        expect(TokenType::RPAREN, "Expected ')' after parameters");
        std::string ret_type = "void";
        if (check(TokenType::ARROW)) {
            advance(); ret_type = ""; // ->
            while (!check(TokenType::COLON)) {
                if (check(TokenType::KEYWORD_TYPE)) {
                    std::string t = advance().value;
                    if (t == "int") t = "int64_t";
                    else if (t == "int32") t = "int32_t";
                    else if (t == "int64") t = "int64_t";
                    else if (t == "int128") t = "np_int128";
                    else if (t == "int256") t = "np_int256";
                    else if (t == "float") t = "double";
                    else if (t == "float32") t = "float";
                    else if (t == "float64") t = "double";
                    else if (t == "string") t = "np_string";
                    else if (t == "array" || t == "dict") t = "np_var";
                    ret_type += t;
                } else if (check(TokenType::COMMA)) { ret_type += ", "; advance(); }
                else {
                    std::cerr << "Syntax Error on line " << peek().line << ": Unexpected token in function return type\n";
                    exit(1);
                }
            }
            if (ret_type.find(",") != std::string::npos) ret_type = "std::tuple<" + ret_type + ">";
        }
        expect(TokenType::COLON, "Expected ':' before function block");
        if (check(TokenType::NEWLINE)) advance();
        return ret_type + " " + name + "(" + params + ") {\n" + parseBlock() + "}\n";
    }

    if (check(TokenType::KEYWORD_STRUCT)) {
        advance(); // struct
        std::string struct_name = advance().value;
        expect(TokenType::COLON, "Expected ':' after struct name");
        if (check(TokenType::NEWLINE)) advance();
        
        expect(TokenType::INDENT, "Expected indentation for struct body");
        std::vector<std::pair<std::string, std::string>> fields;
        while (!check(TokenType::DEDENT) && !isAtEnd()) {
            if (check(TokenType::NEWLINE)) { advance(); continue; }
            if (check(TokenType::KEYWORD_TYPE)) {
                std::string type = advance().value;
                std::string field_name = advance().value;
                fields.push_back({field_name, type});
                if (check(TokenType::NEWLINE)) advance();
            } else {
                std::cerr << "Syntax Error on line " << peek().line << ": Expected field definition in struct\n";
                exit(1);
            }
        }
        if (check(TokenType::DEDENT)) advance();
        structs[struct_name] = fields;
        return ""; // struct definition does not generate C++ code directly
    }

    if (check(TokenType::IDENTIFIER) && structs.count(peek().value)) {
        std::string type = advance().value; // struct name
        std::string name = advance().value; // variable name
        variables[name] = type;
        
        if (check(TokenType::ASSIGN)) {
            advance(); // =
            return "np_var " + name + " = " + parseExpression() + ";\n";
        }
        
        std::string init = "np_var " + name + " = std::map<std::string, np_var>{\n";
        auto& fields = structs[type];
        for (size_t i = 0; i < fields.size(); ++i) {
            init += "{\"" + fields[i].first + "\", ";
            std::string ftype = fields[i].second;
            if (ftype == "int" || ftype == "int32" || ftype == "int64" || ftype == "int128" || ftype == "int256" || ftype == "float" || ftype == "float32" || ftype == "float64") {
                init += "0";
            } else if (ftype == "string") {
                init += "np_string(\"\")";
            } else if (ftype == "bool") {
                init += "false";
            } else if (ftype == "array") {
                init += "std::vector<np_var>{}";
            } else if (ftype == "dict") {
                init += "std::map<std::string, np_var>{}";
            } else {
                init += "np_var()";
            }
            init += "}";
            if (i < fields.size() - 1) init += ", ";
        }
        init += "};\n";
        return init;
    }

    if (check(TokenType::KEYWORD_TRY)) {
        advance(); // try
        expect(TokenType::COLON, "Expected ':' after try");
        if (check(TokenType::NEWLINE)) advance();
        return "try {\n" + parseBlock() + "}\n";
    }

    if (check(TokenType::KEYWORD_EXCEPT)) {
        advance(); // except
        if (check(TokenType::IDENTIFIER) && peek().value == "Exception") {
            advance();
        } else {
            std::cerr << "Syntax Error on line " << peek().line << ": Expected Exception type after except\n";
            exit(1);
        }
        std::string var_name = advance().value;
        expect(TokenType::COLON, "Expected ':' after except declaration");
        if (check(TokenType::NEWLINE)) advance();
        variables[var_name] = "string";
        return "catch (const std::exception& _e_std) {\nnp_var " + var_name + " = std::string(_e_std.what());\n" + parseBlock() + "}\n";
    }

    if (check(TokenType::KEYWORD_THROW)) {
        advance(); // throw
        std::string expr = parseExpression();
        return "throw std::runtime_error(static_cast<std::string>(np_var(" + expr + ")));\n";
    }

    if (check(TokenType::KEYWORD_IMPORT)) {
        advance(); // import
        Token path_tok = advance();
        if (path_tok.type == TokenType::IDENTIFIER) {
            imported_modules.insert(path_tok.value);
            if (check(TokenType::NEWLINE)) advance();
            return "";
        }
        if (path_tok.type != TokenType::STRING_LITERAL) {
            std::cerr << "Syntax Error on line " << path_tok.line << ": Expected string literal or identifier after import\n";
            exit(1);
        }
        std::string filename = path_tok.value;
        
        if (imported_files.count(filename) == 0) {
            imported_files.insert(filename);
            
            std::ifstream file(filename);
            if (!file.is_open()) {
                std::cerr << "Error: Could not open import file " << filename << "\n";
                exit(1);
            }
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string import_source = buffer.str();
            
            Lexer import_lexer(import_source);
            std::vector<Token> import_tokens = import_lexer.tokenize();
            
            Parser import_parser(import_tokens);
            import_parser.parse();
            
            this->global_code += import_parser.global_code;
            this->translated_code += import_parser.translated_code;
            
            for (auto const& [k, v] : import_parser.variables) {
                this->variables[k] = v;
            }
            for (auto const& [k, v] : import_parser.structs) {
                this->structs[k] = v;
            }
        }
        if (check(TokenType::NEWLINE)) advance();
        return "";
    }

    if (check(TokenType::KEYWORD_IF)) { advance(); std::string cond = parseExpression(); expect(TokenType::COLON, "Expected ':' after 'if' condition"); if (check(TokenType::NEWLINE)) advance(); return "if (" + cond + ") {\n" + parseBlock() + "}\n"; }
    if (check(TokenType::KEYWORD_ELIF)) { advance(); std::string cond = parseExpression(); expect(TokenType::COLON, "Expected ':' after 'elif' condition"); if (check(TokenType::NEWLINE)) advance(); return "else if (" + cond + ") {\n" + parseBlock() + "}\n"; }
    if (check(TokenType::KEYWORD_ELSE)) { advance(); expect(TokenType::COLON, "Expected ':' after 'else'"); if (check(TokenType::NEWLINE)) advance(); return "else {\n" + parseBlock() + "}\n"; }
    if (check(TokenType::KEYWORD_WHILE)) { advance(); std::string cond = parseExpression(); expect(TokenType::COLON, "Expected ':' after 'while' condition"); if (check(TokenType::NEWLINE)) advance(); return "while (" + cond + ") {\n" + parseBlock() + "}\n"; }

    if (check(TokenType::KEYWORD_FOR)) {
        advance(); std::string var = advance().value; 
        expect(TokenType::KEYWORD_IN, "Expected 'in' in for loop");
        if (check(TokenType::IDENTIFIER) && peek().value == "range") {
            advance(); // range
            expect(TokenType::LPAREN, "Expected '(' after range");
            variables[var] = "int"; // loop vars are explicitly int
            std::string start = parseExpressionUntil(TokenType::COMMA, TokenType::COMMA);
            expect(TokenType::COMMA, "Expected ',' to separate range arguments");
            std::string end = parseExpressionUntil(TokenType::RPAREN, TokenType::RPAREN);
            expect(TokenType::RPAREN, "Expected ')' after range arguments");
            expect(TokenType::COLON, "Expected ':' after for loop");
            if (check(TokenType::NEWLINE)) advance();
            return "for (int " + var + " = " + start + "; " + var + " < " + end + "; ++" + var + ") {\n" + parseBlock() + "}\n";
        } else {
            variables[var] = "np_var"; // loop var is np_var by default for collections
            std::string coll = parseExpressionUntil(TokenType::COLON, TokenType::COLON);
            expect(TokenType::COLON, "Expected ':' after for loop");
            if (check(TokenType::NEWLINE)) advance();
            return "for (const np_var& " + var + " : " + coll + ") {\n" + parseBlock() + "}\n";
        }
    }

    if (check(TokenType::KEYWORD_RETURN)) {
        advance();
        std::string expr = parseExpression();
        if (expr.find(",") != std::string::npos) return "return {" + expr + "};\n";
        return "return " + expr + ";\n";
    }

    if (check(TokenType::KEYWORD_TYPE)) {
        std::string type = advance().value;
        std::string np_type = type;
        if (type == "int") type = "int64_t";
        else if (type == "int32") type = "int32_t";
        else if (type == "int64") type = "int64_t";
        else if (type == "int128") type = "np_int128";
        else if (type == "int256") type = "np_int256";
        else if (type == "float") type = "double";
        else if (type == "float32") type = "float";
        else if (type == "float64") type = "double";
        else if (type == "string") type = "np_string";
        else if (type == "array" || type == "dict") type = "np_var";
        std::string name = advance().value;
        variables[name] = np_type; // Track variable type
        
        std::string arr_size = "";
        if (np_type == "array" && check(TokenType::LBRACKET)) {
            advance(); // [
            arr_size = parseExpressionUntil(TokenType::RBRACKET, TokenType::RBRACKET);
            if (check(TokenType::RBRACKET)) advance(); // ]
        }

        if (check(TokenType::COMMA)) {
            std::string vars = name;
            while (check(TokenType::COMMA)) {
                advance(); // ,
                if (check(TokenType::KEYWORD_TYPE)) advance(); // type
                std::string next_name = advance().value;
                variables[next_name] = np_type;
                vars += ", " + next_name;
            }
            if (check(TokenType::ASSIGN)) advance();
            return "auto [" + vars + "] = " + parseExpression() + ";\n";
        }
        
        if (check(TokenType::ASSIGN)) {
            advance(); return type + " " + name + " = " + parseExpression() + ";\n";
        }
        
        if (np_type == "array" && !arr_size.empty()) return type + " " + name + " = std::vector<np_var>(" + arr_size + ");\n";
        if (np_type == "array") return type + " " + name + " = std::vector<np_var>{};\n";
        if (np_type == "dict") return type + " " + name + " = std::map<std::string, np_var>{};\n";
        return type + " " + name + ";\n";
    }
    
    std::string expr = parseExpression();
    if (!expr.empty()) return expr + ";\n";
    return "";
}