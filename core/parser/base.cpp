#include "parser.hpp"
#include <iostream>
#include <unordered_set>

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

bool Parser::check(TokenType type) const {
    return peek().type == type;
}

bool Parser::isAtEnd() const {
    return check(TokenType::EOF_TOKEN);
}

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
    if (op == "|") return 22;
    if (op == "^" || op == "~") return 24;
    if (op == "&") return 26;
    if (op == "==" || op == "!=") return 30;
    if (op == "<" || op == ">" || op == "<=" || op == ">=") return 30;
    if (op == "<<" || op == ">>") return 35;
    if (op == "+" || op == "-") return 40;
    if (op == "*" || op == "/" || op == "%") return 50;
    if (op == "^") return 60;
    return -1;
}
