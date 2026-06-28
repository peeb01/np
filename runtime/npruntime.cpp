#include "npruntime.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cmath>
#include <cstdlib>

// ==========================================
// 1. Core runtime structures & helpers
// ==========================================

int np_throw_error(const std::string& msg) {
    std::cerr << msg << std::endl;
    std::exit(1);
    return 0;
}

// np_string implementation
np_string np_string::slice(int start, int end) const {
    int len = this->length();
    if (end == -999999) end = len;
    if (start < 0) start += len; if (start < 0) start = 0; if (start > len) start = len;
    if (end < 0) end += len; if (end < 0) end = 0; if (end > len) end = len;
    if (end < start) end = start;
    return this->substr(start, end - start);
}

np_var np_string::split(const std::string& delim) const {
    std::vector<np_var> res;
    size_t start = 0;
    size_t end = this->find(delim);
    while (end != std::string::npos) {
        res.push_back(this->substr(start, end - start));
        start = end + delim.length();
        end = this->find(delim, start);
    }
    res.push_back(this->substr(start));
    return np_var(res);
}

np_string np_string::join(const np_var& arr) const {
    std::string res = "";
    if (std::holds_alternative<ListPtr>(arr.v) && std::get<ListPtr>(arr.v)) {
        auto const& vec = std::get<ListPtr>(arr.v)->vec;
        for (size_t i = 0; i < vec.size(); ++i) {
            res += static_cast<std::string>(vec[i]);
            if (i < vec.size() - 1) res += *this;
        }
    }
    return np_string(res);
}

np_string np_string::trim() const {
    size_t first = this->find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = this->find_last_not_of(" \t\r\n");
    return this->substr(first, last - first + 1);
}

// np_int128 implementation
np_int128::np_int128(const char* str) {
    val = 0;
    bool neg = false;
    if (*str == '-') { neg = true; str++; }
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            val = val * 10 + (*str - '0');
        }
        str++;
    }
    if (neg) val = -val;
}

std::string np_int128::to_string() const {
    if (val == 0) return "0";
    std::string s = "";
    __int128 temp = val;
    bool neg = false;
    if (temp < 0) { neg = true; temp = -temp; }
    while (temp > 0) {
        s += (char)('0' + (temp % 10));
        temp /= 10;
    }
    if (neg) s += "-";
    std::reverse(s.begin(), s.end());
    return s;
}

// np_int256 implementation
np_int256::np_int256(const char* str) {
    low = 0; high = 0;
    bool neg = false;
    if (*str == '-') { neg = true; str++; }
    while (*str) {
        if (*str >= '0' && *str <= '9') {
            *this = *this * 10 + np_int256(static_cast<__int128>(*str - '0'));
        }
        str++;
    }
    if (neg) *this = np_int256() - *this;
}

unsigned int np_int256::divmod10() {
    unsigned int words[8];
    words[0] = (unsigned int)(low & 0xFFFFFFFF);
    words[1] = (unsigned int)((low >> 32) & 0xFFFFFFFF);
    words[2] = (unsigned int)((low >> 64) & 0xFFFFFFFF);
    words[3] = (unsigned int)((low >> 96) & 0xFFFFFFFF);
    words[4] = (unsigned int)(high & 0xFFFFFFFF);
    words[5] = (unsigned int)((high >> 32) & 0xFFFFFFFF);
    words[6] = (unsigned int)((high >> 64) & 0xFFFFFFFF);
    words[7] = (unsigned int)((high >> 96) & 0xFFFFFFFF);
    unsigned long long carry = 0;
    for (int i = 7; i >= 0; --i) {
        unsigned long long cur = words[i] + (carry << 32);
        words[i] = cur / 10;
        carry = cur % 10;
    }
    low = ((unsigned __int128)words[3] << 96) | ((unsigned __int128)words[2] << 64) | ((unsigned __int128)words[1] << 32) | words[0];
    high = ((__int128)words[7] << 96) | ((__int128)words[6] << 64) | ((__int128)words[5] << 32) | words[4];
    return (unsigned int)carry;
}

std::string np_int256::to_string() const {
    if (high == 0 && low == 0) return "0";
    bool neg = false;
    np_int256 temp = *this;
    if (high < 0) { neg = true; temp = np_int256() - temp; }
    std::string s = "";
    while (temp.high > 0 || temp.low > 0) {
        s += (char)('0' + temp.divmod10());
    }
    if (neg) s += "-";
    std::reverse(s.begin(), s.end());
    return s;
}

np_int256 operator+(np_int256 a, np_int256 b) {
    unsigned __int128 low = a.low + b.low;
    __int128 high = a.high + b.high;
    if (low < a.low) high += 1;
    return np_int256(low, high);
}

np_int256 operator-(np_int256 a, np_int256 b) {
    unsigned __int128 low = a.low - b.low;
    __int128 high = a.high - b.high;
    if (a.low < b.low) high -= 1;
    return np_int256(low, high);
}

