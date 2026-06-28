#pragma once

#include <iostream>
#include <string>
#include <tuple>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <vector>
#include <map>
#include <variant>
#include <memory>
#include <cstdint>
#include <cmath>
#include <type_traits>
#include <algorithm>
#include <chrono>
#include <thread>
#include <regex>

struct np_var;
struct np_var_list;
struct np_var_dict;

using ListPtr = std::shared_ptr<np_var_list>;
using DictPtr = std::shared_ptr<np_var_dict>;

// np_string wrapper to add utility methods
struct np_string : public std::string {
    using std::string::string;
    np_string() : std::string() {}
    np_string(const std::string& s) : std::string(s) {}
    np_string(const char* s) : std::string(s) {}
    np_string slice(int start, int end) const;
    np_var split(const std::string& delim) const;
    np_string join(const np_var& arr) const;
    np_string trim() const;
};

inline np_string operator+(const np_string& lhs, const np_string& rhs) { return np_string(static_cast<const std::string&>(lhs) + static_cast<const std::string&>(rhs)); }
inline np_string operator+(const np_string& lhs, const char* rhs) { return np_string(static_cast<const std::string&>(lhs) + rhs); }
inline np_string operator+(const char* lhs, const np_string& rhs) { return np_string(lhs + static_cast<const std::string&>(rhs)); }
inline np_string operator+(const np_string& lhs, const std::string& rhs) { return np_string(static_cast<const std::string&>(lhs) + rhs); }
inline np_string operator+(const std::string& lhs, const np_string& rhs) { return np_string(lhs + static_cast<const std::string&>(rhs)); }

int np_throw_error(const std::string& msg);

#define NP_INT_PRIMITIVE_OPS(SelfType, PrimType) \
    friend SelfType operator+(SelfType a, PrimType b) { return a + SelfType(b); } \
    friend SelfType operator+(PrimType a, SelfType b) { return SelfType(a) + b; } \
    friend SelfType operator-(SelfType a, PrimType b) { return a - SelfType(b); } \
    friend SelfType operator-(PrimType a, SelfType b) { return SelfType(a) - b; } \
    friend SelfType operator*(SelfType a, PrimType b) { return a * SelfType(b); } \
    friend SelfType operator*(PrimType a, SelfType b) { return SelfType(a) * b; } \
    friend SelfType operator/(SelfType a, PrimType b) { return a / SelfType(b); } \
    friend SelfType operator/(PrimType a, SelfType b) { return SelfType(a) / b; } \
    friend SelfType operator%(SelfType a, PrimType b) { return a % SelfType(b); } \
    friend SelfType operator%(PrimType a, SelfType b) { return SelfType(a) % b; } \
    friend bool operator>(SelfType a, PrimType b) { return a > SelfType(b); } \
    friend bool operator>(PrimType a, SelfType b) { return SelfType(a) > b; } \
    friend bool operator<(SelfType a, PrimType b) { return a < SelfType(b); } \
    friend bool operator<(PrimType a, SelfType b) { return SelfType(a) < b; } \
    friend bool operator>=(SelfType a, PrimType b) { return a >= SelfType(b); } \
    friend bool operator>=(PrimType a, SelfType b) { return SelfType(a) >= b; } \
    friend bool operator<=(SelfType a, PrimType b) { return a <= SelfType(b); } \
    friend bool operator<=(PrimType a, SelfType b) { return SelfType(a) <= b; } \
    friend bool operator==(SelfType a, PrimType b) { return a == SelfType(b); } \
    friend bool operator==(PrimType a, SelfType b) { return SelfType(a) == b; } \
    friend bool operator!=(SelfType a, PrimType b) { return a != SelfType(b); } \
    friend bool operator!=(PrimType a, SelfType b) { return SelfType(a) != b; }

