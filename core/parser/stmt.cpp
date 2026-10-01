#include "parser.hpp"
#include <iostream>

std::unique_ptr<BlockStmtAST> Parser::parseBlock() {
    std::vector<std::unique_ptr<ASTNode>> statements;
    while (check(TokenType::NEWLINE)) advance();
    if (check(TokenType::INDENT)) {
        advance();
        while (!check(TokenType::DEDENT) && !isAtEnd()) {
            if (check(TokenType::NEWLINE)) { advance(); continue; }
            auto stmt = parseStatement();
            if (stmt) statements.push_back(std::move(stmt));
        }
        if (check(TokenType::DEDENT)) advance();
    }
    return std::make_unique<BlockStmtAST>(std::move(statements));
}

std::string Parser::parseType() {
    bool is_pointer = false;
    if (check(TokenType::OPERATOR) && peek().value == "*") {
        advance(); // *
        is_pointer = true;
    }

    std::string type_name = "";
    if (check(TokenType::LPAREN)) {
        advance(); // (
        type_name = "(";
        bool first = true;
        while (!check(TokenType::RPAREN) && !isAtEnd()) {
            if (!first) {
                if (check(TokenType::COMMA)) advance();
                type_name += ",";
            }
            type_name += parseType();
            first = false;
        }
        expect(TokenType::RPAREN, "Expected ')' in type");
        type_name += ")";
        if (is_pointer) type_name = "*" + type_name;
        return type_name;
    }

    if (check(TokenType::KEYWORD_FN)) {
        type_name += advance().value; // fn / func / function
        if (check(TokenType::LPAREN)) {
            type_name += "(";
            advance(); // (
            bool first = true;
            while (!check(TokenType::RPAREN) && !isAtEnd()) {
                if (!first) {
                    if (check(TokenType::COMMA)) advance();
                    type_name += ",";
                }
                type_name += parseType();
                first = false;
            }
            expect(TokenType::RPAREN, "Expected ')' in function type");
            type_name += ")";
        }
        if (check(TokenType::ARROW)) {
            advance(); // ->
            type_name += "->" + parseType();
        }
        if (is_pointer) type_name = "*" + type_name;
        return type_name;
    }
    
    if (check(TokenType::KEYWORD_TYPE)) {
        type_name = advance().value;
    } else if (check(TokenType::IDENTIFIER)) {
        std::string id = advance().value;
        if (check(TokenType::DOT)) {
            advance(); // .
            Token member_tok = peek();
            expect(TokenType::IDENTIFIER, "Expected member identifier after '.' in type");
            type_name = id + "_" + member_tok.value;
        } else {
            if (!namespace_prefix.empty() && !isBuiltIn(id)) {
                type_name = namespace_prefix + "_" + id;
            } else {
                type_name = id;
            }
        }
    }
    if (is_pointer) type_name = "*" + type_name;
    return type_name;
}

std::unique_ptr<StmtAST> Parser::parseBreak() {
    advance(); // break
    if (check(TokenType::NEWLINE)) advance();
    return std::make_unique<BreakStmtAST>();
}

std::unique_ptr<StmtAST> Parser::parseContinue() {
    advance(); // continue
    if (check(TokenType::NEWLINE)) advance();
    return std::make_unique<ContinueStmtAST>();
}

std::unique_ptr<StmtAST> Parser::parseDefer() {
    advance(); // defer
    auto stmt = parseStatement();
    return std::make_unique<DeferStmtAST>(std::move(stmt));
}

std::unique_ptr<StmtAST> Parser::parseGo() {
    advance(); // go
    auto expr = parseExpression();
    if (check(TokenType::NEWLINE)) advance();
    return std::make_unique<GoStmtAST>(std::move(expr));
}
    