np_int256 np_int256::multiply128(unsigned __int128 x, unsigned __int128 y) {
    uint64_t x1 = x >> 64, x0 = x & 0xFFFFFFFFFFFFFFFFULL;
    uint64_t y1 = y >> 64, y0 = y & 0xFFFFFFFFFFFFFFFFULL;
    unsigned __int128 p00 = (unsigned __int128)x0 * y0;
    unsigned __int128 p01 = (unsigned __int128)x0 * y1;
    unsigned __int128 p10 = (unsigned __int128)x1 * y0;
    unsigned __int128 p11 = (unsigned __int128)x1 * y1;
    unsigned __int128 middle = p01 + p10;
    unsigned __int128 carry = (middle < p01) ? ((unsigned __int128)1 << 64) : 0;
    unsigned __int128 low = p00 + (middle << 64);
    if (low < p00) carry += 1;
    __int128 high = p11 + (middle >> 64) + carry;
    return np_int256(low, high);
}

np_int256 operator*(np_int256 a, np_int256 b) {
    np_int256 res = np_int256::multiply128(a.low, b.low);
    res.high += (a.high * b.low) + (a.low * b.high);
    return res;
}

np_int256 shift_left(np_int256 a, int shift) {
    if (shift <= 0) return a;
    if (shift >= 256) return np_int256(0, 0);
    if (shift >= 128) return np_int256(0, a.low << (shift - 128));
    unsigned __int128 low = a.low << shift;
    __int128 high = (a.high << shift) | (a.low >> (128 - shift));
    return np_int256(low, high);
}

np_int256 shift_right_logical(np_int256 a, int shift) {
    if (shift <= 0) return a;
    if (shift >= 256) return np_int256(0, 0);
    unsigned __int128 a_high = (unsigned __int128)a.high;
    if (shift >= 128) return np_int256(a_high >> (shift - 128), 0);
    unsigned __int128 low = (a.low >> shift) | (a_high << (128 - shift));
    __int128 high = a_high >> shift;
    return np_int256(low, high);
}

void divmod256(np_int256 num, np_int256 den, np_int256& quot, np_int256& rem) {
    if (den.high == 0 && den.low == 0) { quot = np_int256(0,0); rem = np_int256(0,0); return; }
    bool num_neg = num.high < 0;
    bool den_neg = den.high < 0;
    np_int256 u_num = num_neg ? (np_int256(0,0) - num) : num;
    np_int256 u_den = den_neg ? (np_int256(0,0) - den) : den;
    np_int256 q(0,0);
    np_int256 r(0,0);
    for (int i = 255; i >= 0; --i) {
        r = shift_left(r, 1);
        unsigned int bit = 0;
        if (i >= 128) bit = (unsigned int)((u_num.high >> (i - 128)) & 1);
        else bit = (unsigned int)((u_num.low >> i) & 1);
        r.low |= bit;
        if (r >= u_den) {
            r = r - u_den;
            if (i >= 128) q.high |= ((__int128)1 << (i - 128));
            else q.low |= ((unsigned __int128)1 << i);
        }
    }
    if (num_neg ^ den_neg) quot = np_int256(0,0) - q;
    else quot = q;
    if (num_neg) rem = np_int256(0,0) - r;
    else rem = r;
}

np_int256 operator/(np_int256 a, np_int256 b) { np_int256 q, r; divmod256(a, b, q, r); return q; }
np_int256 operator%(np_int256 a, np_int256 b) { np_int256 q, r; divmod256(a, b, q, r); return r; }

bool operator<(np_int256 a, np_int256 b) {
    if (a.high != b.high) return a.high < b.high;
    return a.low < b.low;
}

// Helper converters for np_var variant
np_int256 to_int256(const np_var& var) {
    if (std::holds_alternative<np_int256>(var.v)) return std::get<np_int256>(var.v);
    if (std::holds_alternative<np_int128>(var.v)) return np_int256(std::get<np_int128>(var.v));
    if (std::holds_alternative<int64_t>(var.v)) return np_int256(std::get<int64_t>(var.v));
    if (std::holds_alternative<double>(var.v)) return np_int256(static_cast<int64_t>(std::get<double>(var.v)));
    if (std::holds_alternative<np_string>(var.v)) return np_int256(std::get<np_string>(var.v).c_str());
    return np_int256();
}

np_int128 to_int128(const np_var& var) {
    if (std::holds_alternative<np_int128>(var.v)) return std::get<np_int128>(var.v);
    if (std::holds_alternative<np_int256>(var.v)) return np_int128(std::get<np_int256>(var.v).low);
    if (std::holds_alternative<int64_t>(var.v)) return np_int128(std::get<int64_t>(var.v));
    if (std::holds_alternative<double>(var.v)) return np_int128(static_cast<int64_t>(std::get<double>(var.v)));
    if (std::holds_alternative<np_string>(var.v)) return np_int128(std::get<np_string>(var.v).c_str());
    return np_int128();
}

