#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include "include/lexer.hpp"
#include "include/parser.hpp"
#include "include/llvm_codegen.hpp"
#include <llvm/Support/raw_os_ostream.h>

std::string readFile(const std::string& filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error: Could not open file " << filename << "\n";
        exit(1);
    }
    std::stringstream buffer;
    buffer << file.rdbuf();
    return buffer.str();
}

void runPipeline(const std::string& filename, bool is_build_mode, const std::vector<std::string>& run_args = {}) {
    std::string source_code = readFile(filename);

    // Lexical Analysis
    Lexer lexer(source_code);
    std::vector<Token> tokens = lexer.tokenize();

    // Syntax Analysis & AST Creation
    Parser parser(tokens);
    parser.parse();

    // Code Generation using LLVM
    LLVMCodeGen codegen;
    codegen.compile(parser.getAST());
    codegen.optimize();
    
    std::string obj_filename = "temp.o";
    codegen.writeObjectFile(obj_filename);
    
    std::string out_binary = is_build_mode ? "app.out" : "run_tmp.out";
    
    std::string runtime_path = "runtime/libnpruntime.a";
    {
        std::ifstream check_file(runtime_path);
        if (!check_file.good()) {
            runtime_path = "/usr/local/lib/libnpruntime.a";
        }
    }
    
    // Link using g++
    std::string link_cmd = "g++ " + obj_filename + " " + runtime_path + " -o " + out_binary + " -pthread";
    int link_result = std::system(link_cmd.c_str());
    
    // Clean up temporary object file
    std::remove(obj_filename.c_str());
    
    if (link_result != 0) {
        std::cerr << "Error: Linking failed!\n";
        return;
    }
    
    if (!is_build_mode) {
        // Run mode: execute the temporary binary and then delete it
        std::string cmd = "./" + out_binary;
        for (const auto& arg : run_args) {
            cmd += " \"" + arg + "\"";
        }
        int run_result = std::system(cmd.c_str());
        (void)run_result;
        std::remove(out_binary.c_str());
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  ./np <file.np>         (Run like Python)\n";
        std::cout << "  ./np build <file.np>   (Build binary like Go)\n";
        return 1;
    }

    std::string arg1 = argv[1];

    if (arg1 == "build") {
        if (argc < 3) {
            std::cerr << "Error: Please specify the entry file to build.\n";
            return 1;
        }
        runPipeline(argv[2], true);
    } else {
        std::vector<std::string> run_args;
        for (int i = 2; i < argc; ++i) {
            run_args.push_back(argv[i]);
        }
        runPipeline(arg1, false, run_args);
    }

    return 0;
}