std::unique_ptr<StmtAST> Parser::parseFunction(bool is_kernel) {
    if (check(TokenType::KEYWORD_FN)) {
        advance(); // fn, func, function
    }
    
    std::string receiver_name = "";
    std::string receiver_type = "";
    bool is_method = false;

    if (check(TokenType::LPAREN)) {
        is_method = true;
        advance(); // (
        bool pointer_first = false;
        if (check(TokenType::OPERATOR) && peek().value == "*") {
            advance(); // *
            pointer_first = true;
        }
        std::string first_id = advance().value;
        if (pointer_first) {
            first_id = "*" + first_id;
        }
        if (check(TokenType::COLON)) {
            advance(); // :
            receiver_name = first_id;
            receiver_type = parseType();
        } else if (check(TokenType::OPERATOR) && peek().value == "*") {
            advance(); // *
            std::string type_id = advance().value;
            receiver_name = first_id;
            receiver_type = "*" + type_id;
        } else if (check(TokenType::IDENTIFIER) || check(TokenType::KEYWORD_TYPE)) {
            std::string second_id = advance().value;
            if (structs.count(first_id) || (first_id[0] >= 'A' && first_id[0] <= 'Z') || (first_id[0] == '*')) {
                receiver_type = first_id;
                receiver_name = second_id;
            } else {
                receiver_name = first_id;
                receiver_type = second_id;
            }
        } else {
            receiver_name = "self";
            receiver_type = first_id;
        }
        expect(TokenType::RPAREN, "Expected ')' after receiver in method declaration");
    }

    std::string name = advance().value;
    std::vector<std::string> type_params;
    if (check(TokenType::LBRACKET)) {
        advance(); // consume [
        while (!check(TokenType::RBRACKET) && !isAtEnd()) {
            if (check(TokenType::IDENTIFIER)) {
                type_params.push_back(advance().value);
            }
            if (check(TokenType::COMMA)) advance();
        }
        expect(TokenType::RBRACKET, "Expected ']' after generic type parameters");
    }
    std::string base_struct = receiver_type;
    if (!base_struct.empty() && base_struct[0] == '*') {
        base_struct = base_struct.substr(1);
    }
    if (is_method) {
        struct_methods[base_struct].insert(name);
        name = base_struct + "_" + name;
    } else if (!namespace_prefix.empty()) {
        name = namespace_prefix + "_" + name;
    }
    
    is_inside_function = true;
    local_variables.clear();
    
    std::vector<FuncParam> params;
    if (is_method) {
        params.push_back({receiver_type, receiver_name});
        variables[receiver_name] = base_struct;
        local_variables.insert(receiver_name);
    }

    expect(TokenType::LPAREN, "Expected '(' after function name");
    while (!check(TokenType::RPAREN) && !isAtEnd()) {
        std::string param_name = "";
        std::string type_name = "";
        if (check(TokenType::KEYWORD_TYPE) || check(TokenType::KEYWORD_FN)) {
            type_name = parseType();
            param_name = advance().value;
        } else if (check(TokenType::IDENTIFIER)) {
            std::string id = advance().value;
            bool is_type_param = false;
            for (const auto& tp : type_params) {
                if (tp == id) { is_type_param = true; break; }
            }
            if (is_type_param) {
                type_name = id;
                param_name = advance().value;
            } else if (check(TokenType::COLON)) {
                advance(); // :
                param_name = id;
                type_name = parseType();
            } else if (check(TokenType::DOT)) {
                advance(); // .
                Token member_tok = peek();
                expect(TokenType::IDENTIFIER, "Expected member identifier after '.' in parameter type");
                std::string member = member_tok.value;
                type_name = id + "_" + member;
                param_name = advance().value;
            } else {
                if (!check(TokenType::COMMA) && !check(TokenType::RPAREN)) {
                    type_name = id;
                    param_name = advance().value;
                } else {
                    param_name = id;
                    type_name = "auto";
                }
            }
        } else {
            std::cerr << "Syntax Error on line " << peek().line << ": Unexpected token in function parameters\n";
            exit(1);
        }
        params.push_back({type_name, param_name});
        variables[param_name] = type_name;
        local_variables.insert(param_name);
        if (check(TokenType::COMMA)) advance();
    }
    expect(TokenType::RPAREN, "Expected ')' after parameters");
    
    std::string ret_type = "void";
    if (check(TokenType::ARROW)) {
        advance(); // ->
        ret_type = parseType();
        while (check(TokenType::COMMA)) {
            advance(); // ,
            ret_type += "," + parseType();
        }
    }
    expect(TokenType::COLON, "Expected ':' before function block");
    if (check(TokenType::NEWLINE)) advance();
    auto body = parseBlock();
    
    is_inside_function = false;
    local_variables.clear();
    
    return std::make_unique<FuncDeclStmtAST>(name, std::move(params), ret_type, std::move(body), is_kernel);
}