// np_var implementation
np_var::operator int64_t() const {
    if(std::holds_alternative<int64_t>(v)) return std::get<int64_t>(v);
    if(std::holds_alternative<double>(v)) return static_cast<int64_t>(std::get<double>(v));
    if(std::holds_alternative<np_int128>(v)) return static_cast<int64_t>(std::get<np_int128>(v));
    if(std::holds_alternative<np_int256>(v)) return static_cast<int64_t>(std::get<np_int256>(v).low);
    return 0;
}

np_var::operator double() const {
    if(std::holds_alternative<double>(v)) return std::get<double>(v);
    if(std::holds_alternative<int64_t>(v)) return static_cast<double>(std::get<int64_t>(v));
    if(std::holds_alternative<np_int128>(v)) return static_cast<double>(std::get<np_int128>(v));
    if(std::holds_alternative<np_int256>(v)) return static_cast<double>(std::get<np_int256>(v).low);
    return 0.0;
}

np_var::operator np_string() const { return np_string(to_string()); }
np_var::operator bool() const {
    if(std::holds_alternative<bool>(v)) return std::get<bool>(v);
    return false;
}
np_var::operator np_int128() const { return to_int128(*this); }
np_var::operator np_int256() const { return to_int256(*this); }

np_string np_var::to_string() const {
    if (std::holds_alternative<int64_t>(v)) return std::to_string(std::get<int64_t>(v));
    if (std::holds_alternative<double>(v)) {
        std::string s = std::to_string(std::get<double>(v));
        s.erase(s.find_last_not_of('0') + 1, std::string::npos);
        if (s.back() == '.') s += "0";
        return s;
    }
    if (std::holds_alternative<np_string>(v)) return std::get<np_string>(v);
    if (std::holds_alternative<bool>(v)) return std::get<bool>(v) ? "true" : "false";
    if (std::holds_alternative<np_int128>(v)) return std::get<np_int128>(v).to_string();
    if (std::holds_alternative<np_int256>(v)) return std::get<np_int256>(v).to_string();
    if (std::holds_alternative<ListPtr>(v)) {
        std::string s = "[";
        auto arr = std::get<ListPtr>(v);
        if (arr) {
            for (size_t i = 0; i < arr->vec.size(); ++i) {
                s += arr->vec[i].to_string();
                if (i < arr->vec.size() - 1) s += ", ";
            }
        }
        return s + "]";
    }
    if (std::holds_alternative<DictPtr>(v)) {
        std::string s = "{";
        auto dict = std::get<DictPtr>(v);
        if (dict) {
            bool first = true;
            for (auto const& [k, val] : dict->map) {
                if (!first) s += ", ";
                s += "\"" + k + "\": " + static_cast<std::string>(val.to_string());
                first = false;
            }
        }
        return s + "}";
    }
    return "";
}

int np_var::length() const {
    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) return std::get<ListPtr>(v)->vec.size();
    if (std::holds_alternative<DictPtr>(v) && std::get<DictPtr>(v)) return std::get<DictPtr>(v)->map.size();
    if (std::holds_alternative<np_string>(v)) return std::get<np_string>(v).length();
    return 0;
}

np_var np_var::shape() const {
    if (!std::holds_alternative<ListPtr>(v) || !std::get<ListPtr>(v)) return std::vector<np_var>{};
    std::vector<np_var> dims;
    np_var current = *this;
    while (std::holds_alternative<ListPtr>(current.v) && std::get<ListPtr>(current.v)) {
        auto& vec = std::get<ListPtr>(current.v)->vec;
        dims.push_back(static_cast<int>(vec.size()));
        if (vec.empty()) break;
        current = vec[0];
    }
    return dims;
}

bool np_var::has_key(const std::string& key) const {
    if (std::holds_alternative<DictPtr>(v) && std::get<DictPtr>(v)) return std::get<DictPtr>(v)->map.count(key) > 0;
    return false;
}
bool np_var::has_key(const char* key) const { return has_key(std::string(key)); }

void np_var::append(const np_var& val) {
    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) std::get<ListPtr>(v)->vec.push_back(val);
}

np_var np_var::pop() {
    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) {
        auto& vec = std::get<ListPtr>(v)->vec;
        if (!vec.empty()) { np_var last = vec.back(); vec.pop_back(); return last; }
    }
    return np_var();
}

void np_var::clear() {
    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) std::get<ListPtr>(v)->vec.clear();
    else if (std::holds_alternative<DictPtr>(v) && std::get<DictPtr>(v)) std::get<DictPtr>(v)->map.clear();
}

np_var np_var::keys() const {
    if (std::holds_alternative<DictPtr>(v) && std::get<DictPtr>(v)) {
        std::vector<np_var> k_vec;
        for (auto const& [k, val] : std::get<DictPtr>(v)->map) k_vec.push_back(k);
        return np_var(k_vec);
    }
    return np_var(std::vector<np_var>{});
}