struct np_int128 {
    __int128 val;
    np_int128() : val(0) {}
    np_int128(__int128 v) : val(v) {}
    np_int128(const char* str);
    std::string to_string() const;
    operator double() const { return (double)val; }
    operator int64_t() const { return (int64_t)val; }
    operator __int128() const { return val; }
    friend std::ostream& operator<<(std::ostream& os, const np_int128& val) { os << val.to_string(); return os; }
    friend np_int128 operator+(np_int128 a, np_int128 b) { return a.val + b.val; }
    friend np_int128 operator-(np_int128 a, np_int128 b) { return a.val - b.val; }
    friend np_int128 operator*(np_int128 a, np_int128 b) { return a.val * b.val; }
    friend np_int128 operator/(np_int128 a, np_int128 b) { return a.val / b.val; }
    friend np_int128 operator%(np_int128 a, np_int128 b) { return a.val % b.val; }
    friend bool operator>(np_int128 a, np_int128 b) { return a.val > b.val; }
    friend bool operator<(np_int128 a, np_int128 b) { return a.val < b.val; }
    friend bool operator>=(np_int128 a, np_int128 b) { return a.val >= b.val; }
    friend bool operator<=(np_int128 a, np_int128 b) { return a.val <= b.val; }
    friend bool operator==(np_int128 a, np_int128 b) { return a.val == b.val; }
    friend bool operator!=(np_int128 a, np_int128 b) { return a.val != b.val; }
    NP_INT_PRIMITIVE_OPS(np_int128, int)
    NP_INT_PRIMITIVE_OPS(np_int128, int64_t)
};

struct np_int256 {
    unsigned __int128 low;
    __int128 high;
    np_int256() : low(0), high(0) {}
    np_int256(__int128 v) : low(v), high(v < 0 ? -1 : 0) {}
    np_int256(unsigned __int128 l, __int128 h) : low(l), high(h) {}
    np_int256(const char* str);
    unsigned int divmod10();
    std::string to_string() const;
    operator double() const { return (double)high * std::pow(2.0, 128) + (double)low; }
    operator int64_t() const { return (int64_t)low; }
    friend std::ostream& operator<<(std::ostream& os, const np_int256& val) { os << val.to_string(); return os; }
    friend np_int256 operator+(np_int256 a, np_int256 b);
    friend np_int256 operator-(np_int256 a, np_int256 b);
    static np_int256 multiply128(unsigned __int128 x, unsigned __int128 y);
    friend np_int256 operator*(np_int256 a, np_int256 b);
    friend np_int256 shift_left(np_int256 a, int shift);
    friend np_int256 shift_right_logical(np_int256 a, int shift);
    friend void divmod256(np_int256 num, np_int256 den, np_int256& quot, np_int256& rem);
    friend np_int256 operator/(np_int256 a, np_int256 b);
    friend np_int256 operator%(np_int256 a, np_int256 b);
    friend bool operator==(np_int256 a, np_int256 b) { return a.low == b.low && a.high == b.high; }
    friend bool operator!=(np_int256 a, np_int256 b) { return !(a == b); }
    friend bool operator<(np_int256 a, np_int256 b);
    friend bool operator>(np_int256 a, np_int256 b) { return b < a; }
    friend bool operator<=(np_int256 a, np_int256 b) { return !(b < a); }
    friend bool operator>=(np_int256 a, np_int256 b) { return !(a < b); }
    NP_INT_PRIMITIVE_OPS(np_int256, int)
    NP_INT_PRIMITIVE_OPS(np_int256, int64_t)
};

struct np_var {
    std::variant<int64_t, double, np_string, bool, ListPtr, DictPtr, np_int128, np_int256> v;
    np_var() : v(int64_t(0)) {}
    np_var(int val) : v(static_cast<int64_t>(val)) {}
    np_var(int64_t val) : v(val) {}
    np_var(double val) : v(val) {}
    np_var(const char* val) : v(np_string(val)) {}
    np_var(np_string val) : v(val) {}
    np_var(std::string val) : v(np_string(val)) {}
    np_var(bool val) : v(val) {}
    np_var(np_int128 val) : v(val) {}
    np_var(np_int256 val) : v(val) {}
    np_var(const std::vector<np_var>& val);
    np_var(const std::map<std::string, np_var>& val);
    np_var(ListPtr val) : v(val) {}
    np_var(DictPtr val) : v(val) {}
    