std::unique_ptr<StmtAST> Parser::parseStruct() {
    advance(); // struct
    std::string struct_name = advance().value;
    if (!namespace_prefix.empty()) {
        struct_name = namespace_prefix + "_" + struct_name;
    }
    expect(TokenType::COLON, "Expected ':' after struct name");
    if (check(TokenType::NEWLINE)) advance();
    expect(TokenType::INDENT, "Expected indentation for struct body");
    std::vector<StructField> fields;
    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        if (check(TokenType::NEWLINE)) { advance(); continue; }
        
        std::string type_name = "";
        bool has_type = false;
        if (check(TokenType::KEYWORD_TYPE)) {
            type_name = advance().value;
            has_type = true;
        } else if (check(TokenType::IDENTIFIER)) {
            std::string id = advance().value;
            if (check(TokenType::DOT)) {
                advance(); // .
                Token member_tok = peek();
                expect(TokenType::IDENTIFIER, "Expected member identifier after '.' in struct field type");
                std::string member = member_tok.value;
                type_name = id + "_" + member;
            } else {
                if (!namespace_prefix.empty() && !isBuiltIn(id)) {
                    type_name = namespace_prefix + "_" + id;
                } else {
                    type_name = id;
                }
            }
            has_type = true;
        }
        
        if (has_type) {
            std::string field_name = advance().value;
            fields.push_back({type_name, field_name});
            if (check(TokenType::NEWLINE)) advance();
        } else {
            std::cerr << "Syntax Error on line " << peek().line << ": Expected field definition in struct\n";
            exit(1);
        }
    }
    if (check(TokenType::DEDENT)) advance();
    structs[struct_name] = {};
    for (const auto& f : fields) {
        structs[struct_name].push_back({f.name, f.type});
    }
    return std::make_unique<StructDeclStmtAST>(struct_name, std::move(fields));
}

std::unique_ptr<StmtAST> Parser::parseTryExcept() {
    advance(); // try
    expect(TokenType::COLON, "Expected ':' after try");
    if (check(TokenType::NEWLINE)) advance();
    auto try_b = parseBlock();
    
    expect(TokenType::KEYWORD_EXCEPT, "Expected 'except' block after try");
    expect(TokenType::IDENTIFIER, "Expected Exception class name");
    std::string exception_var = advance().value;
    expect(TokenType::COLON, "Expected ':' after except");
    if (check(TokenType::NEWLINE)) advance();
    auto except_b = parseBlock();
    return std::make_unique<TryExceptStmtAST>(std::move(try_b), exception_var, std::move(except_b));
}

std::unique_ptr<StmtAST> Parser::parseThrow() {
    advance(); // throw
    auto expr = parseExpression();
    if (check(TokenType::NEWLINE)) advance();
    return std::make_unique<ThrowStmtAST>(std::move(expr));
}

std::unique_ptr<StmtAST> Parser::parseIf() {
    advance(); // if
    auto cond = parseExpression();
    expect(TokenType::COLON, "Expected ':' after 'if' condition");
    if (check(TokenType::NEWLINE)) advance();
    auto block = parseBlock();
    
    std::vector<IfCondBlock> cases;
    cases.push_back({std::move(cond), std::move(block)});
    
    std::unique_ptr<BlockStmtAST> else_block = nullptr;
    while (true) {
        if (check(TokenType::KEYWORD_ELIF)) {
            advance(); // elif
            auto elif_cond = parseExpression();
            expect(TokenType::COLON, "Expected ':' after 'elif' condition");
            if (check(TokenType::NEWLINE)) advance();
            auto elif_block = parseBlock();
            cases.push_back({std::move(elif_cond), std::move(elif_block)});
        } else if (check(TokenType::KEYWORD_ELSE)) {
            advance(); // else
            expect(TokenType::COLON, "Expected ':' after 'else'");
            if (check(TokenType::NEWLINE)) advance();
            else_block = parseBlock();
            break;
        } else {
            break;
        }
    }
    return std::make_unique<IfStmtAST>(std::move(cases), std::move(else_block));
}