np_var np_var::values() const {
    if (std::holds_alternative<DictPtr>(v) && std::get<DictPtr>(v)) {
        std::vector<np_var> v_vec;
        for (auto const& [k, val] : std::get<DictPtr>(v)->map) v_vec.push_back(val);
        return np_var(v_vec);
    }
    return np_var(std::vector<np_var>{});
}

np_var np_var::slice(int start, int end) const {
    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) {
        auto const& vec = std::get<ListPtr>(v)->vec;
        int len = vec.size();
        if (end == -999999) end = len;
        if (start < 0) start += len; if (start < 0) start = 0; if (start > len) start = len;
        if (end < 0) end += len; if (end < 0) end = 0; if (end > len) end = len;
        if (end < start) end = start;
        std::vector<np_var> res_vec;
        for (int i = start; i < end; ++i) res_vec.push_back(vec[i]);
        return np_var(res_vec);
    }
    if (std::holds_alternative<np_string>(v)) {
        np_string s = std::get<np_string>(v);
        int len = s.length();
        if (end == -999999) end = len;
        if (start < 0) start += len; if (start < 0) start = 0; if (start > len) start = len;
        if (end < 0) end += len; if (end < 0) end = 0; if (end > len) end = len;
        if (end < start) end = start;
        return s.substr(start, end - start);
    }
    return np_var();
}

np_var::const_iterator np_var::begin() const {
    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) return std::get<ListPtr>(v)->vec.data();
    return nullptr;
}

np_var::const_iterator np_var::end() const {
    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) return std::get<ListPtr>(v)->vec.data() + std::get<ListPtr>(v)->vec.size();
    return nullptr;
}

std::ostream& operator<<(std::ostream& os, const np_var& var) { os << var.to_string(); return os; }

np_var& np_var::operator[](int index) { return std::get<ListPtr>(v)->vec[index]; }
np_var& np_var::operator[](const std::string& key) { return std::get<DictPtr>(v)->map[key]; }
np_var& np_var::operator[](const char* key) { return std::get<DictPtr>(v)->map[std::string(key)]; }

