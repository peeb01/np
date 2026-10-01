#include "parser.hpp"
#include "lexer.hpp"
#include <iostream>

std::unique_ptr<ExprAST> Parser::parseExpression() {
    return parseExpression(0);
}

std::unique_ptr<ExprAST> Parser::parseExpression(int min_precedence) {
    auto lhs = parsePrimary();
    if (!lhs) return nullptr;

    while (true) {
        if (isAtEnd()) break;
        
        Token op_token = peek();
        if (op_token.type != TokenType::OPERATOR && 
            op_token.type != TokenType::KEYWORD_AND && 
            op_token.type != TokenType::KEYWORD_OR &&
            op_token.type != TokenType::BIT_AND &&
            op_token.type != TokenType::BIT_OR &&
            op_token.type != TokenType::BIT_XOR &&
            op_token.type != TokenType::LSHIFT &&
            op_token.type != TokenType::RSHIFT) {
            break;
        }
        
        std::string op = op_token.value;
        int prec = getPrecedence(op);
        if (prec < min_precedence) break;
        
        advance(); // consume op
        
        int next_min_prec = (op == "^") ? prec : prec + 1;
        auto rhs = parseExpression(next_min_prec);
        if (!rhs) {
            std::cerr << "Syntax Error on line " << op_token.line << ": Expected expression after operator '" << op << "'\n";
            exit(1);
        }
        
        lhs = std::make_unique<BinaryExprAST>(op, std::move(lhs), std::move(rhs));
    }
    return lhs;
}