std::unique_ptr<StmtAST> Parser::parseWhile() {
    advance(); // while
    auto cond = parseExpression();
    expect(TokenType::COLON, "Expected ':' after 'while' condition");
    if (check(TokenType::NEWLINE)) advance();
    auto body = parseBlock();
    return std::make_unique<WhileStmtAST>(std::move(cond), std::move(body));
}

std::unique_ptr<StmtAST> Parser::parseFor() {
    advance(); // for
    Token var_tok = advance();
    if (var_tok.type != TokenType::IDENTIFIER) {
        std::cerr << "Syntax Error on line " << var_tok.line << ": Expected identifier for loop variable\n";
        exit(1);
    }
    std::string var_name = var_tok.value;
    if (is_inside_function) {
        local_variables.insert(var_name);
    } else if (!namespace_prefix.empty()) {
        var_name = namespace_prefix + "_" + var_name;
    }
    expect(TokenType::KEYWORD_IN, "Expected 'in' in for loop");
    
    std::unique_ptr<ForStmtAST> for_node;
    if (check(TokenType::IDENTIFIER) && peek().value == "range") {
        advance(); // range
        expect(TokenType::LPAREN, "Expected '(' after range");
        variables[var_name] = "int";
        auto start = parseExpression();
        expect(TokenType::COMMA, "Expected ',' in range");
        auto end = parseExpression();
        expect(TokenType::RPAREN, "Expected ')' after range");
        expect(TokenType::COLON, "Expected ':' after for loop");
        if (check(TokenType::NEWLINE)) advance();
        auto body = parseBlock();
        for_node = std::make_unique<ForStmtAST>(var_name, std::move(start), std::move(end), std::move(body));
    } else {
        variables[var_name] = "np_var";
        auto coll = parseExpression();
        expect(TokenType::COLON, "Expected ':' after for loop");
        if (check(TokenType::NEWLINE)) advance();
        auto body = parseBlock();
        for_node = std::make_unique<ForStmtAST>(var_name, std::move(coll), std::move(body));
    }
    return for_node;
}

std::unique_ptr<StmtAST> Parser::parseReturn() {
    advance(); // return
    std::unique_ptr<ExprAST> expr = nullptr;
    if (!check(TokenType::NEWLINE)) {
        expr = parseExpression();
        if (check(TokenType::COMMA)) {
            std::vector<std::unique_ptr<ExprAST>> elements;
            elements.push_back(std::move(expr));
            while (check(TokenType::COMMA)) {
                advance(); // ,
                elements.push_back(parseExpression());
            }
            expr = std::make_unique<ListExprAST>(std::move(elements));
        }
    }
    if (check(TokenType::NEWLINE)) advance();
    return std::make_unique<ReturnStmtAST>(std::move(expr));
}

