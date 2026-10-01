#include "parser.hpp"
#include "lexer.hpp"
#include "package_manager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <unordered_set>
#include <filesystem>

static std::string extractDefaultAlias(const std::string& path) {
    std::string s = path;
    if (s.size() > 3 && s.substr(s.size() - 3) == ".np") {
        s = s.substr(0, s.size() - 3);
    }
    size_t last_slash = s.find_last_of("/\\");
    if (last_slash != std::string::npos) {
        s = s.substr(last_slash + 1);
    }
    return s;
}

static std::string resolveImportCandidate(const std::string& p) {
    if (std::filesystem::exists(p) && !std::filesystem::is_directory(p)) {
        return p;
    }
    // Check with .np extension
    if (std::filesystem::exists(p + ".np")) {
        return p + ".np";
    }
    // Check directory entry points
    if (std::filesystem::exists(p) && std::filesystem::is_directory(p)) {
        std::filesystem::path path_obj(p);
        std::string dir_name = path_obj.filename().string();
        
        if (std::filesystem::exists(p + "/mod.np")) return p + "/mod.np";
        if (std::filesystem::exists(p + "/main.np")) return p + "/main.np";
        if (std::filesystem::exists(p + "/index.np")) return p + "/index.np";
        if (std::filesystem::exists(p + "/" + dir_name + ".np")) return p + "/" + dir_name + ".np";

        std::string sub_p = p + "/" + dir_name;
        if (std::filesystem::exists(sub_p) && std::filesystem::is_directory(sub_p)) {
            if (std::filesystem::exists(sub_p + "/mod.np")) return sub_p + "/mod.np";
            if (std::filesystem::exists(sub_p + "/main.np")) return sub_p + "/main.np";
            if (std::filesystem::exists(sub_p + "/index.np")) return sub_p + "/index.np";
            if (std::filesystem::exists(sub_p + "/" + dir_name + ".np")) return sub_p + "/" + dir_name + ".np";
        }
    }
    return "";
}

static std::string resolveImportPath(const std::string& filename) {
    // 1. Check direct relative/local path
    std::string direct = resolveImportCandidate(filename);
    if (!direct.empty()) return direct;

    // 2. Check in .np_packages/
    std::string pkg_path = ".np_packages/" + filename;
    std::string in_pkg = resolveImportCandidate(pkg_path);
    if (!in_pkg.empty()) return in_pkg;

    // 3. Search inside installed packages in .np_packages/<host>/<user>/<repo>
    if (std::filesystem::exists(".np_packages") && std::filesystem::is_directory(".np_packages")) {
        for (const auto& host_entry : std::filesystem::directory_iterator(".np_packages")) {
            if (host_entry.is_directory()) {
                for (const auto& user_entry : std::filesystem::directory_iterator(host_entry.path())) {
                    if (user_entry.is_directory()) {
                        for (const auto& repo_entry : std::filesystem::directory_iterator(user_entry.path())) {
                            if (repo_entry.is_directory()) {
                                std::string nested = (repo_entry.path() / filename).string();
                                std::string found = resolveImportCandidate(nested);
                                if (!found.empty()) return found;
                            }
                        }
                    }
                }
            }
        }
    }

    // 4. Remote package check and download
    if (isRemotePath(filename)) {
        if (downloadRemotePackage(filename)) {
            in_pkg = resolveImportCandidate(pkg_path);
            if (!in_pkg.empty()) return in_pkg;
        }
    }

    return "";
}

static std::string readSourceFile(const std::string& resolved_path, const std::string& orig_name) {
    std::ifstream file(resolved_path);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open package or file '" << orig_name 
                  << "' (resolved to: '" << resolved_path << "')\n";
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

std::unique_ptr<StmtAST> Parser::parseImport() {
    advance(); // consume 'import'

    // Grouped import: import ( "pkg1" \n "pkg2" )
    if (check(TokenType::LPAREN)) {
        advance(); // consume '('
        while (!check(TokenType::RPAREN) && !isAtEnd()) {
            if (check(TokenType::NEWLINE)) { 
                advance(); 
                continue; 
            }
            parseSingleImport();
            if (check(TokenType::NEWLINE)) advance();
        }
        expect(TokenType::RPAREN, "Expected ')' at end of grouped import");
        if (check(TokenType::NEWLINE)) advance();
        return nullptr;
    }

    return parseSingleImport();
}

std::unique_ptr<StmtAST> Parser::parseSingleImport() {
    std::string filename = "";
    std::string alias = "";
    bool is_module_import = false;

    // Go-style alias preceding path: import myalias "path/pkg"
    if (check(TokenType::IDENTIFIER) && peek(1).type == TokenType::STRING_LITERAL) {
        alias = advance().value;
        filename = advance().value;
    } else {
        Token path_tok = advance();
        if (path_tok.type == TokenType::IDENTIFIER) {
            filename = path_tok.value;
            is_module_import = true;
        } else if (path_tok.type == TokenType::STRING_LITERAL) {
            filename = path_tok.value;
        } else {
            std::cerr << "Syntax Error on line " << path_tok.line << ": Expected identifier or string literal after import\n";
            exit(1);
        }

        // Python-style 'as alias'
        if (check(TokenType::IDENTIFIER) && peek().value == "as") {
            advance(); // consume 'as'
            Token alias_tok = peek();
            expect(TokenType::IDENTIFIER, "Expected alias identifier after 'as'");
            alias = alias_tok.value;
        }
    }

    // Recognize standard library modules even if written as string literal, e.g. import "time"
    static const std::unordered_set<std::string> stdlib_modules = {
        "time", "json", "math", "sys", "regex", "os", "crypto", "threads", "gpu"
    };
    if (stdlib_modules.count(filename) > 0) {
        is_module_import = true;
    }

    if (is_module_import) {
        if (alias.empty()) {
            alias = filename;
        }
        import_aliases.insert(alias);
        imported_modules.insert(filename);
        if (check(TokenType::NEWLINE)) advance();
        return std::make_unique<ImportStmtAST>(filename);
    }

    // Resolve file/package path without requiring .np extension
    std::string resolved_path = resolveImportPath(filename);
    if (resolved_path.empty()) {
        std::cerr << "Error: Could not resolve package or file '" << filename << "'\n";
        exit(1);
    }

    std::string default_alias = extractDefaultAlias(filename);
    if (!alias.empty()) {
        import_aliases.insert(alias);
    } else if (!default_alias.empty()) {
        import_aliases.insert(default_alias);
    }

    static std::unordered_set<std::string> imported_files;
    if (imported_files.count(resolved_path) == 0) {
        imported_files.insert(resolved_path);
        std::string import_source = readSourceFile(resolved_path, filename);
        Lexer import_lexer(import_source);
        std::vector<Token> import_tokens = import_lexer.tokenize();

        std::string sub_alias = alias.empty() ? namespace_prefix : alias;
        Parser import_parser(import_tokens, sub_alias);
        import_parser.parse();

        // Merge AST declarations, variables, and structs
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