std::unique_ptr<ExprAST> Parser::parsePrimary() {
    Token t = peek();
    
    // Prefix unary operators: not, -, +, ~
    if (t.type == TokenType::KEYWORD_NOT || t.type == TokenType::BIT_XOR || (t.type == TokenType::OPERATOR && (t.value == "-" || t.value == "+"))) {
        advance();
        std::string op = t.value;
        auto operand = parseExpression(70); // unary precedence
        if (!operand) {
            std::cerr << "Syntax Error on line " << t.line << ": Expected expression after prefix operator\n";
            exit(1);
        }
        return std::make_unique<BinaryExprAST>(op, std::make_unique<NumberExprAST>("0", false), std::move(operand));
    }
    
    // Nil literal
    if (t.type == TokenType::KEYWORD_NIL) {
        advance();
        return parsePostFix(std::make_unique<NilExprAST>());
    }
    
    // Parentheses grouping
    if (t.type == TokenType::LPAREN) {
        advance();
        auto expr = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after expression");
        return parsePostFix(std::move(expr));
    }
    
    // Type conversion call, e.g., int(x), float(y), string(z)
    if (t.type == TokenType::KEYWORD_TYPE) {
        advance();
        expect(TokenType::LPAREN, "Expected '(' after type name for conversion");
        auto arg = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after conversion argument");
        
        std::vector<std::unique_ptr<ExprAST>> args;
        args.push_back(std::move(arg));
        std::unique_ptr<ExprAST> expr = std::make_unique<CallExprAST>(t.value, std::move(args));
        return parsePostFix(std::move(expr));
    }

    // Literals
    if (t.type == TokenType::INT_LITERAL) {
        advance();
        return parsePostFix(std::make_unique<NumberExprAST>(t.value, false));
    }
    if (t.type == TokenType::FLOAT_LITERAL) {
        advance();
        return parsePostFix(std::make_unique<NumberExprAST>(t.value, true));
    }
    if (t.type == TokenType::STRING_LITERAL) {
        advance();
        return parsePostFix(std::make_unique<StringExprAST>(t.value));
    }
    if (t.type == TokenType::FSTRING_LITERAL) {
        advance();
        std::string raw = t.value;
        std::vector<std::unique_ptr<ExprAST>> parts;
        size_t i = 0;
        while (i < raw.length()) {
            if (raw[i] == '{') {
                if (i + 1 < raw.length() && raw[i + 1] == '{') {
                    parts.push_back(std::make_unique<StringExprAST>("{"));
                    i += 2;
                    continue;
                }
                size_t close_pos = raw.find('}', i + 1);
                if (close_pos == std::string::npos) {
                    std::cerr << "Syntax Error: Unclosed '{' in f-string\n";
                    exit(1);
                }
                std::string expr_str = raw.substr(i + 1, close_pos - (i + 1));
                Lexer sub_lexer(expr_str);
                auto sub_tokens = sub_lexer.tokenize();
                Parser sub_parser(sub_tokens);
                sub_parser.variables = variables;
                sub_parser.local_variables = local_variables;
                sub_parser.struct_methods = struct_methods;
                sub_parser.enums = enums;
                sub_parser.structs = structs;
                sub_parser.import_aliases = import_aliases;
                auto sub_expr = sub_parser.parseExpression();
                parts.push_back(std::move(sub_expr));
                i = close_pos + 1;
            } else {
                size_t next_open = raw.find('{', i);
                std::string text = (next_open == std::string::npos) ? raw.substr(i) : raw.substr(i, next_open - i);
                parts.push_back(std::make_unique<StringExprAST>(text));
                i = (next_open == std::string::npos) ? raw.length() : next_open;
            }
        }
        if (parts.empty()) {
            return parsePostFix(std::make_unique<StringExprAST>(""));
        }
        std::unique_ptr<ExprAST> combined = std::move(parts[0]);
        if (combined->getType() != ASTNodeType::STRING_LITERAL) {
            combined = std::make_unique<BinaryExprAST>("+", std::make_unique<StringExprAST>(""), std::move(combined));
        }
        for (size_t p = 1; p < parts.size(); ++p) {
            combined = std::make_unique<BinaryExprAST>("+", std::move(combined), std::move(parts[p]));
        }
        return parsePostFix(std::move(combined));
    }
    if (t.type == TokenType::BOOL_LITERAL) {
        advance();
        return parsePostFix(std::make_unique<BoolExprAST>(t.value == "true"));
    }

    // Channel receive: <-ch
    if (t.type == TokenType::LARROW) {
        advance(); // <-
        auto ch = parsePrimary();
        std::vector<std::unique_ptr<ExprAST>> recvArgs;
        recvArgs.push_back(std::move(ch));
        return parsePostFix(std::make_unique<CallExprAST>("chan_recv", std::move(recvArgs)));
    }
    
    // List literal or List comprehension
    if (t.type == TokenType::LBRACKET) {
        advance(); // consume [
        
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
            auto comp_expr = parseExpression();
            expect(TokenType::KEYWORD_FOR, "Expected 'for' in list comprehension");
            Token var_tok = advance();
            if (var_tok.type != TokenType::IDENTIFIER) {
                std::cerr << "Syntax Error on line " << var_tok.line << ": Expected identifier after 'for'\n";
                exit(1);
            }
            std::string var_name = var_tok.value;
            expect(TokenType::KEYWORD_IN, "Expected 'in' in list comprehension");
            
            std::unique_ptr<ListCompExprAST> comp_node;
            if (check(TokenType::IDENTIFIER) && peek().value == "range") {
                advance(); // range
                expect(TokenType::LPAREN, "Expected '(' after range");
                auto start = parseExpression();
                expect(TokenType::COMMA, "Expected ',' in range");
                auto end = parseExpression();
                expect(TokenType::RPAREN, "Expected ')' after range");
                
                std::unique_ptr<ExprAST> cond = nullptr;
                if (check(TokenType::KEYWORD_IF)) {
                    advance(); // if
                    cond = parseExpression();
                }
                expect(TokenType::RBRACKET, "Expected ']' at end of list comprehension");
                comp_node = std::make_unique<ListCompExprAST>(std::move(comp_expr), var_name, std::move(start), std::move(end), std::move(cond));
            } else {
                auto coll = parseExpression();
                std::unique_ptr<ExprAST> cond = nullptr;
                if (check(TokenType::KEYWORD_IF)) {
                    advance(); // if
                    cond = parseExpression();
                }
                expect(TokenType::RBRACKET, "Expected ']' at end of list comprehension");
                comp_node = std::make_unique<ListCompExprAST>(std::move(comp_expr), var_name, std::move(coll), std::move(cond));
            }
            return parsePostFix(std::move(comp_node));
        } else {
            // Regular List Literal: [a, b, c]
            std::vector<std::unique_ptr<ExprAST>> elements;
            while (!check(TokenType::RBRACKET) && !isAtEnd()) {
                elements.push_back(parseExpression());
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RBRACKET, "Expected ']' at end of list literal");
            return parsePostFix(std::make_unique<ListExprAST>(std::move(elements)));
        }
    }
    
    // Dict literal: {k: v, k2: v2}
    if (t.type == TokenType::LBRACE) {
        advance(); // consume {
        std::vector<std::pair<std::unique_ptr<ExprAST>, std::unique_ptr<ExprAST>>> key_values;
        while (!check(TokenType::RBRACE) && !isAtEnd()) {
            auto key = parseExpression();
            expect(TokenType::COLON, "Expected ':' in dictionary");
            auto val = parseExpression();
            key_values.push_back({std::move(key), std::move(val)});
            if (check(TokenType::COMMA)) advance();
        }
        expect(TokenType::RBRACE, "Expected '}' at end of dict literal");
        return parsePostFix(std::make_unique<DictExprAST>(std::move(key_values)));
    }
    
    // Identifier
    if (t.type == TokenType::IDENTIFIER) {
        advance();
        std::unique_ptr<ExprAST> expr;
        
        std::string identifier_name = t.value;
        if (!namespace_prefix.empty() && !isBuiltIn(identifier_name) && 
            local_variables.count(identifier_name) == 0 && 
            import_aliases.count(identifier_name) == 0) {
            identifier_name = namespace_prefix + "_" + identifier_name;
        }
        
        if (check(TokenType::LPAREN)) {
            advance(); // consume (
            std::vector<std::unique_ptr<ExprAST>> args;
            while (!check(TokenType::RPAREN) && !isAtEnd()) {
                if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::ASSIGN) {
                    std::string kw = advance().value;
                    advance(); // consume =
                    auto val = parseExpression();
                    args.push_back(std::make_unique<NamedArgExprAST>(kw, std::move(val)));
                } else {
                    args.push_back(parseExpression());
                }
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN, "Expected ')' after function arguments");
            expr = std::make_unique<CallExprAST>(identifier_name, std::move(args));
        } else {
            expr = std::make_unique<VariableExprAST>(identifier_name);
        }
        return parsePostFix(std::move(expr));
    }
    
    std::cerr << "Syntax Error on line " << t.line << ": Unexpected token '" << t.value << "' in expression\n";
    exit(1);
    return nullptr;
}