std::unique_ptr<StmtAST> Parser::parseVarDecl() {
    if (check(TokenType::KEYWORD_VAR)) {
        advance(); // var
        std::string name = advance().value;
        std::string type_name = "auto";
        if (check(TokenType::KEYWORD_TYPE)) {
            type_name = advance().value;
        } else if (check(TokenType::IDENTIFIER) && peek().type != TokenType::ASSIGN) {
            type_name = advance().value;
        }
        if (is_inside_function) {
            local_variables.insert(name);
        } else if (!namespace_prefix.empty()) {
            name = namespace_prefix + "_" + name;
        }
        variables[name] = type_name;
        std::unique_ptr<ExprAST> initializer = nullptr;
        if (check(TokenType::ASSIGN)) {
            advance(); // =
            initializer = parseExpression();
        }
        if (check(TokenType::NEWLINE)) advance();
        std::vector<VarDeclItem> vars;
        vars.push_back(VarDeclItem(type_name, name));
        return std::make_unique<VarDeclStmtAST>(std::move(vars), std::move(initializer));
    }

    bool is_decl = false;
    std::string type_name = "";
    if (check(TokenType::KEYWORD_TYPE)) {
        is_decl = true;
        type_name = advance().value;
    } else if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::IDENTIFIER) {
        is_decl = true;
        type_name = advance().value;
        if (!namespace_prefix.empty() && !isBuiltIn(type_name)) {
            type_name = namespace_prefix + "_" + type_name;
        }
    } else if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::DOT && 
               peek(2).type == TokenType::IDENTIFIER && peek(3).type == TokenType::IDENTIFIER) {
        is_decl = true;
        std::string alias = advance().value;
        advance(); // .
        std::string struct_name = advance().value;
        type_name = alias + "_" + struct_name;
    }
    
    if (is_decl) {
        std::string name = advance().value;
        if (is_inside_function) {
            local_variables.insert(name);
        } else if (!namespace_prefix.empty()) {
            name = namespace_prefix + "_" + name;
        }
        variables[name] = type_name;
        
        std::unique_ptr<ExprAST> size_expr = nullptr;
        if (type_name == "array" && check(TokenType::LBRACKET)) {
            advance(); // [
            size_expr = parseExpression();
            expect(TokenType::RBRACKET, "Expected ']' after array size");
        }
        
        std::vector<VarDeclItem> vars;
        vars.push_back(VarDeclItem(type_name, name, std::move(size_expr)));
        
        while (check(TokenType::COMMA)) {
            advance(); // ,
            std::string next_type = type_name;
            if (check(TokenType::KEYWORD_TYPE)) {
                next_type = advance().value;
            } else if (check(TokenType::IDENTIFIER)) {
                if (peek(1).type == TokenType::DOT && peek(2).type == TokenType::IDENTIFIER) {
                    std::string alias = advance().value;
                    advance(); // .
                    next_type = alias + "_" + advance().value;
                } else if (peek(1).type == TokenType::IDENTIFIER) {
                    next_type = advance().value;
                    if (!namespace_prefix.empty() && !isBuiltIn(next_type)) {
                        next_type = namespace_prefix + "_" + next_type;
                    }
                }
            }
            
            std::unique_ptr<ExprAST> next_size_expr = nullptr;
            if (next_type == "array" && check(TokenType::LBRACKET)) {
                advance(); // [
                next_size_expr = parseExpression();
                expect(TokenType::RBRACKET, "Expected ']' after array size");
            }
            
            std::string next_name = advance().value;
            if (is_inside_function) {
                local_variables.insert(next_name);
            } else if (!namespace_prefix.empty()) {
                next_name = namespace_prefix + "_" + next_name;
            }
            variables[next_name] = next_type;
            vars.push_back(VarDeclItem(next_type, next_name, std::move(next_size_expr)));
        }
        
        std::unique_ptr<ExprAST> initializer = nullptr;
        if (check(TokenType::ASSIGN)) {
            advance(); // =
            initializer = parseExpression();
        }
        if (check(TokenType::NEWLINE)) advance();
        return std::make_unique<VarDeclStmtAST>(std::move(vars), std::move(initializer));
    }
    return nullptr;
}