np_var np_var::operator+(const np_var& o) const {
    if (std::holds_alternative<np_int256>(v) || std::holds_alternative<np_int256>(o.v)) return np_var(to_int256(*this) + to_int256(o));
    if (std::holds_alternative<np_int128>(v) || std::holds_alternative<np_int128>(o.v)) return np_var(to_int128(*this) + to_int128(o));
    if(std::holds_alternative<ListPtr>(v) && std::holds_alternative<ListPtr>(o.v)) {
        auto a_ptr = std::get<ListPtr>(v); auto b_ptr = std::get<ListPtr>(o.v);
        if (a_ptr && b_ptr) {
            std::vector<np_var> res_vec; size_t min_sz = std::min(a_ptr->vec.size(), b_ptr->vec.size());
            for (size_t i = 0; i < min_sz; ++i) res_vec.push_back(a_ptr->vec[i] + b_ptr->vec[i]);
            return np_var(res_vec);
        }
    }
    if (std::holds_alternative<ListPtr>(v) && !std::holds_alternative<ListPtr>(o.v) && !std::holds_alternative<DictPtr>(o.v)) {
        auto a_ptr = std::get<ListPtr>(v);
        if (a_ptr) {
            std::vector<np_var> res_vec;
            for (size_t i = 0; i < a_ptr->vec.size(); ++i) res_vec.push_back(a_ptr->vec[i] + o);
            return np_var(res_vec);
        }
    }
    if (!std::holds_alternative<ListPtr>(v) && !std::holds_alternative<DictPtr>(v) && std::holds_alternative<ListPtr>(o.v)) {
        auto b_ptr = std::get<ListPtr>(o.v);
        if (b_ptr) {
            std::vector<np_var> res_vec;
            for (size_t i = 0; i < b_ptr->vec.size(); ++i) res_vec.push_back((*this) + b_ptr->vec[i]);
            return np_var(res_vec);
        }
    }
    if(std::holds_alternative<np_string>(v) || std::holds_alternative<np_string>(o.v)) return to_string() + o.to_string();
    if(std::holds_alternative<int64_t>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<int64_t>(v) + std::get<int64_t>(o.v);
    if(std::holds_alternative<double>(v) && std::holds_alternative<double>(o.v)) return std::get<double>(v) + std::get<double>(o.v);
    if(std::holds_alternative<int64_t>(v) && std::holds_alternative<double>(o.v)) return std::get<int64_t>(v) + std::get<double>(o.v);
    if(std::holds_alternative<double>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<double>(v) + std::get<int64_t>(o.v);
    return 0;
}

np_var np_var::operator^(const np_var& o) const {
    if(std::holds_alternative<ListPtr>(v) && std::holds_alternative<ListPtr>(o.v)) {
        auto a_ptr = std::get<ListPtr>(v); auto b_ptr = std::get<ListPtr>(o.v);
        if (a_ptr && b_ptr) {
            std::vector<np_var> res_vec; size_t min_sz = std::min(a_ptr->vec.size(), b_ptr->vec.size());
            for (size_t i = 0; i < min_sz; ++i) res_vec.push_back(a_ptr->vec[i] ^ b_ptr->vec[i]);
            return np_var(res_vec);
        }
    }
    if (std::holds_alternative<ListPtr>(v) && !std::holds_alternative<ListPtr>(o.v) && !std::holds_alternative<DictPtr>(o.v)) {
        auto a_ptr = std::get<ListPtr>(v);
        if (a_ptr) {
            std::vector<np_var> res_vec;
            for (size_t i = 0; i < a_ptr->vec.size(); ++i) res_vec.push_back(a_ptr->vec[i] ^ o);
            return np_var(res_vec);
        }
    }
    if (!std::holds_alternative<ListPtr>(v) && !std::holds_alternative<DictPtr>(v) && std::holds_alternative<ListPtr>(o.v)) {
        auto b_ptr = std::get<ListPtr>(o.v);
        if (b_ptr) {
            std::vector<np_var> res_vec;
            for (size_t i = 0; i < b_ptr->vec.size(); ++i) res_vec.push_back((*this) ^ b_ptr->vec[i]);
            return np_var(res_vec);
        }
    }
    if(std::holds_alternative<int64_t>(v) && std::holds_alternative<int64_t>(o.v)) return static_cast<int64_t>(std::pow(std::get<int64_t>(v), std::get<int64_t>(o.v)));
    if(std::holds_alternative<double>(v) && std::holds_alternative<double>(o.v)) return std::pow(std::get<double>(v), std::get<double>(o.v));
    if(std::holds_alternative<int64_t>(v) && std::holds_alternative<double>(o.v)) return std::pow(std::get<int64_t>(v), std::get<double>(o.v));
    if(std::holds_alternative<double>(v) && std::holds_alternative<int64_t>(o.v)) return std::pow(std::get<double>(v), std::get<int64_t>(o.v));
    return 0;
}

#define NP_MATH_OP_IMPL(op) np_var np_var::operator op(const np_var& o) const { \
    if (std::holds_alternative<np_int256>(v) || std::holds_alternative<np_int256>(o.v)) return np_var(to_int256(*this) op to_int256(o)); \
    if (std::holds_alternative<np_int128>(v) || std::holds_alternative<np_int128>(o.v)) return np_var(to_int128(*this) op to_int128(o)); \
    if(std::holds_alternative<ListPtr>(v) && std::holds_alternative<ListPtr>(o.v)) { \
        auto a_ptr = std::get<ListPtr>(v); auto b_ptr = std::get<ListPtr>(o.v); \
        if (a_ptr && b_ptr) { \
            std::vector<np_var> res_vec; size_t min_sz = std::min(a_ptr->vec.size(), b_ptr->vec.size()); \
            for (size_t i = 0; i < min_sz; ++i) res_vec.push_back(a_ptr->vec[i] op b_ptr->vec[i]); \
            return np_var(res_vec); \
        } \
    } \
    if (std::holds_alternative<ListPtr>(v) && !std::holds_alternative<ListPtr>(o.v) && !std::holds_alternative<DictPtr>(o.v)) { \
        auto a_ptr = std::get<ListPtr>(v); \
        if (a_ptr) { \
            std::vector<np_var> res_vec; \
            for (size_t i = 0; i < a_ptr->vec.size(); ++i) res_vec.push_back(a_ptr->vec[i] op o); \
            return np_var(res_vec); \
        } \
    } \
    if (!std::holds_alternative<ListPtr>(v) && !std::holds_alternative<DictPtr>(v) && std::holds_alternative<ListPtr>(o.v)) { \
        auto b_ptr = std::get<ListPtr>(o.v); \
        if (b_ptr) { \
            std::vector<np_var> res_vec; \
            for (size_t i = 0; i < b_ptr->vec.size(); ++i) res_vec.push_back((*this) op b_ptr->vec[i]); \
            return np_var(res_vec); \
        } \
    } \
    if(std::holds_alternative<int64_t>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<int64_t>(v) op std::get<int64_t>(o.v); \
    if(std::holds_alternative<double>(v) && std::holds_alternative<double>(o.v)) return std::get<double>(v) op std::get<double>(o.v); \
    if(std::holds_alternative<int64_t>(v) && std::holds_alternative<double>(o.v)) return std::get<int64_t>(v) op std::get<double>(o.v); \
    if(std::holds_alternative<double>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<double>(v) op std::get<int64_t>(o.v); \
    return 0; \
}
NP_MATH_OP_IMPL(-)
NP_MATH_OP_IMPL(*)
NP_MATH_OP_IMPL(/)

np_var np_var::operator%(const np_var& o) const {
    if (std::holds_alternative<np_int256>(v) || std::holds_alternative<np_int256>(o.v)) return np_var(to_int256(*this) % to_int256(o));
    if (std::holds_alternative<np_int128>(v) || std::holds_alternative<np_int128>(o.v)) return np_var(to_int128(*this) % to_int128(o));
    if(std::holds_alternative<ListPtr>(v) && std::holds_alternative<ListPtr>(o.v)) {
        auto a_ptr = std::get<ListPtr>(v); auto b_ptr = std::get<ListPtr>(o.v);
        if (a_ptr && b_ptr) {
            std::vector<np_var> res_vec; size_t min_sz = std::min(a_ptr->vec.size(), b_ptr->vec.size());
            for (size_t i = 0; i < min_sz; ++i) res_vec.push_back(a_ptr->vec[i] % b_ptr->vec[i]);
            return np_var(res_vec);
        }
    }
    if (std::holds_alternative<ListPtr>(v) && !std::holds_alternative<ListPtr>(o.v) && !std::holds_alternative<DictPtr>(o.v)) {
        auto a_ptr = std::get<ListPtr>(v);
        if (a_ptr) {
            std::vector<np_var> res_vec;
            for (size_t i = 0; i < a_ptr->vec.size(); ++i) res_vec.push_back(a_ptr->vec[i] % o);
            return np_var(res_vec);
        }
    }
    if (!std::holds_alternative<ListPtr>(v) && !std::holds_alternative<DictPtr>(v) && std::holds_alternative<ListPtr>(o.v)) {
        auto b_ptr = std::get<ListPtr>(o.v);
        if (b_ptr) {
            std::vector<np_var> res_vec;
            for (size_t i = 0; i < b_ptr->vec.size(); ++i) res_vec.push_back((*this) % b_ptr->vec[i]);
            return np_var(res_vec);
        }
    }
    if(std::holds_alternative<int64_t>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<int64_t>(v) % std::get<int64_t>(o.v);
    if(std::holds_alternative<double>(v) && std::holds_alternative<double>(o.v)) return std::fmod(std::get<double>(v), std::get<double>(o.v));
    if(std::holds_alternative<int64_t>(v) && std::holds_alternative<double>(o.v)) return std::fmod(static_cast<double>(std::get<int64_t>(v)), std::get<double>(o.v));
    if(std::holds_alternative<double>(v) && std::holds_alternative<int64_t>(o.v)) return std::fmod(std::get<double>(v), static_cast<double>(std::get<int64_t>(o.v)));
    return 0;
}

#define NP_CMP_OP_IMPL(op) bool np_var::operator op(const np_var& o) const { \
    if (std::holds_alternative<np_int256>(v) || std::holds_alternative<np_int256>(o.v)) return to_int256(*this) op to_int256(o); \
    if (std::holds_alternative<np_int128>(v) || std::holds_alternative<np_int128>(o.v)) return to_int128(*this) op to_int128(o); \
    if(std::holds_alternative<int64_t>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<int64_t>(v) op std::get<int64_t>(o.v); \
    if(std::holds_alternative<double>(v) && std::holds_alternative<double>(o.v)) return std::get<double>(v) op std::get<double>(o.v); \
    if(std::holds_alternative<int64_t>(v) && std::holds_alternative<double>(o.v)) return std::get<int64_t>(v) op std::get<double>(o.v); \
    if(std::holds_alternative<double>(v) && std::holds_alternative<int64_t>(o.v)) return std::get<double>(v) op std::get<int64_t>(o.v); \
    if(std::holds_alternative<np_string>(v) && std::holds_alternative<np_string>(o.v)) return static_cast<const std::string&>(std::get<np_string>(v)) op static_cast<const std::string&>(std::get<np_string>(o.v)); \
    if(std::holds_alternative<bool>(v) && std::holds_alternative<bool>(o.v)) return std::get<bool>(v) op std::get<bool>(o.v); \
    return false; \
}
NP_CMP_OP_IMPL(>)
NP_CMP_OP_IMPL(<)
NP_CMP_OP_IMPL(>=)
NP_CMP_OP_IMPL(<=)
NP_CMP_OP_IMPL(==)
NP_CMP_OP_IMPL(!=)

void np_var::sort() {
    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) {
        std::sort(std::get<ListPtr>(v)->vec.begin(), std::get<ListPtr>(v)->vec.end());
    }
}

void np_var::reverse() {
    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) {
        std::reverse(std::get<ListPtr>(v)->vec.begin(), std::get<ListPtr>(v)->vec.end());
    }
}

bool np_var::contains(const np_var& val) const {
    if (std::holds_alternative<ListPtr>(v) && std::get<ListPtr>(v)) {
        auto const& vec = std::get<ListPtr>(v)->vec;
        for (auto const& item : vec) {
            if (item == val) return true;
        }
    }
    if (std::holds_alternative<DictPtr>(v) && std::get<DictPtr>(v)) {
        return std::get<DictPtr>(v)->map.count(static_cast<std::string>(val)) > 0;
    }
    if (std::holds_alternative<np_string>(v)) {
        return std::get<np_string>(v).find(static_cast<std::string>(val)) != std::string::npos;
    }
    return false;
}

np_var np_var::split(const np_var& delim) const {
    if (std::holds_alternative<np_string>(v)) return std::get<np_string>(v).split(static_cast<std::string>(delim));
    return np_var(std::vector<np_var>{});
}

np_var np_var::join(const np_var& arr) const {
    if (std::holds_alternative<np_string>(v)) return std::get<np_string>(v).join(arr);
    return np_var("");
}

np_var np_var::trim() const {
    if (std::holds_alternative<np_string>(v)) return std::get<np_string>(v).trim();
    return *this;
}

// Read/Write files
std::string np_read_file(const std::string& filename) {
    std::ifstream f(filename);
    if (!f.is_open()) return "";
    std::stringstream buf;
    buf << f.rdbuf();
    return buf.str();
}

int np_write_file(const std::string& filename, const std::string& content) {
    std::ofstream f(filename);
    if (!f.is_open()) return 0;
    f << content;
    return 1;
}

// Type Conversions
int np_to_int(const std::string& s) { try { return std::stoi(s); } catch(...) { return 0; } }
int np_to_int(float v) { return static_cast<int>(v); }
float np_to_float(const std::string& s) { try { return std::stof(s); } catch(...) { return 0.0f; } }
float np_to_float(int64_t v) { return static_cast<float>(v); }
std::string np_to_string(int v) { return std::to_string(v); }
std::string np_to_string(int64_t v) { return std::to_string(v); }
std::string np_to_string(double v) { return std::to_string(v); }

int np_to_int(const np_var& v) { return static_cast<int>(static_cast<int64_t>(v)); }
float np_to_float(const np_var& v) { return static_cast<float>(static_cast<double>(v)); }
std::string np_to_string(const np_var& v) { return v.to_string(); }

std::string np_to_string(const np_int128& v) { return v.to_string(); }
std::string np_to_string(const np_int256& v) { return v.to_string(); }
int np_to_int(const np_int128& v) { return static_cast<int>(static_cast<int64_t>(v)); }
int np_to_int(const np_int256& v) { return static_cast<int>(static_cast<int64_t>(v.low)); }
float np_to_float(const np_int128& v) { return static_cast<float>(static_cast<double>(v)); }
float np_to_float(const np_int256& v) { return static_cast<float>(static_cast<double>(v.low)); }

// ==========================================
// 2. Sys and time modules
// ==========================================
np_var np_sys_argv;
void np_init_args(int argc, char* argv[]) {
    std::vector<np_var> args;
    for (int i = 0; i < argc; ++i) {
        args.push_back(np_var(std::string(argv[i])));
    }
    np_sys_argv = np_var(args);
}

double np_time_now() {
    auto now = std::chrono::system_clock::now();
    auto duration = now.time_since_epoch();
    return std::chrono::duration_cast<std::chrono::microseconds>(duration).count() / 1000000.0;
}

void np_time_sleep(double secs) {
    std::this_thread::sleep_for(std::chrono::microseconds(static_cast<int64_t>(secs * 1000000.0)));
}

std::string np_time_format(double ts, const std::string& fmt) {
    std::time_t raw_time = static_cast<std::time_t>(ts);
    std::tm* timeinfo = std::localtime(&raw_time);
    if (!timeinfo) return "";
    char buffer[256];
    std::strftime(buffer, sizeof(buffer), fmt.c_str(), timeinfo);
    return std::string(buffer);
}

// ==========================================
// 3. JSON and Regex modules
// ==========================================
std::string np_json_stringify(const np_var& val) {
    if (std::holds_alternative<int64_t>(val.v)) {
        return std::to_string(std::get<int64_t>(val.v));
    }
    if (std::holds_alternative<double>(val.v)) {
        return std::to_string(std::get<double>(val.v));
    }
    if (std::holds_alternative<bool>(val.v)) {
        return std::get<bool>(val.v) ? "true" : "false";
    }
    if (std::holds_alternative<np_string>(val.v)) {
        std::string s = std::get<np_string>(val.v);
        std::string res = "\"";
        for (char c : s) {
            if (c == '"') res += "\\\"";
            else if (c == '\\') res += "\\\\";
            else if (c == '\n') res += "\\n";
            else if (c == '\t') res += "\\t";
            else if (c == '\r') res += "\\r";
            else res += c;
        }
        res += "\"";
        return res;
    }
    if (std::holds_alternative<ListPtr>(val.v)) {
        ListPtr ptr = std::get<ListPtr>(val.v);
        if (!ptr) return "[]";
        std::string res = "[";
        for (size_t i = 0; i < ptr->vec.size(); ++i) {
            res += np_json_stringify(ptr->vec[i]);
            if (i < ptr->vec.size() - 1) res += ",";
        }
        res += "]";
        return res;
    }
    if (std::holds_alternative<DictPtr>(val.v)) {
        DictPtr ptr = std::get<DictPtr>(val.v);
        if (!ptr) return "{}";
        std::string res = "{";
        auto it = ptr->map.begin();
        while (it != ptr->map.end()) {
            res += "\"" + it->first + "\":" + np_json_stringify(it->second);
            it++;
            if (it != ptr->map.end()) res += ",";
        }
        res += "}";
        return res;
    }
    if (std::holds_alternative<np_int128>(val.v)) {
        return std::get<np_int128>(val.v).to_string();
    }
    if (std::holds_alternative<np_int256>(val.v)) {
        return std::get<np_int256>(val.v).to_string();
    }
    return "null";
}

struct np_json_parser_t {
    std::string src;
    size_t pos = 0;
    void skip_whitespace() {
        while (pos < src.size() && (src[pos] == ' ' || src[pos] == '\t' || src[pos] == '\n' || src[pos] == '\r')) pos++;
    }
    char peek() {
        skip_whitespace();
        if (pos >= src.size()) return '\0';
        return src[pos];
    }
    char get() {
        skip_whitespace();
        if (pos >= src.size()) return '\0';
        return src[pos++];
    }
    np_var parse_value() {
        char c = peek();
        if (c == '{') return parse_object();
        if (c == '[') return parse_array();
        if (c == '"') return parse_string();
        if (c == 't' || c == 'f') return parse_bool();
        if (c == 'n') { pos += 4; return np_var(); }
        if ((c >= '0' && c <= '9') || c == '-') return parse_number();
        return np_var();
    }
    np_var parse_object() {
        get(); // '{'
        std::map<std::string, np_var> obj;
        if (peek() == '}') { get(); return np_var(obj); }
        while (true) {
            np_var key_var = parse_string();
            std::string key = static_cast<std::string>(key_var);
            if (get() != ':') return np_var(obj);
            np_var val = parse_value();
            obj[key] = val;
            char c = peek();
            if (c == '}') { get(); break; }
            if (c == ',') get();
            else break;
        }
        return np_var(obj);
    }
    np_var parse_array() {
        get(); // '['
        std::vector<np_var> arr;
        if (peek() == ']') { get(); return np_var(arr); }
        while (true) {
            arr.push_back(parse_value());
            char c = peek();
            if (c == ']') { get(); break; }
            if (c == ',') get();
            else break;
        }
        return np_var(arr);
    }
    np_var parse_string() {
        get(); // '"'
        std::string s = "";
        while (pos < src.size()) {
            char c = src[pos++];
            if (c == '"') break;
            if (c == '\\' && pos < src.size()) {
                char next = src[pos++];
                if (next == '"') s += '"';
                else if (next == '\\') s += '\\';
                else if (next == 'n') s += '\n';
                else if (next == 't') s += '\t';
                else if (next == 'r') s += '\r';
                else s += next;
            } else {
                s += c;
            }
        }
        return np_var(s);
    }
    np_var parse_bool() {
        if (peek() == 't') { pos += 4; return np_var(true); }
        else { pos += 5; return np_var(false); }
    }
    np_var parse_number() {
        size_t start = pos;
        bool is_double = false;
        while (pos < src.size()) {
            char c = src[pos];
            if ((c >= '0' && c <= '9') || c == '-' || c == '+' || c == 'e' || c == 'E') pos++;
            else if (c == '.') { is_double = true; pos++; }
            else break;
        }
        std::string num_str = src.substr(start, pos - start);
        if (is_double) {
            try { return np_var(std::stod(num_str)); } catch(...) { return np_var(0.0); }
        } else {
            try { return np_var(static_cast<int64_t>(std::stoll(num_str))); } catch(...) { return np_var(static_cast<int64_t>(0)); }
        }
    }
};

np_var np_json_parse(const std::string& src) {
    np_json_parser_t parser;
    parser.src = src;
    return parser.parse_value();
}

bool np_regex_match(const std::string& pattern, const std::string& text) {
    try { std::regex re(pattern); return std::regex_match(text, re); } catch(...) { return false; }
}

std::string np_regex_find(const std::string& pattern, const std::string& text) {
    try {
        std::regex re(pattern); std::smatch m;
        if (std::regex_search(text, m, re)) return m.str(0);
    } catch(...) {}
    return "";
}

std::string np_regex_replace(const std::string& pattern, const std::string& repl, const std::string& text) {
    try { std::regex re(pattern); return std::regex_replace(text, re, repl); } catch(...) { return text; }
}

// Note: The C-compatible extern "C" Runtime API wrapper implementations have been split into runtime/npruntime_api.cpp for maintainability.
