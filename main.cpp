#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <thread>
#include <chrono>
#include "include/lexer.hpp"
#include "include/parser.hpp"
#include "include/llvm_codegen.hpp"
#include "include/package_manager.hpp"
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

#ifndef _WIN32
#include <unistd.h>
#endif

void runPipeline(const std::string& filename, bool is_build_mode, const std::string& custom_out_binary = "", const std::vector<std::string>& run_args = {}, int opt_level = 3) {
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
    codegen.optimize(opt_level);
    
    std::string obj_filename = "temp.o";
    codegen.writeObjectFile(obj_filename);
    
    std::string out_binary;
    if (is_build_mode) {
        out_binary = custom_out_binary.empty() ? "app.out" : custom_out_binary;
        std::filesystem::path out_path(out_binary);
        if (out_path.has_parent_path()) {
            std::filesystem::create_directories(out_path.parent_path());
        }
    } else {
        out_binary = "run_tmp.out";
    }
    
    std::string runtime_path = "runtime/libnpruntime.a";
    {
        std::ifstream check_file(runtime_path);
        if (!check_file.good()) {
            runtime_path = "/usr/local/lib/libnpruntime.a";
        }
    }
    
    // Link using g++
    std::string link_cmd = "g++ " + obj_filename + " " + runtime_path + " -o " + out_binary + " -pthread -ldl";
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
        // In WSL on Windows filesystems, newly written binaries may be temporarily locked by OS scanners
        int retries = 0;
        while (run_result != 0 && retries < 5) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            run_result = std::system(cmd.c_str());
            retries++;
        }
        (void)run_result;
        std::remove(out_binary.c_str());
    }
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cout << "Usage:\n";
        std::cout << "  ./np <file.np> [-O0|-O1|-O2|-O3]                     (Run like Python)\n";
        std::cout << "  ./np build <file.np> [-o output] [-O0|-O1|-O2|-O3]   (Build binary like Go)\n";
        std::cout << "  ./np get                                             (Install dependencies in np.req)\n";
        std::cout << "  ./np install                                         (Alias for get)\n";
        return 1;
    }

    std::string arg1 = argv[1];

    if (arg1 == "get" || arg1 == "install") {
        getPackages();
        return 0;
    } else if (arg1 == "build") {
        std::string entry_file = "";
        std::string output_binary = "app.out";
        int opt_level = 3;

        for (int i = 2; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-o" || arg == "--output") {
                if (i + 1 < argc) {
                    output_binary = argv[++i];
                } else {
                    std::cerr << "Error: Flag " << arg << " requires an output filename.\n";
                    return 1;
                }
            } else if (arg.rfind("-o=", 0) == 0) {
                output_binary = arg.substr(3);
            } else if (arg.rfind("--output=", 0) == 0) {
                output_binary = arg.substr(9);
            } else if (arg == "-O0") {
                opt_level = 0;
            } else if (arg == "-O1") {
                opt_level = 1;
            } else if (arg == "-O2") {
                opt_level = 2;
            } else if (arg == "-O3") {
                opt_level = 3;
            } else if (arg.empty() || arg[0] != '-') {
                if (entry_file.empty()) {
                    entry_file = arg;
                }
            }
        }

        if (entry_file.empty()) {
            std::cerr << "Error: Please specify the entry file to build.\n";
            std::cout << "Usage:\n";
            std::cout << "  ./np build <file.np> [-o output] [-O0|-O1|-O2|-O3]\n";
            std::cout << "  ./np build -o <output> <file.np> [-O0|-O1|-O2|-O3]\n";
            return 1;
        }

        runPipeline(entry_file, true, output_binary, {}, opt_level);
    } else {
        std::string entry_file = "";
        int opt_level = 3;
        std::vector<std::string> run_args;

        for (int i = 1; i < argc; ++i) {
            std::string arg = argv[i];
            if (arg == "-O0") {
                opt_level = 0;
            } else if (arg == "-O1") {
                opt_level = 1;
            } else if (arg == "-O2") {
                opt_level = 2;
            } else if (arg == "-O3") {
                opt_level = 3;
            } else if (entry_file.empty() && !arg.empty() && arg[0] != '-') {
                entry_file = arg;
            } else {
                run_args.push_back(arg);
            }
        }

        if (entry_file.empty()) {
            std::cerr << "Error: Please specify the script file to run.\n";
            return 1;
        }

        runPipeline(entry_file, false, "", run_args, opt_level);
    }

    return 0;
}