std::unique_ptr<StmtAST> Parser::parseSwitch() {
    advance(); // switch
    auto cond = parseExpression();
    expect(TokenType::COLON, "Expected ':' after switch condition");
    if (check(TokenType::NEWLINE)) advance();
    expect(TokenType::INDENT, "Expected indentation for switch body");
    
    std::vector<SwitchCase> cases;
    std::unique_ptr<BlockStmtAST> default_block = nullptr;
    
    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        if (check(TokenType::NEWLINE)) { advance(); continue; }
        
        if (check(TokenType::KEYWORD_CASE)) {
            advance(); // case
            auto case_val = parseExpression();
            expect(TokenType::COLON, "Expected ':' after case expression");
            if (check(TokenType::NEWLINE)) advance();
            auto case_body = parseBlock();
            cases.push_back({std::move(case_val), std::move(case_body)});
        } else if (check(TokenType::KEYWORD_DEFAULT)) {
            advance(); // default
            expect(TokenType::COLON, "Expected ':' after default");
            if (check(TokenType::NEWLINE)) advance();
            default_block = parseBlock();
        } else {
            std::cerr << "Syntax Error on line " << peek().line << ": Expected 'case' or 'default' in switch body\n";
            exit(1);
        }
    }
    if (check(TokenType::DEDENT)) advance();
    return std::make_unique<SwitchStmtAST>(std::move(cond), std::move(cases), std::move(default_block));
}

std::unique_ptr<StmtAST> Parser::parseEnum() {
    advance(); // enum
    std::string enum_name = advance().value;
    expect(TokenType::COLON, "Expected ':' after enum name");
    if (check(TokenType::NEWLINE)) advance();
    
    std::vector<std::pair<std::string, int>> members;
    int current_val = 0;
    
    if (check(TokenType::INDENT)) {
        advance();
        while (!check(TokenType::DEDENT) && !isAtEnd()) {
            if (check(TokenType::NEWLINE)) { advance(); continue; }
            Token item_tok = advance();
            std::string item_name = item_tok.value;
            if (check(TokenType::ASSIGN)) {
                advance(); // =
                Token val_tok = advance();
                current_val = std::stoi(val_tok.value);
            }
            members.push_back({item_name, current_val});
            enums[enum_name][item_name] = current_val;
            variables[enum_name + "_" + item_name] = "int";
            current_val++;
            if (check(TokenType::COMMA)) advance();
            if (check(TokenType::NEWLINE)) advance();
        }
        if (check(TokenType::DEDENT)) advance();
    } else {
        while (!check(TokenType::NEWLINE) && !isAtEnd()) {
            Token item_tok = advance();
            std::string item_name = item_tok.value;
            if (check(TokenType::ASSIGN)) {
                advance(); // =
                Token val_tok = advance();
                current_val = std::stoi(val_tok.value);
            }
            members.push_back({item_name, current_val});
            enums[enum_name][item_name] = current_val;
            variables[enum_name + "_" + item_name] = "int";
            current_val++;
            if (check(TokenType::COMMA)) advance();
        }
        if (check(TokenType::NEWLINE)) advance();
    }
    
    return std::make_unique<EnumDeclStmtAST>(enum_name, std::move(members));
}

std::unique_ptr<StmtAST> Parser::parseAssert() {
    int line = advance().line; // assert
    auto cond = parseExpression();
    std::unique_ptr<ExprAST> msg = nullptr;
    if (check(TokenType::COMMA)) {
        advance(); // ,
        msg = parseExpression();
    }
    if (check(TokenType::NEWLINE)) advance();
    return std::make_unique<AssertStmtAST>(std::move(cond), std::move(msg), line);
}

std::unique_ptr<StmtAST> Parser::parseInterface() {
    advance(); // interface
    std::string iface_name = advance().value;
    expect(TokenType::COLON, "Expected ':' after interface name");
    if (check(TokenType::NEWLINE)) advance();
    expect(TokenType::INDENT, "Expected indentation for interface body");
    
    std::vector<std::string> methods;
    while (!check(TokenType::DEDENT) && !isAtEnd()) {
        if (check(TokenType::NEWLINE)) { advance(); continue; }
        
        if (check(TokenType::KEYWORD_FN)) {
            advance(); // fn / func / function
            std::string method_name = advance().value;
            expect(TokenType::LPAREN, "Expected '(' after method name in interface");
            while (!check(TokenType::RPAREN) && !isAtEnd()) {
                advance();
            }
            expect(TokenType::RPAREN, "Expected ')' after parameters in interface method");
            if (check(TokenType::ARROW)) {
                advance(); // ->
                parseType();
            }
            if (check(TokenType::NEWLINE)) advance();
            methods.push_back(method_name);
            interfaces[iface_name].insert(method_name);
        } else {
            advance();
        }
    }
    if (check(TokenType::DEDENT)) advance();
    return std::make_unique<InterfaceDeclStmtAST>(iface_name, std::move(methods));
}