    operator int64_t() const;
    operator double() const;
    operator np_string() const;
    operator bool() const;
    operator np_int128() const;
    operator np_int256() const;
    np_string to_string() const;
    int length() const;
    np_var shape() const;
    bool has_key(const std::string& key) const;
    bool has_key(const char* key) const;
    void append(const np_var& val);
    np_var pop();
    void clear();
    np_var keys() const;
    np_var values() const;
    np_var slice(int start, int end) const;
    void sort();
    void reverse();
    bool contains(const np_var& val) const;
    np_var split(const np_var& delim) const;
    np_var join(const np_var& arr) const;
    np_var trim() const;
    using const_iterator = const np_var*;
    const_iterator begin() const;
    const_iterator end() const;
    friend std::ostream& operator<<(std::ostream& os, const np_var& var);
    np_var& operator[](int index);
    np_var& operator[](const std::string& key);
    np_var& operator[](const char* key);
    
    np_var operator+(const np_var& o) const;
    np_var operator^(const np_var& o) const;
#define NP_MATH_OP(op) np_var operator op(const np_var& o) const;
    NP_MATH_OP(-)
    NP_MATH_OP(*)
    NP_MATH_OP(/)
    NP_MATH_OP(%)
#define NP_CMP_OP(op) bool operator op(const np_var& o) const;
    NP_CMP_OP(>)
    NP_CMP_OP(<)
    NP_CMP_OP(>=)
    NP_CMP_OP(<=)
    NP_CMP_OP(==)
    NP_CMP_OP(!=)
};

struct np_var_list { std::vector<np_var> vec; np_var_list(const std::vector<np_var>& v) : vec(v) {} };
struct np_var_dict { std::map<std::string, np_var> map; np_var_dict(const std::map<std::string, np_var>& m) : map(m) {} };

inline np_var::np_var(const std::vector<np_var>& val) : v(std::make_shared<np_var_list>(val)) {}
inline np_var::np_var(const std::map<std::string, np_var>& val) : v(std::make_shared<np_var_dict>(val)) {}

np_int256 to_int256(const np_var& var);
np_int128 to_int128(const np_var& var);

inline np_var operator+(int64_t lhs, const np_var& rhs) { return np_var(lhs) + rhs; }
inline np_var operator+(int lhs, const np_var& rhs) { return np_var(static_cast<int64_t>(lhs)) + rhs; }
inline np_var operator+(double lhs, const np_var& rhs) { return np_var(lhs) + rhs; }
inline np_var operator+(const np_string& lhs, const np_var& rhs) { return np_var(lhs) + rhs; }
inline np_var operator+(const std::string& lhs, const np_var& rhs) { return np_var(lhs) + rhs; }
inline np_var operator+(const char* lhs, const np_var& rhs) { return np_var(lhs) + rhs; }
inline np_var operator+(const np_var& lhs, int64_t rhs) { return lhs + np_var(rhs); }
inline np_var operator+(const np_var& lhs, int rhs) { return lhs + np_var(static_cast<int64_t>(rhs)); }
inline np_var operator+(const np_var& lhs, double rhs) { return lhs + np_var(rhs); }
inline np_var operator+(const np_var& lhs, const np_string& rhs) { return lhs + np_var(rhs); }
inline np_var operator+(const np_var& lhs, const std::string& rhs) { return lhs + np_var(rhs); }
inline np_var operator+(const np_var& lhs, const char* rhs) { return lhs + np_var(rhs); }

