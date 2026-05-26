// main.cpp
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include "include/lexer.hpp"
#include "include/parser.hpp"
#include "include/codegen.hpp"

// ฟังก์ชันช่วยอ่านไฟล์ np-lang เข้ามาเป็น String
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

void runPipeline(const std::string& filename, bool is_build_mode) {
    std::string source_code = readFile(filename);

    // 1. Lexical Analysis
    Lexer lexer(source_code);
    std::vector<Token> tokens = lexer.tokenize();

    // 2. Syntax Analysis & Transpilation
    Parser parser(tokens);
    parser.parse();

    // 3. Code Generation & Execution Mode Decision
    CodeGen codegen(parser.getTranslatedCode());
    if (is_build_mode) {
        // std::cout << "[np-lang] Building native binary...\n";
        codegen.generateCPlusPlus("output_tmp.cpp");
        
        int result = std::system("g++ -std=c++17 -O3 output_tmp.cpp -o app.out 2> /dev/null");
        if (result == 0) {
            // std::cout << "[np-lang] Build successful! Created 'app.out'\n";
            std::remove("output_tmp.cpp");
        } else {
            std::cerr << "Syntax Error: Invalid np-lang code structure.\n";
            std::remove("output_tmp.cpp");
        }
    } else {
        // std::cout << "[np-lang] Compiling..." << std::endl;
        codegen.generateCPlusPlus("run_tmp.cpp");
        int compile_result = std::system("g++ -std=c++17 -O3 run_tmp.cpp -o run_tmp.out");
        if (compile_result != 0) {
            std::cerr << "[np-lang] C++ Compilation Failed! See errors above.\n";
        } else {
            // std::cout << "[np-lang] Running..." << std::endl;
            std::system("./run_tmp.out");
        }
        std::remove("run_tmp.cpp");
        std::remove("run_tmp.out");
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
        // เช่นสั่ง `./np main.np`
        runPipeline(arg1, false);
    }

    return 0;
}