#pragma once
#include <string>

class CodeGen {
public:
    CodeGen(const std::string& cpp_code);
    void generateCPlusPlus(const std::string& output_filename);
private:
    std::string cpp_code;
};