#define NP_GLOBAL_MATH_OP_DECL(op) \
inline np_var operator op(int64_t lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \
inline np_var operator op(int lhs, const np_var& rhs) { return np_var(static_cast<int64_t>(lhs)) op rhs; } \
inline np_var operator op(double lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \
inline np_var operator op(const np_var& lhs, int64_t rhs) { return lhs op np_var(rhs); } \
inline np_var operator op(const np_var& lhs, int rhs) { return lhs op np_var(static_cast<int64_t>(rhs)); } \
inline np_var operator op(const np_var& lhs, double rhs) { return lhs op np_var(rhs); }

NP_GLOBAL_MATH_OP_DECL(-)
NP_GLOBAL_MATH_OP_DECL(*)
NP_GLOBAL_MATH_OP_DECL(/)
NP_GLOBAL_MATH_OP_DECL(%)
NP_GLOBAL_MATH_OP_DECL(^)

#define NP_GLOBAL_CMP_OP_DECL(op) \
inline bool operator op(int64_t lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \
inline bool operator op(int lhs, const np_var& rhs) { return np_var(static_cast<int64_t>(lhs)) op rhs; } \
inline bool operator op(double lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \
inline bool operator op(bool lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \
inline bool operator op(const np_string& lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \
inline bool operator op(const std::string& lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \
inline bool operator op(const char* lhs, const np_var& rhs) { return np_var(lhs) op rhs; } \
inline bool operator op(const np_var& lhs, int64_t rhs) { return lhs op np_var(rhs); } \
inline bool operator op(const np_var& lhs, int rhs) { return lhs op np_var(static_cast<int64_t>(rhs)); } \
inline bool operator op(const np_var& lhs, double rhs) { return lhs op np_var(rhs); } \
inline bool operator op(const np_var& lhs, bool rhs) { return lhs op np_var(rhs); } \
inline bool operator op(const np_var& lhs, const np_string& rhs) { return lhs op np_var(rhs); } \
inline bool operator op(const np_var& lhs, const std::string& rhs) { return lhs op np_var(rhs); } \
inline bool operator op(const np_var& lhs, const char* rhs) { return lhs op np_var(rhs); }

NP_GLOBAL_CMP_OP_DECL(>)
NP_GLOBAL_CMP_OP_DECL(<)
NP_GLOBAL_CMP_OP_DECL(>=)
NP_GLOBAL_CMP_OP_DECL(<=)
NP_GLOBAL_CMP_OP_DECL(==)
NP_GLOBAL_CMP_OP_DECL(!=)

template<typename T, typename U> auto np_pow_call(T lhs, U rhs) {
    if constexpr (std::is_same_v<T, np_var> || std::is_same_v<U, np_var>) return np_var(lhs) ^ np_var(rhs);
    else if constexpr (std::is_floating_point_v<T> || std::is_floating_point_v<U>) return std::pow(static_cast<double>(lhs), static_cast<double>(rhs));
    else return static_cast<int64_t>(std::pow(static_cast<double>(lhs), static_cast<double>(rhs)));
}
struct np_pow_helper_t {};
inline np_pow_helper_t np_pow_helper;
template<typename LHS> struct np_pow_proxy { LHS lhs; template<typename RHS> auto operator*(RHS rhs) const { return np_pow_call(lhs, rhs); } };
template<typename LHS> np_pow_proxy<LHS> operator*(LHS lhs, np_pow_helper_t) { return np_pow_proxy<LHS>{lhs}; }

template<typename T> inline double np_sqrt(T v) { return std::sqrt(static_cast<double>(v)); }
inline double np_sqrt(const np_var& v) { return std::sqrt(static_cast<double>(v)); }

template<typename T> inline auto np_abs(T v) { return std::abs(v); }
inline np_var np_abs(const np_var& v) {
    if (std::holds_alternative<double>(v.v)) return std::abs(std::get<double>(v.v));
    if (std::holds_alternative<int64_t>(v.v)) return std::abs(std::get<int64_t>(v.v));
    return v;
}

template<typename T, typename U> auto np_min(T a, U b) {
    if constexpr (std::is_same_v<T, np_var> || std::is_same_v<U, np_var>) return np_var(a) < np_var(b) ? np_var(a) : np_var(b);
    else return a < b ? a : b;
}
template<typename T, typename U> auto np_max(T a, U b) {
    if constexpr (std::is_same_v<T, np_var> || std::is_same_v<U, np_var>) return np_var(a) > np_var(b) ? np_var(a) : np_var(b);
    else return a > b ? a : b;
}

template<typename T> inline double np_round(T v) { return std::round(static_cast<double>(v)); }
inline double np_round(const np_var& v) { return std::round(static_cast<double>(v)); }

inline int np_len(const std::string& s) { return s.length(); }
inline int np_len(const char* s) { return std::string(s).length(); }
inline int np_len(const np_var& v) { return v.length(); }

std::string np_read_file(const std::string& filename);
inline std::string np_read_file(const np_var& filename) { return np_read_file(static_cast<std::string>(filename)); }

int np_write_file(const std::string& filename, const std::string& content);
inline int np_write_file(const np_var& filename, const np_var& content) { return np_write_file(static_cast<std::string>(filename), static_cast<std::string>(content)); }

template<typename T> void np_print(T val) { std::cout << val << std::endl; }
template<typename T, typename... Args> void np_print(T val, Args... args) { std::cout << val << " "; np_print(args...); }

struct np_input_proxy {
    template<typename T> operator T() { T val; std::cin >> val; std::cin.ignore(10000, '\n'); return val; }
    operator std::string() { std::string s; std::getline(std::cin, s); return s; }
};
#define np_input() np_input_proxy()

int np_to_int(const std::string& s);
int np_to_int(float v);
float np_to_float(const std::string& s);
float np_to_float(int64_t v);
std::string np_to_string(int v);
std::string np_to_string(int64_t v);
std::string np_to_string(double v);

int np_to_int(const np_var& v);
float np_to_float(const np_var& v);
std::string np_to_string(const np_var& v);

std::string np_to_string(const np_int128& v);
std::string np_to_string(const np_int256& v);
int np_to_int(const np_int128& v);
int np_to_int(const np_int256& v);
float np_to_float(const np_int128& v);
float np_to_float(const np_int256& v);

template <typename T> std::string np_type(T) { return "unknown"; }
template <> inline std::string np_type(int) { return "int32"; }
template <> inline std::string np_type(int64_t) { return "int64"; }
template <> inline std::string np_type(float) { return "float32"; }
template <> inline std::string np_type(double) { return "float64"; }
template <> inline std::string np_type(std::string) { return "string"; }
template <> inline std::string np_type(np_string) { return "string"; }
template <> inline std::string np_type(bool) { return "bool"; }
template <> inline std::string np_type(np_int128) { return "int128"; }
template <> inline std::string np_type(np_int256) { return "int256"; }
template <> inline std::string np_type(np_var v) {
    if(std::holds_alternative<int64_t>(v.v)) return "int";
    if(std::holds_alternative<double>(v.v)) return "float";
    if(std::holds_alternative<np_string>(v.v)) return "string";
    if(std::holds_alternative<bool>(v.v)) return "bool";
    if(std::holds_alternative<ListPtr>(v.v)) return "array";
    if(std::holds_alternative<DictPtr>(v.v)) return "dict";
    if(std::holds_alternative<np_int128>(v.v)) return "int128";
    if(std::holds_alternative<np_int256>(v.v)) return "int256";
    return "unknown";
}

// sys, time, json, regex module helpers
extern np_var np_sys_argv;
void np_init_args(int argc, char* argv[]);
double np_time_now();
void np_time_sleep(double secs);
inline void np_time_sleep(const np_var& secs) { np_time_sleep(static_cast<double>(secs)); }
std::string np_time_format(double ts, const std::string& fmt);
inline std::string np_time_format(const np_var& ts, const np_var& fmt) { return np_time_format(static_cast<double>(ts), static_cast<std::string>(fmt)); }
std::string np_json_stringify(const np_var& val);
np_var np_json_parse(const std::string& src);
inline np_var np_json_parse(const np_var& src) { return np_json_parse(static_cast<std::string>(src)); }
bool np_regex_match(const std::string& pattern, const std::string& text);
inline bool np_regex_match(const np_var& pattern, const np_var& text) { return np_regex_match(static_cast<std::string>(pattern), static_cast<std::string>(text)); }
std::string np_regex_find(const std::string& pattern, const std::string& text);
inline std::string np_regex_find(const np_var& pattern, const np_var& text) { return np_regex_find(static_cast<std::string>(pattern), static_cast<std::string>(text)); }
std::string np_regex_replace(const std::string& pattern, const std::string& repl, const std::string& text);
inline std::string np_regex_replace(const np_var& pattern, const np_var& repl, const np_var& text) { return np_regex_replace(static_cast<std::string>(pattern), static_cast<std::string>(repl), static_cast<std::string>(text)); }


// ==========================================
// C-Compatible extern "C" Runtime API for LLVM IR
// ==========================================
extern "C" {
    // String API
    void* np_rt_string_create(const char* s);
    void np_rt_string_destroy(void* s);
    void* np_rt_string_concat(void* s1, void* s2);
    void* np_rt_string_concat_char(void* s1, const char* s2);
    void* np_rt_string_concat_char_lhs(const char* s1, void* s2);
    const char* np_rt_string_c_str(void* s);
    int64_t np_rt_string_len(void* s);
    void* np_rt_string_slice(void* s, int64_t start, int64_t end);
    bool np_rt_string_eq(void* s1, void* s2);
    bool np_rt_string_lt(void* s1, void* s2);
    bool np_rt_string_contains(void* s, void* sub);

    // np_var API (for lists, dicts, dynamic values)
    void* np_rt_var_create_int(int64_t v);
    void* np_rt_var_create_float(double v);
    void* np_rt_var_create_string(void* s);
    void* np_rt_var_create_bool(bool v);
    void* np_rt_var_create_list();
    void* np_rt_var_create_dict();
    void* np_rt_var_create_int128(const char* s);
    void* np_rt_var_create_int256(const char* s);
    void np_rt_var_destroy(void* v);
    
    // List/Dict actions
    void np_rt_var_append(void* list_var, void* val_var);
    void* np_rt_var_pop(void* list_var);
    void* np_rt_var_get_index(void* list_var, int64_t index);
    void np_rt_var_set_index(void* list_var, int64_t index, void* val_var);
    void* np_rt_var_get_key(void* dict_var, const char* key);
    void np_rt_var_set_key(void* dict_var, const char* key, void* val_var);
    int64_t np_rt_var_len(void* v);
    void* np_rt_var_slice(void* v, int64_t start, int64_t end);

    // np_var Operators
    void* np_rt_var_add(void* lhs, void* rhs);
    void* np_rt_var_sub(void* lhs, void* rhs);
    void* np_rt_var_mul(void* lhs, void* rhs);
    void* np_rt_var_div(void* lhs, void* rhs);
    void* np_rt_var_mod(void* lhs, void* rhs);
    void* np_rt_var_pow(void* lhs, void* rhs);
    bool np_rt_var_gt(void* lhs, void* rhs);
    bool np_rt_var_lt(void* lhs, void* rhs);
    bool np_rt_var_ge(void* lhs, void* rhs);
    bool np_rt_var_le(void* lhs, void* rhs);
    bool np_rt_var_eq(void* lhs, void* rhs);
    bool np_rt_var_ne(void* lhs, void* rhs);

    // Standard Print API
    void np_rt_print_int(int64_t v);
    void np_rt_print_float(double v);
    void np_rt_print_bool(bool v);
    void np_rt_print_string(void* s);
    void np_rt_print_var(void* v);

    // Conversion API
    int64_t np_rt_to_int_string(void* s);
    int64_t np_rt_to_int_var(void* v);
    double np_rt_to_float_string(void* s);
    double np_rt_to_float_var(void* v);
    void* np_rt_to_string_int(int64_t v);
    void* np_rt_to_string_float(double v);
    void* np_rt_to_string_var(void* v);

    // Input API
    int64_t np_rt_input_int();
    double np_rt_input_float();
    void* np_rt_input_string();

    // Module functions
    void np_rt_sys_init_args(int argc, char* argv[]);
    void* np_rt_sys_get_argv();
    double np_rt_time_now();
    void np_rt_time_sleep(double secs);
    void* np_rt_time_format(double ts, void* fmt);
    void* np_rt_json_stringify(void* v);
    void* np_rt_json_parse(void* s);
    bool np_rt_regex_match(void* pattern, void* text);
    void* np_rt_regex_find(void* pattern, void* text);
    void* np_rt_regex_replace(void* pattern, void* repl, void* text);
    void* np_rt_type_var(void* v);
}
