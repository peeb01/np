#include "../include/codegen.hpp"
#include <fstream>
#include <iostream>

CodeGen::CodeGen(const std::string& cpp_code) : cpp_code(cpp_code) {}

void CodeGen::generateCPlusPlus(const std::string& output_filename) {
    std::ofstream out(output_filename);
    if (!out.is_open()) {
        std::cerr << "Error: Cannot write to " << output_filename << "\n";
        return;
    }

    out << "#include <iostream>\n";
    out << "#include <string>\n";
    out << "#include <tuple>\n\n";
    out << "#include <cstdlib>\n\n";
    out << "#include <vector>\n";
    out << "#include <map>\n";
    out << "#include <variant>\n";
    out << "#include <memory>\n\n";
    out << "#include <cstdint>\n\n";
    out << "#include <cmath>\n\n";
    out << "#include <type_traits>\n\n";
    
    out << "int np_throw_error(const std::string& msg) { std::cerr << msg << std::endl; std::exit(1); return 0; }\n\n";

    out << "struct np_var;\n";
    out << "struct np_var_list;\n";
    out << "struct np_var_dict;\n";
    out << "using ListPtr = std::shared_ptr<np_var_list>;\n";
    out << "using DictPtr = std::shared_ptr<np_var_dict>;\n";
    out << "struct np_var {\n";
    out << "    std::variant<int64_t, double, std::string, bool, ListPtr, DictPtr> v;\n";
    out << "    np_var() : v(0) {}\n";
    out << "    np_var(int val) : v(static_cast<int64_t>(val)) {}\n";
    out << "    np_var(int64_t val) : v(val) {}\n";
    out << "    np_var(double val) : v(val) {}\n";
    out << "    np_var(const char* val) : v(std::string(val)) {}\n";
    out << "    np_var(std::string val) : v(val) {}\n";
    out << "    np_var(bool val) : v(val) {}\n";
    out << "    np_var(const std::vector<np_var>& val);\n";
    out << "    np_var(const std::map<std::string, np_var>& val);\n";
    out << "    np_var(ListPtr val) : v(val) {}\n";
    out << "    np_var(DictPtr val) : v(val) {}\n";
    
    out << "    operator int() const;\n";
    out << "    operator float() const;\n";
    out << "    operator std::string() const;\n";
    out << "    operator bool() const;\n";
    out << "    std::string to_string() const;\n";
    out << "    int length() const;\n";
    out << "    np_var shape() const;\n";
    out << "    bool has_key(const std::string& key) const;\n";
    out << "    bool has_key(const char* key) const;\n";
    out << "    void append(const np_var& val);\n";
    out << "    friend std::ostream& operator<<(std::ostream& os, const np_var& var);\n";
    out << "    np_var& operator[](int index);\n";
    out << "    np_var& operator[](const std::string& key);\n";
    out << "    np_var& operator[](const char* key);\n";
    
    out << "    np_var operator+(const np_var& o) const;\n";
    out << "    np_var operator^(const np_var& o) const;\n";
    out << "#define NP_MATH_OP(op) np_var operator op(const np_var& o) const;\n";
    out << "    NP_MATH_OP(-)\n    NP_MATH_OP(*)\n    NP_MATH_OP(/)\n";
    out << "#define NP_CMP_OP(op) bool operator op(const np_var& o) const;\n";
    out << "    NP_CMP_OP(>)\n    NP_CMP_OP(<)\n    NP_CMP_OP(>=)\n    NP_CMP_OP(<=)\n    NP_CMP_OP(==)\n    NP_CMP_OP(!=)\n";
    out << "};\n\n";
    
    out << "struct np_var_list { std::vector<np_var> vec; };\n";
    out << "struct np_var_dict { std::map<std::string, np_var> map; };\n\n";
    
    out << "inline np_var::np_var(const std::vector<np_var>& val) : v(std::make_shared<np_var_list>(np_var_list{val})) {}\n";
    out << "inline np_var::np_var(const std::map<std::string, np_var>& val) : v(std::make_shared<np_var_dict>(np_var_dict{val})) {}\n";
    out << "inline np_var::operator int() const { if(std::holds_alternative<int64_t>(v)) return static_cast<int>(std::get<int64_t>(v)); if(std::holds_alternative<double>(v)) return static_cast<int>(std::get<double>(v)); return 0; }\n";
    out << "inline np_var::operator float() const { if(std::holds_alternative<double>(v)) return static_cast<float>(std::get<double>(v)); if(std::holds_alternative<int64_t>(v)) return static_cast<float>(std::get<int64_t>(v)); return 0.0f; }\n";
    out << "inline np_var::operator std::string() const { return to_string(); }\n";
    out << "inline np_var::operator bool() const { if(std::holds_alternative<bool>(v)) return std::get<bool>(v); return false; }\n";
    out << "inline std::string np_var::to_string() const {\n";
    out << "    if (std::holds_alternative<int64_t>(v)) return std::to_string(std::get<int64_t>(v));\n";
    out << "    if (std::holds_alternative<double>(v)) {\n";
    out << "        std::string s = std::to_string(std::get<double>(v));\n";
    out << "        s.erase(s.find_last_not_of('0') + 1, std::string::npos); if (s.back() == '.') s += \"0\"; return s;\n";
    out << "    }\n";
    out << "    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v);\n";
    out << "    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? \"true\" : \"false\";\n";
    out << "    if (std::holds_alternative<ListPtr>(v)) {\n";
    out << "        std::string s = \"[\"; auto arr = std::get<ListPtr>(v);\n";
    out << "        if (arr) { for (size_t i = 0; i < arr->vec.size(); ++i) { s += arr->vec[i].to_string(); if (i < arr->vec.size() - 1) s += \", \"; } }\n";
    out << "        return s + \"]\";\n";
    out << "    }\n";
    out << "    if (std::holds_alternative<DictPtr>(v)) {\n";
    out << "        std::string s = \"{\"; auto dict = std::get<DictPtr>(v);\n";
    out << "        if (dict) { bool first = true; for (auto const& [k, val] : dict->map) { if (!first) s += \", \"; s += \"\\\"\" + k + \"\\\": \" + val.to_string(); first = false; } }\n";
    out << "        return s + \"}\";\n";
    out << "    }\n";
    out << "    return \"\";\n";
    out << "}\n";
    out << "inline int np_var::length() const {\n";
    out << "    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) return std::get<ListPtr>(v)->vec.size();\n";
    out << "    if (std::holds_alternative<DictPtr>(v) && std::get<DictPtr>(v)) return std::get<DictPtr>(v)->map.size();\n";
    out << "    if (std::holds_alternative<std::string>(v)) return std::get<std::string>(v).length();\n";
    out << "    return 0;\n";
    out << "}\n";
    out << "inline np_var np_var::shape() const {\n";
    out << "    if (!std::holds_alternative<ListPtr>(v) || !std::get<ListPtr>(v)) return std::vector<np_var>{};\n";
    out << "    std::vector<np_var> dims;\n";
    out << "    np_var current = *this;\n";
    out << "    while (std::holds_alternative<ListPtr>(current.v) && std::get<ListPtr>(current.v)) {\n";
    out << "        auto& vec = std::get<ListPtr>(current.v)->vec;\n";
    out << "        dims.push_back(static_cast<int>(vec.size()));\n";
    out << "        if (vec.empty()) break; current = vec[0];\n";
    out << "    }\n";
    out << "    return dims;\n";
    out << "}\n";
    out << "inline bool np_var::has_key(const std::string& key) const {\n";
    out << "    if (std::holds_alternative<DictPtr>(v) && std::get<DictPtr>(v)) return std::get<DictPtr>(v)->map.count(key) > 0;\n";
    out << "    return false;\n";
    out << "}\n";
    out << "inline bool np_var::has_key(const char* key) const { return has_key(std::string(key)); }\n";
    out << "inline void np_var::append(const np_var& val) {\n";
    out << "    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) std::get<ListPtr>(v)->vec.push_back(val);\n";
    out << "}\n";
    out << "inline std::ostream& operator<<(std::ostream& os, const np_var& var) { os << var.to_string(); return os; }\n";
    out << "inline np_var& np_var::operator[](int index) { return std::get<ListPtr>(v)->vec[index]; }\n";
    out << "inline np_var& np_var::operator[](const std::string& key) { return std::get<DictPtr>(v)->map[key]; }\n";
    out << "inline np_var& np_var::operator[](const char* key) { return std::get<DictPtr>(v)->map[std::string(key)]; }\n";
    out << "inline np_var np_var::operator+(const np_var& o) const { if(std::holds_alternative<std::string>(v) || std::holds_alternative<std::string>(o.v)) return to_string() + o.to_string(); if(std::holds_alternative<int64_t>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<int64_t>(v) + std::get<int64_t>(o.v); if(std::holds_alternative<double>(v) && std::holds_alternative<double>(o.v)) return std::get<double>(v) + std::get<double>(o.v); if(std::holds_alternative<int64_t>(v) && std::holds_alternative<double>(o.v)) return std::get<int64_t>(v) + std::get<double>(o.v); if(std::holds_alternative<double>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<double>(v) + std::get<int64_t>(o.v); return 0; }\n";
    out << "inline np_var np_var::operator^(const np_var& o) const { if(std::holds_alternative<int64_t>(v) && std::holds_alternative<int64_t>(o.v)) return static_cast<int64_t>(std::pow(std::get<int64_t>(v), std::get<int64_t>(o.v))); if(std::holds_alternative<double>(v) && std::holds_alternative<double>(o.v)) return std::pow(std::get<double>(v), std::get<double>(o.v)); if(std::holds_alternative<int64_t>(v) && std::holds_alternative<double>(o.v)) return std::pow(std::get<int64_t>(v), std::get<double>(o.v)); if(std::holds_alternative<double>(v) && std::holds_alternative<int64_t>(o.v)) return std::pow(std::get<double>(v), std::get<int64_t>(o.v)); return 0; }\n";
    out << "#define NP_MATH_OP_IMPL(op) inline np_var np_var::operator op(const np_var& o) const { if(std::holds_alternative<int64_t>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<int64_t>(v) op std::get<int64_t>(o.v); if(std::holds_alternative<double>(v) && std::holds_alternative<double>(o.v)) return std::get<double>(v) op std::get<double>(o.v); if(std::holds_alternative<int64_t>(v) && std::holds_alternative<double>(o.v)) return std::get<int64_t>(v) op std::get<double>(o.v); if(std::holds_alternative<double>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<double>(v) op std::get<int64_t>(o.v); return 0; }\n";
    out << "NP_MATH_OP_IMPL(-)\nNP_MATH_OP_IMPL(*)\nNP_MATH_OP_IMPL(/)\n";
    out << "#define NP_CMP_OP_IMPL(op) inline bool np_var::operator op(const np_var& o) const { if(std::holds_alternative<int64_t>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<int64_t>(v) op std::get<int64_t>(o.v); if(std::holds_alternative<double>(v) && std::holds_alternative<double>(o.v)) return std::get<double>(v) op std::get<double>(o.v); if(std::holds_alternative<int64_t>(v) && std::holds_alternative<double>(o.v)) return std::get<int64_t>(v) op std::get<double>(o.v); if(std::holds_alternative<double>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<double>(v) op std::get<int64_t>(o.v); if(std::holds_alternative<std::string>(v) && std::holds_alternative<std::string>(o.v)) return std::get<std::string>(v) op std::get<std::string>(o.v); return false; }\n";
    out << "NP_CMP_OP_IMPL(>)\nNP_CMP_OP_IMPL(<)\nNP_CMP_OP_IMPL(>=)\nNP_CMP_OP_IMPL(<=)\nNP_CMP_OP_IMPL(==)\nNP_CMP_OP_IMPL(!=)\n\n";

    out << "inline np_var operator+(int lhs, const np_var& rhs) { return np_var(lhs) + rhs; }\n";
    out << "inline np_var operator+(float lhs, const np_var& rhs) { return np_var(lhs) + rhs; }\n";
    out << "inline np_var operator+(const std::string& lhs, const np_var& rhs) { return np_var(lhs) + rhs; }\n";
    out << "inline np_var operator+(const char* lhs, const np_var& rhs) { return np_var(lhs) + rhs; }\n";
    out << "inline np_var operator+(const np_var& lhs, int rhs) { return lhs + np_var(rhs); }\n";
    out << "inline np_var operator+(const np_var& lhs, float rhs) { return lhs + np_var(rhs); }\n";
    out << "inline np_var operator+(const np_var& lhs, const std::string& rhs) { return lhs + np_var(rhs); }\n";
    out << "inline np_var operator+(const np_var& lhs, const char* rhs) { return lhs + np_var(rhs); }\n";
    out << "#define NP_GLOBAL_MATH_OP(op) \\\n";
    out << "inline np_var operator op(int lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \\\n";
    out << "inline np_var operator op(float lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \\\n";
    out << "inline np_var operator op(const np_var& lhs, int rhs) { return lhs op np_var(rhs); } \\\n";
    out << "inline np_var operator op(const np_var& lhs, float rhs) { return lhs op np_var(rhs); }\n";
    out << "NP_GLOBAL_MATH_OP(-)\nNP_GLOBAL_MATH_OP(*)\nNP_GLOBAL_MATH_OP(/)\n";
    out << "NP_GLOBAL_MATH_OP(^)\n";
    out << "#define NP_GLOBAL_CMP_OP(op) \\\n";
    out << "inline bool operator op(int lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \\\n";
    out << "inline bool operator op(float lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \\\n";
    out << "inline bool operator op(bool lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \\\n";
    out << "inline bool operator op(const std::string& lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \\\n";
    out << "inline bool operator op(const char* lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \\\n";
    out << "inline bool operator op(const np_var& lhs, int rhs) { return lhs op np_var(rhs); } \\\n";
    out << "inline bool operator op(const np_var& lhs, float rhs) { return lhs op np_var(rhs); } \\\n";
    out << "inline bool operator op(const np_var& lhs, bool rhs) { return lhs op np_var(rhs); } \\\n";
    out << "inline bool operator op(const np_var& lhs, const std::string& rhs) { return lhs op np_var(rhs); } \\\n";
    out << "inline bool operator op(const np_var& lhs, const char* rhs) { return lhs op np_var(rhs); }\n";
    out << "NP_GLOBAL_CMP_OP(>)\nNP_GLOBAL_CMP_OP(<)\nNP_GLOBAL_CMP_OP(>=)\nNP_GLOBAL_CMP_OP(<=)\nNP_GLOBAL_CMP_OP(==)\nNP_GLOBAL_CMP_OP(!=)\n\n";

    out << "template<typename T, typename U> auto np_pow_call(T lhs, U rhs) {\n";
    out << "    if constexpr (std::is_same_v<T, np_var> || std::is_same_v<U, np_var>) return np_var(lhs) ^ np_var(rhs);\n";
    out << "    else if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) return std::pow(static_cast<double>(lhs), static_cast<double>(rhs));\n";
    out << "    else return static_cast<int64_t>(std::pow(static_cast<double>(lhs), static_cast<double>(rhs)));\n";
    out << "}\n";
    out << "struct np_pow_helper_t {} np_pow_helper;\n";
    out << "template<typename LHS> struct np_pow_proxy { LHS lhs; template<typename RHS> auto operator*(RHS rhs) const { return np_pow_call(lhs, rhs); } };\n";
    out << "template<typename LHS> np_pow_proxy<LHS> operator*(LHS lhs, np_pow_helper_t) { return np_pow_proxy<LHS>{lhs}; }\n\n";

    // np-lang native standard library implementation
    out << "template<typename T> void np_print(T val) { std::cout << val << std::endl; }\n";
    out << "template<typename T, typename... Args> void np_print(T val, Args... args) { std::cout << val << \" \"; np_print(args...); }\n\n";

    out << "struct np_input_proxy {\n";
    out << "    template<typename T> operator T() { T val; std::cin >> val; std::cin.ignore(10000, '\\n'); return val; }\n";
    out << "    operator std::string() { std::string s; std::getline(std::cin, s); return s; }\n";
    out << "};\n";
    out << "#define np_input() np_input_proxy()\n\n";

    out << "int np_to_int(const std::string& s) { try { return std::stoi(s); } catch(...) { return 0; } }\n";
    out << "int np_to_int(float v) { return static_cast<int>(v); }\n";
    out << "float np_to_float(const std::string& s) { try { return std::stof(s); } catch(...) { return 0.0f; } }\n";
    out << "float np_to_float(int64_t v) { return static_cast<float>(v); }\n";
    out << "std::string np_to_string(int v) { return std::to_string(v); }\n";
    out << "std::string np_to_string(int64_t v) { return std::to_string(v); }\n";
    out << "std::string np_to_string(double v) { return std::to_string(v); }\n\n";

    out << "int np_to_int(const np_var& v) { return static_cast<int>(v); }\n";
    out << "float np_to_float(const np_var& v) { return static_cast<float>(v); }\n";
    out << "std::string np_to_string(const np_var& v) { return v.to_string(); }\n\n";

    out << "template <typename T> std::string np_type(T) { return \"unknown\"; }\n";
    out << "template <> std::string np_type(int) { return \"int32\"; }\n";
    out << "template <> std::string np_type(int64_t) { return \"int64\"; }\n";
    out << "template <> std::string np_type(float) { return \"float32\"; }\n";
    out << "template <> std::string np_type(double) { return \"float64\"; }\n";
    out << "template <> std::string np_type(std::string) { return \"string\"; }\n";
    out << "template <> std::string np_type(bool) { return \"bool\"; }\n\n";
    out << "template <> std::string np_type(np_var v) { if(std::holds_alternative<int64_t>(v.v)) return \"int\"; if(std::holds_alternative<double>(v.v)) return \"float\"; if(std::holds_alternative<std::string>(v.v)) return \"string\"; if(std::holds_alternative<bool>(v.v)) return \"bool\"; if(std::holds_alternative<ListPtr>(v.v)) return \"array\"; if(std::holds_alternative<DictPtr>(v.v)) return \"dict\"; return \"unknown\"; }\n\n";

    // Insert dynamic parsed code
    out << cpp_code << "\n";

    out.close();
}