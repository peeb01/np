#include "../include/parser.hpp"
#include <iostream>

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
    return global_code + "\nint main() {\n" + translated_code + "\nreturn 0;\n}\n";
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

        if (t.type == TokenType::IDENTIFIER && t.value == "input") {
            if (check(TokenType::LPAREN)) { advance(); if (check(TokenType::RPAREN)) advance(); }
            expr += "np_input()"; prev_token = t; continue;
        }
        if (t.type == TokenType::IDENTIFIER && t.value == "print") { expr += "np_print"; prev_token = t; continue; }
        if (t.type == TokenType::IDENTIFIER && t.value == "type") { expr += "np_type"; prev_token = t; continue; }

        // Auto-binding for Array properties
        if (t.type == TokenType::IDENTIFIER && (t.value == "length" || t.value == "shape")) {
            if (prev_token.type == TokenType::DOT) {
                expr += t.value;
                if (!check(TokenType::LPAREN)) expr += "()";
                prev_token = t;
                continue;
            }
        }

        // Array Literal & Indexing Tracker
        if (t.type == TokenType::LBRACKET) {
            nesting++;
            
            Token prev_actual = (pos >= 2) ? tokens[pos - 2] : Token{TokenType::EOF_TOKEN, "", 0};
            if (prev_actual.type == TokenType::IDENTIFIER || 
                prev_actual.type == TokenType::RBRACKET || 
                prev_actual.type == TokenType::RPAREN || 
                prev_actual.type == TokenType::RBRACE) {
                expr += "[";
                is_index_stack.push_back(true);
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
        
        // Dictionary Dictionary Tracker
        if (t.type == TokenType::LBRACE) {
            nesting++;
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
        } else {
            if (t.type == TokenType::STRING_LITERAL) expr += "\"" + t.value + "\"";
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
                else if (type == "float") type = "double";
                else if (type == "float32") type = "float";
                else if (type == "float64") type = "double";
                else if (type == "string") type = "std::string";
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
                    else if (t == "float") t = "double";
                    else if (t == "float32") t = "float";
                    else if (t == "float64") t = "double";
                    else if (t == "string") t = "std::string";
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

    if (check(TokenType::KEYWORD_IF)) { advance(); std::string cond = parseExpression(); expect(TokenType::COLON, "Expected ':' after 'if' condition"); if (check(TokenType::NEWLINE)) advance(); return "if (" + cond + ") {\n" + parseBlock() + "}\n"; }
    if (check(TokenType::KEYWORD_ELIF)) { advance(); std::string cond = parseExpression(); expect(TokenType::COLON, "Expected ':' after 'elif' condition"); if (check(TokenType::NEWLINE)) advance(); return "else if (" + cond + ") {\n" + parseBlock() + "}\n"; }
    if (check(TokenType::KEYWORD_ELSE)) { advance(); expect(TokenType::COLON, "Expected ':' after 'else'"); if (check(TokenType::NEWLINE)) advance(); return "else {\n" + parseBlock() + "}\n"; }
    if (check(TokenType::KEYWORD_WHILE)) { advance(); std::string cond = parseExpression(); expect(TokenType::COLON, "Expected ':' after 'while' condition"); if (check(TokenType::NEWLINE)) advance(); return "while (" + cond + ") {\n" + parseBlock() + "}\n"; }

    if (check(TokenType::KEYWORD_FOR)) {
        advance(); std::string var = advance().value; 
        variables[var] = "int"; // loop vars are explicitly int
        expect(TokenType::KEYWORD_IN, "Expected 'in' in for loop");
        Token r = advance(); if (r.value != "range") { std::cerr << "Syntax Error on line " << r.line << ": Expected 'range'\n"; exit(1); }
        expect(TokenType::LPAREN, "Expected '(' after range");
        std::string start = parseExpressionUntil(TokenType::COMMA, TokenType::COMMA);
        expect(TokenType::COMMA, "Expected ',' to separate range arguments");
        std::string end = parseExpressionUntil(TokenType::RPAREN, TokenType::RPAREN);
        expect(TokenType::RPAREN, "Expected ')' after range arguments");
        expect(TokenType::COLON, "Expected ':' after for loop");
        if (check(TokenType::NEWLINE)) advance();
        return "for (int " + var + " = " + start + "; " + var + " < " + end + "; ++" + var + ") {\n" + parseBlock() + "}\n";
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
        else if (type == "float") type = "double";
        else if (type == "float32") type = "float";
        else if (type == "float64") type = "double";
        else if (type == "string") type = "std::string";
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