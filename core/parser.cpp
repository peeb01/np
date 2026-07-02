#include "../include/parser.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <filesystem>
#include "../include/lexer.hpp"

static std::string readFile(const std::string& filename) {
    std::string path = filename;
    
    // 1. Check if the local path exists and is a directory
    if (std::filesystem::exists(path) && std::filesystem::is_directory(path)) {
        if (std::filesystem::exists(path + "/mod.np")) path = path + "/mod.np";
        else if (std::filesystem::exists(path + "/main.np")) path = path + "/main.np";
        else if (std::filesystem::exists(path + "/index.np")) path = path + "/index.np";
    } else {
        // 2. Check if the file is not openable locally, then check .np_packages/
        std::ifstream test_file(path);
        if (!test_file.is_open()) {
            std::string pkg_path = ".np_packages/" + filename;
            if (std::filesystem::exists(pkg_path) && std::filesystem::is_directory(pkg_path)) {
                if (std::filesystem::exists(pkg_path + "/mod.np")) path = pkg_path + "/mod.np";
                else if (std::filesystem::exists(pkg_path + "/main.np")) path = pkg_path + "/main.np";
                else if (std::filesystem::exists(pkg_path + "/index.np")) path = pkg_path + "/index.np";
            } else {
                path = pkg_path;
            }
        }
    }
    
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << " (also checked .np_packages/" << filename << " and directory entry points)\n";
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

Parser::Parser(const std::vector<Token>& tokens, const std::string& namespace_prefix)
    : tokens(tokens), pos(0), namespace_prefix(namespace_prefix), is_inside_function(false) {}

bool Parser::isBuiltIn(const std::string& name) const {
    static const std::unordered_set<std::string> builtins = {
        "print", "int", "float", "string", "bool", "array", "dict", "type", "len",
        "read_file", "write_file", "input_int", "input_float", "input_string",
        "net_listen", "net_accept", "net_connect", "net_send", "net_recv", "net_close",
        "range", "Exception"
    };
    return builtins.count(name) > 0;
}

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

int Parser::getPrecedence(const std::string& op) const {
    if (op == "or") return 10;
    if (op == "and") return 20;
    if (op == "==" || op == "!=") return 30;
    if (op == "<" || op == ">" || op == "<=" || op == ">=") return 30;
    if (op == "+" || op == "-") return 40;
    if (op == "*" || op == "/" || op == "%") return 50;
    if (op == "^") return 60;
    return -1;
}

void Parser::parse() {
    while (!isAtEnd()) {
        if (check(TokenType::NEWLINE)) { advance(); continue; }
        auto stmt = parseStatement();
        if (stmt) {
            ast_root.push_back(std::move(stmt));
        }
    }
}

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
            op_token.type != TokenType::KEYWORD_OR) {
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
    
    // Prefix unary operators: not, -, +
    if (t.type == TokenType::KEYWORD_NOT || (t.type == TokenType::OPERATOR && (t.value == "-" || t.value == "+"))) {
        advance();
        std::string op = t.value;
        auto operand = parseExpression(70); // unary precedence
        if (!operand) {
            std::cerr << "Syntax Error on line " << t.line << ": Expected expression after prefix operator\n";
            exit(1);
        }
        // Represent unary -x as 0 - x, not x as not(x) or similar
        return std::make_unique<BinaryExprAST>(op, std::make_unique<NumberExprAST>("0", false), std::move(operand));
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
    if (t.type == TokenType::BOOL_LITERAL) {
        advance();
        return parsePostFix(std::make_unique<BoolExprAST>(t.value == "true"));
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
                args.push_back(parseExpression());
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
            
            // Check if LHS is a namespace/import alias
            if (expr->getType() == ASTNodeType::VARIABLE_EXPR) {
                auto var_expr = static_cast<VariableExprAST*>(expr.get());
                if (import_aliases.count(var_expr->name) > 0) {
                    if (check(TokenType::LPAREN)) {
                        advance(); // consume (
                        std::vector<std::unique_ptr<ExprAST>> args;
                        while (!check(TokenType::RPAREN) && !isAtEnd()) {
                            args.push_back(parseExpression());
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
            
            expr = std::make_unique<DotAccessExprAST>(std::move(expr), member.value);
        } else if (check(TokenType::LBRACKET)) {
            advance(); // consume [
            
            std::unique_ptr<ExprAST> start = nullptr;
            std::unique_ptr<ExprAST> end = nullptr;
            bool is_slice = false;
            
            if (check(TokenType::COLON)) {
                is_slice = true;
                advance(); // consume :
                if (!check(TokenType::RBRACKET)) {
                    end = parseExpression();
                }
            } else {
                auto first_expr = parseExpression();
                if (check(TokenType::COLON)) {
                    is_slice = true;
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
        } else {
            break;
        }
    }
    return expr;
}

std::unique_ptr<StmtAST> Parser::parseStatement() {
    if (check(TokenType::NEWLINE)) { advance(); return nullptr; }
    
    // Function definition
    if (check(TokenType::KEYWORD_FN)) {
        advance(); // fn
        std::string name = advance().value;
        if (!namespace_prefix.empty()) {
            name = namespace_prefix + "_" + name;
        }
        
        is_inside_function = true;
        local_variables.clear();
        
        expect(TokenType::LPAREN, "Expected '(' after function name");
        std::vector<FuncParam> params;
        while (!check(TokenType::RPAREN) && !isAtEnd()) {
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
                    expect(TokenType::IDENTIFIER, "Expected member identifier after '.' in parameter type");
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
                std::string param_name = advance().value;
                params.push_back({type_name, param_name});
                variables[param_name] = type_name;
                local_variables.insert(param_name);
                if (check(TokenType::COMMA)) advance();
            } else {
                std::cerr << "Syntax Error on line " << peek().line << ": Unexpected token in function parameters\n";
                exit(1);
            }
        }
        expect(TokenType::RPAREN, "Expected ')' after parameters");
        
        std::string ret_type = "void";
        if (check(TokenType::ARROW)) {
            advance(); // ->
            ret_type = "";
            while (!check(TokenType::COLON)) {
                if (check(TokenType::KEYWORD_TYPE)) {
                    ret_type += advance().value;
                } else if (check(TokenType::IDENTIFIER)) {
                    std::string r_name = advance().value;
                    if (check(TokenType::DOT)) {
                        advance(); // .
                        Token member_tok = peek();
                        expect(TokenType::IDENTIFIER, "Expected member identifier after '.' in return type");
                        std::string member = member_tok.value;
                        ret_type += r_name + "_" + member;
                    } else {
                        if (!namespace_prefix.empty() && !isBuiltIn(r_name)) {
                            ret_type += namespace_prefix + "_" + r_name;
                        } else {
                            ret_type += r_name;
                        }
                    }
                } else if (check(TokenType::COMMA)) {
                    ret_type += ",";
                    advance();
                } else {
                    std::cerr << "Syntax Error on line " << peek().line << ": Unexpected token in return type\n";
                    exit(1);
                }
            }
        }
        expect(TokenType::COLON, "Expected ':' before function block");
        if (check(TokenType::NEWLINE)) advance();
        auto body = parseBlock();
        
        is_inside_function = false;
        local_variables.clear();
        
        return std::make_unique<FuncDeclStmtAST>(name, std::move(params), ret_type, std::move(body));
    }
    
    // Struct definition
    if (check(TokenType::KEYWORD_STRUCT)) {
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
    
    // Try-Except
    if (check(TokenType::KEYWORD_TRY)) {
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
    
    // Throw
    if (check(TokenType::KEYWORD_THROW)) {
        advance(); // throw
        auto expr = parseExpression();
        if (check(TokenType::NEWLINE)) advance();
        return std::make_unique<ThrowStmtAST>(std::move(expr));
    }
    
    // Import
    if (check(TokenType::KEYWORD_IMPORT)) {
        advance(); // import
        Token path_tok = advance();
        
        std::string filename = "";
        std::string alias = "";
        bool is_module_import = false;
        
        if (path_tok.type == TokenType::IDENTIFIER) {
            filename = path_tok.value;
            is_module_import = true;
        } else if (path_tok.type == TokenType::STRING_LITERAL) {
            filename = path_tok.value;
        } else {
            std::cerr << "Syntax Error on line " << path_tok.line << ": Expected identifier or string literal after import\n";
            exit(1);
        }
        
        // Parse 'as alias'
        if (check(TokenType::IDENTIFIER) && peek().value == "as") {
            advance(); // as
            Token alias_tok = peek();
            expect(TokenType::IDENTIFIER, "Expected alias identifier after 'as'");
            alias = alias_tok.value;
        } else {
            if (is_module_import) {
                alias = filename;
            } else {
                alias = "";
            }
        }
        
        if (!alias.empty()) {
            import_aliases.insert(alias);
        }
        
        if (is_module_import) {
            imported_modules.insert(filename);
            if (check(TokenType::NEWLINE)) advance();
            return std::make_unique<ImportStmtAST>(filename);
        } else {
            static std::unordered_set<std::string> imported_files;
            if (imported_files.count(filename) == 0) {
                imported_files.insert(filename);
                std::string import_source = readFile(filename);
                Lexer import_lexer(import_source);
                std::vector<Token> import_tokens = import_lexer.tokenize();
                Parser import_parser(import_tokens, alias);
                import_parser.parse();
                
                // Merge AST, variables, and structs
                for (auto& node : import_parser.ast_root) {
                    ast_root.push_back(std::move(node));
                }
                for (auto const& [k, v] : import_parser.variables) {
                    variables[k] = v;
                }
                for (auto const& [k, v] : import_parser.structs) {
                    structs[k] = v;
                }
                for (const auto& a : import_parser.import_aliases) {
                    import_aliases.insert(a);
                }
            }
            if (check(TokenType::NEWLINE)) advance();
            return nullptr;
        }
    }
    
    // If
    if (check(TokenType::KEYWORD_IF)) {
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
    
    // While
    if (check(TokenType::KEYWORD_WHILE)) {
        advance(); // while
        auto cond = parseExpression();
        expect(TokenType::COLON, "Expected ':' after 'while' condition");
        if (check(TokenType::NEWLINE)) advance();
        auto body = parseBlock();
        return std::make_unique<WhileStmtAST>(std::move(cond), std::move(body));
    }
    
    // For
    if (check(TokenType::KEYWORD_FOR)) {
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
    
    // Return
    if (check(TokenType::KEYWORD_RETURN)) {
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
    
    // Type (Var declaration)
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

    if (check(TokenType::ASSIGN)) {
        advance(); // =
        auto rhs = parseExpression();
        if (check(TokenType::NEWLINE)) advance();
        return std::make_unique<VarAssignStmtAST>(std::move(expr), std::move(rhs));
    }
    if (check(TokenType::NEWLINE)) advance();
    return std::make_unique<ExprStmtAST>(std::move(expr));
}