std::unique_ptr<StmtAST> Parser::parseStatement() {
    if (check(TokenType::NEWLINE)) { advance(); return nullptr; }
    
    bool is_kernel = false;
    if (check(TokenType::KEYWORD_KERNEL)) {
        advance(); // kernel
        is_kernel = true;
    }
    if (check(TokenType::KEYWORD_FN) || is_kernel) return parseFunction(is_kernel);
    if (check(TokenType::KEYWORD_STRUCT)) return parseStruct();
    if (check(TokenType::KEYWORD_INTERFACE)) return parseInterface();
    if (check(TokenType::KEYWORD_SWITCH)) return parseSwitch();
    if (check(TokenType::KEYWORD_ENUM)) return parseEnum();
    if (check(TokenType::KEYWORD_ASSERT)) return parseAssert();
    if (check(TokenType::KEYWORD_TRY)) return parseTryExcept();
    if (check(TokenType::KEYWORD_THROW)) return parseThrow();
    if (check(TokenType::KEYWORD_IMPORT)) return parseImport();
    if (check(TokenType::KEYWORD_IF)) return parseIf();
    if (check(TokenType::KEYWORD_WHILE)) return parseWhile();
    if (check(TokenType::KEYWORD_FOR)) return parseFor();
    if (check(TokenType::KEYWORD_RETURN)) return parseReturn();
    if (check(TokenType::KEYWORD_BREAK)) return parseBreak();
    if (check(TokenType::KEYWORD_CONTINUE)) return parseContinue();
    if (check(TokenType::KEYWORD_DEFER)) return parseDefer();
    if (check(TokenType::KEYWORD_GO)) return parseGo();
    
    // Check if it's a variable declaration
    auto var_decl = parseVarDecl();
    if (var_decl) return var_decl;
    
    // Variable assignment or Expression statement
    auto expr = parseExpression();
    if (check(TokenType::COMMA)) {
        std::vector<std::unique_ptr<ExprAST>> elements;
        elements.push_back(std::move(expr));
        while (check(TokenType::COMMA)) {
            advance(); // ,
            elements.push_back(parseExpression());
        }
        expr = std::make_unique<ListExprAST>(std::move(elements));
    }

    // Channel send statement: ch <- val
    if (check(TokenType::LARROW)) {
        advance(); // <-
        auto val = parseExpression();
        if (check(TokenType::NEWLINE)) advance();
        std::vector<std::unique_ptr<ExprAST>> sendArgs;
        sendArgs.push_back(std::move(expr));
        sendArgs.push_back(std::move(val));
        return std::make_unique<ExprStmtAST>(std::make_unique<CallExprAST>("chan_send", std::move(sendArgs)));
    }

    // Short variable declaration: x := expr or a, b := expr
    if (check(TokenType::COLON_ASSIGN)) {
        advance(); // :=
        auto rhs = parseExpression();
        if (check(TokenType::NEWLINE)) advance();
        if (expr->getType() == ASTNodeType::VARIABLE_EXPR) {
            auto var_name = static_cast<VariableExprAST*>(expr.get())->name;
            if (is_inside_function) {
                local_variables.insert(var_name);
            }
            std::string inferred_type = "auto";
            if (rhs->getType() == ASTNodeType::CALL_EXPR) {
                auto call = static_cast<CallExprAST*>(rhs.get());
                if (structs.count(call->callee)) {
                    inferred_type = call->callee;
                }
            }
            variables[var_name] = inferred_type;
            std::vector<VarDeclItem> items;
            items.push_back(VarDeclItem("auto", var_name));
            return std::make_unique<VarDeclStmtAST>(std::move(items), std::move(rhs));
        } else if (expr->getType() == ASTNodeType::LIST_LITERAL) {
            auto list_expr = static_cast<ListExprAST*>(expr.get());
            std::vector<VarDeclItem> items;
            for (auto& el : list_expr->elements) {
                if (el->getType() == ASTNodeType::VARIABLE_EXPR) {
                    auto var_name = static_cast<VariableExprAST*>(el.get())->name;
                    if (is_inside_function) {
                        local_variables.insert(var_name);
                    }
                    variables[var_name] = "auto";
                    items.push_back(VarDeclItem("auto", var_name));
                }
            }
            return std::make_unique<VarDeclStmtAST>(std::move(items), std::move(rhs));
        }
    }

    // Compound assignments: +=, -=, *=, /=, %=
    if (check(TokenType::PLUS_EQUAL) || check(TokenType::MINUS_EQUAL) ||
        check(TokenType::MUL_EQUAL) || check(TokenType::DIV_EQUAL) ||
        check(TokenType::MOD_EQUAL)) {
        Token op_tok = advance();
        std::string bin_op = "";
        if (op_tok.type == TokenType::PLUS_EQUAL) bin_op = "+";
        else if (op_tok.type == TokenType::MINUS_EQUAL) bin_op = "-";
        else if (op_tok.type == TokenType::MUL_EQUAL) bin_op = "*";
        else if (op_tok.type == TokenType::DIV_EQUAL) bin_op = "/";
        else if (op_tok.type == TokenType::MOD_EQUAL) bin_op = "%";
        
        auto rhs = parseExpression();
        if (check(TokenType::NEWLINE)) advance();
        if (expr->getType() == ASTNodeType::VARIABLE_EXPR) {
            std::string var_name = static_cast<VariableExprAST*>(expr.get())->name;
            auto lhs_copy = std::make_unique<VariableExprAST>(var_name);
            auto bin_expr = std::make_unique<BinaryExprAST>(bin_op, std::move(lhs_copy), std::move(rhs));
            return std::make_unique<VarAssignStmtAST>(std::move(expr), std::move(bin_expr));
        } else if (expr->getType() == ASTNodeType::DOT_ACCESS) {
            auto dot = static_cast<DotAccessExprAST*>(expr.get());
            if (dot->object->getType() == ASTNodeType::VARIABLE_EXPR) {
                std::string obj_name = static_cast<VariableExprAST*>(dot->object.get())->name;
                auto obj_copy = std::make_unique<VariableExprAST>(obj_name);
                auto read_dot = std::make_unique<DotAccessExprAST>(std::move(obj_copy), dot->member);
                auto bin_expr = std::make_unique<BinaryExprAST>(bin_op, std::move(read_dot), std::move(rhs));
                return std::make_unique<VarAssignStmtAST>(std::move(expr), std::move(bin_expr));
            }
        }
    }
    
    // Increment / Decrement: ++, --
    if (check(TokenType::PLUS_PLUS) || check(TokenType::MINUS_MINUS)) {
        Token op_tok = advance();
        std::string bin_op = (op_tok.type == TokenType::PLUS_PLUS) ? "+" : "-";
        if (check(TokenType::NEWLINE)) advance();
        if (expr->getType() == ASTNodeType::VARIABLE_EXPR) {
            std::string var_name = static_cast<VariableExprAST*>(expr.get())->name;
            auto lhs_copy = std::make_unique<VariableExprAST>(var_name);
            auto one = std::make_unique<NumberExprAST>("1", false);
            auto bin_expr = std::make_unique<BinaryExprAST>(bin_op, std::move(lhs_copy), std::move(one));
            return std::make_unique<VarAssignStmtAST>(std::move(expr), std::move(bin_expr));
        }
    }

    if (check(TokenType::ASSIGN)) {
        advance(); // =
        auto rhs = parseExpression();
        if (check(TokenType::NEWLINE)) advance();
        return std::make_unique<VarAssignStmtAST>(std::move(expr), std::move(rhs));
    }
    if (check(TokenType::NEWLINE)) advance();
    return std::make_unique<ExprStmtAST>(std::move(expr));
}