std::unique_ptr<ExprAST> Parser::parsePostFix(std::unique_ptr<ExprAST> expr) {
    while (true) {
        if (check(TokenType::DOT)) {
            advance();
            Token member = advance();
            if (member.type != TokenType::IDENTIFIER) {
                std::cerr << "Syntax Error on line " << member.line << ": Expected member identifier after '.'\n";
                exit(1);
            }
            
            // Check if LHS is an Enum! e.g. Role.ADMIN
            if (expr->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto var_expr = static_cast<VariableExprAST*>(expr.get());
                if (enums.count(var_expr->name) && enums[var_expr->name].count(member.value)) {
                    int enum_val = enums[var_expr->name][member.value];
                    expr = std::make_unique<NumberExprAST>(std::to_string(enum_val), false);
                    continue;
                }
            }

            // Check if LHS is a namespace/import alias
            if (expr->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto var_expr = static_cast<VariableExprAST*>(expr.get());
                if (import_aliases.count(var_expr->name) > 0) {
                    if (check(TokenType::LPAREN)) {
                        advance(); // consume (
                        std::vector<std::unique_ptr<ExprAST>> args;
                        while (!check(TokenType::RPAREN) && !isAtEnd()) {
                            if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::ASSIGN) {
                                std::string kw = advance().value;
                                advance(); // consume =
                                auto val = parseExpression();
                                args.push_back(std::make_unique<NamedArgExprAST>(kw, std::move(val)));
                            } else {
                                args.push_back(parseExpression());
                            }
                            if (check(TokenType::COMMA)) advance();
                        }
                        expect(TokenType::RPAREN, "Expected ')' after function arguments");
                        expr = std::make_unique<CallExprAST>(var_expr->name + "_" + member.value, std::move(args));
                    } else {
                        expr = std::make_unique<VariableExprAST>(var_expr->name + "_" + member.value);
                    }
                    continue;
                }
            }
            
            if (check(TokenType::LPAREN)) {
                advance(); // consume (
                std::vector<std::unique_ptr<ExprAST>> args;
                std::string struct_type = "";
                if (expr->getType() == ASTNodeType::VARIABLE_EXPR) {
                    auto var_expr = static_cast<VariableExprAST*>(expr.get());
                    if (variables.count(var_expr->name)) {
                        struct_type = variables[var_expr->name];
                    }
                }
                args.push_back(std::move(expr));
                while (!check(TokenType::RPAREN) && !isAtEnd()) {
                    args.push_back(parseExpression());
                    if (check(TokenType::COMMA)) advance();
                }
                expect(TokenType::RPAREN, "Expected ')' after method arguments");

                if (!struct_type.empty() && struct_methods.count(struct_type) && struct_methods[struct_type].count(member.value)) {
                    expr = std::make_unique<CallExprAST>(struct_type + "_" + member.value, std::move(args));
                } else if (!struct_type.empty() && interfaces.count(struct_type) && interfaces[struct_type].count(member.value)) {
                    expr = std::make_unique<CallExprAST>("@interface_" + struct_type + "_" + member.value, std::move(args));
                } else {
                    expr = std::make_unique<CallExprAST>("@method_" + member.value, std::move(args));
                }
            } else {
                expr = std::make_unique<DotAccessExprAST>(std::move(expr), member.value);
            }
        } else if (check(TokenType::LBRACKET)) {
            // Check if this is a generic function call: func_name[T](args...)
            if (expr->getType() == ASTNodeType::VARIABLE_EXPR) {
                size_t scan_pos = pos + 1; // token after '['
                int bracket_depth = 1;
                bool is_generic_call = false;
                while (scan_pos < tokens.size()) {
                    if (tokens[scan_pos].type == TokenType::LBRACKET) bracket_depth++;
                    else if (tokens[scan_pos].type == TokenType::RBRACKET) {
                        bracket_depth--;
                        if (bracket_depth == 0) {
                            if (scan_pos + 1 < tokens.size() && tokens[scan_pos + 1].type == TokenType::LPAREN) {
                                is_generic_call = true;
                            }
                            break;
                        }
                    }
                    scan_pos++;
                }
                if (is_generic_call) {
                    advance(); // consume [
                    while (!check(TokenType::RBRACKET) && !isAtEnd()) {
                        advance(); // consume type token(s)
                    }
                    expect(TokenType::RBRACKET, "Expected ']' after generic type arguments");
                    expect(TokenType::LPAREN, "Expected '(' after generic function call");
                    std::vector<std::unique_ptr<ExprAST>> args;
                    while (!check(TokenType::RPAREN) && !isAtEnd()) {
                        args.push_back(parseExpression());
                        if (check(TokenType::COMMA)) advance();
                    }
                    expect(TokenType::RPAREN, "Expected ')' after function arguments");
                    auto var_expr = static_cast<VariableExprAST*>(expr.get());
                    expr = std::make_unique<CallExprAST>(var_expr->name, std::move(args));
                    continue;
                }
            }
            advance(); // consume [
            
            std::unique_ptr<ExprAST> start = nullptr;
            std::unique_ptr<ExprAST> end = nullptr;
            if (check(TokenType::COLON)) {
                advance(); // consume :
                if (!check(TokenType::RBRACKET)) {
                    end = parseExpression();
                }
            } else {
                auto first_expr = parseExpression();
                if (check(TokenType::COLON)) {
                    advance(); // consume :
                    start = std::move(first_expr);
                    if (!check(TokenType::RBRACKET)) {
                        end = parseExpression();
                    }
                } else {
                    expect(TokenType::RBRACKET, "Expected ']' after index");
                    expr = std::make_unique<IndexAccessExprAST>(std::move(expr), std::move(first_expr));
                    continue;
                }
            }
            
            expect(TokenType::RBRACKET, "Expected ']' after slice");
            expr = std::make_unique<SliceExprAST>(std::move(expr), std::move(start), std::move(end));
        } else if (check(TokenType::LPAREN)) {
            advance(); // consume (
            std::vector<std::unique_ptr<ExprAST>> args;
            while (!check(TokenType::RPAREN) && !isAtEnd()) {
                args.push_back(parseExpression());
                if (check(TokenType::COMMA)) advance();
            }
            expect(TokenType::RPAREN, "Expected ')' after function arguments");
            expr = std::make_unique<CallExprAST>(std::move(expr), std::move(args));
        } else {
            break;
        }
    }
    return expr;
}
