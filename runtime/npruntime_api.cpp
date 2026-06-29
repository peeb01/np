#include "npruntime.hpp"
#include <iostream>
#include <vector>
#include <cstring>

#ifndef _WIN32
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#else
#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#ifdef _WIN32
static void ensure_winsock() {
    static bool initialized = false;
    if (!initialized) {
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
        initialized = true;
    }
}
#endif

// ==========================================
// 4. C-Compatible extern "C" Runtime API Implementation
// ==========================================
extern "C" {
    // String API
    void* np_rt_string_create(const char* s) {
        return new np_string(s);
    }
    void np_rt_string_destroy(void* s) {
        delete static_cast<np_string*>(s);
    }
    void* np_rt_string_concat(void* s1, void* s2) {
        return new np_string(*static_cast<np_string*>(s1) + *static_cast<np_string*>(s2));
    }
    void* np_rt_string_concat_char(void* s1, const char* s2) {
        return new np_string(*static_cast<np_string*>(s1) + s2);
    }
    void* np_rt_string_concat_char_lhs(const char* s1, void* s2) {
        return new np_string(s1 + *static_cast<np_string*>(s2));
    }
    const char* np_rt_string_c_str(void* s) {
        return static_cast<np_string*>(s)->c_str();
    }
    int64_t np_rt_string_len(void* s) {
        return static_cast<np_string*>(s)->length();
    }
    void* np_rt_string_slice(void* s, int64_t start, int64_t end) {
        return new np_string(static_cast<np_string*>(s)->slice(start, end));
    }
    bool np_rt_string_eq(void* s1, void* s2) {
        return static_cast<const std::string&>(*static_cast<np_string*>(s1)) == static_cast<const std::string&>(*static_cast<np_string*>(s2));
    }
    bool np_rt_string_lt(void* s1, void* s2) {
        return static_cast<const std::string&>(*static_cast<np_string*>(s1)) < static_cast<const std::string&>(*static_cast<np_string*>(s2));
    }
    bool np_rt_string_contains(void* s, void* sub) {
        return static_cast<np_string*>(s)->find(*static_cast<np_string*>(sub)) != std::string::npos;
    }

    // np_var API
    void* np_rt_var_create_int(int64_t v) {
        return new np_var(v);
    }
    void* np_rt_var_create_float(double v) {
        return new np_var(v);
    }
    void* np_rt_var_create_string(void* s) {
        return new np_var(*static_cast<np_string*>(s));
    }
    void* np_rt_var_create_bool(bool v) {
        return new np_var(v);
    }
    void* np_rt_var_create_list() {
        return new np_var(std::vector<np_var>{});
    }
    void* np_rt_var_create_dict() {
        return new np_var(std::map<std::string, np_var>{});
    }
    void* np_rt_var_create_int128(const char* s) {
        return new np_var(np_int128(s));
    }
    void* np_rt_var_create_int256(const char* s) {
        return new np_var(np_int256(s));
    }
    void np_rt_var_destroy(void* v) {
        delete static_cast<np_var*>(v);
    }
    
    // List/Dict actions
    void np_rt_var_append(void* list_var, void* val_var) {
        static_cast<np_var*>(list_var)->append(*static_cast<np_var*>(val_var));
    }
    void* np_rt_var_pop(void* list_var) {
        return new np_var(static_cast<np_var*>(list_var)->pop());
    }
    void* np_rt_var_get_index(void* list_var, int64_t index) {
        return new np_var((*static_cast<np_var*>(list_var))[static_cast<int>(index)]);
    }
    void np_rt_var_set_index(void* list_var, int64_t index, void* val_var) {
        (*static_cast<np_var*>(list_var))[static_cast<int>(index)] = *static_cast<np_var*>(val_var);
    }
    void* np_rt_var_get_key(void* dict_var, const char* key) {
        return new np_var((*static_cast<np_var*>(dict_var))[key]);
    }
    void np_rt_var_set_key(void* dict_var, const char* key, void* val_var) {
        (*static_cast<np_var*>(dict_var))[key] = *static_cast<np_var*>(val_var);
    }
    int64_t np_rt_var_len(void* v) {
        return static_cast<np_var*>(v)->length();
    }
    void* np_rt_var_slice(void* v, int64_t start, int64_t end) {
        return new np_var(static_cast<np_var*>(v)->slice(static_cast<int>(start), static_cast<int>(end)));
    }

    // Operators
    void* np_rt_var_add(void* lhs, void* rhs) {
        return new np_var(*static_cast<np_var*>(lhs) + *static_cast<np_var*>(rhs));
    }
    void* np_rt_var_sub(void* lhs, void* rhs) {
        return new np_var(*static_cast<np_var*>(lhs) - *static_cast<np_var*>(rhs));
    }
    void* np_rt_var_mul(void* lhs, void* rhs) {
        return new np_var(*static_cast<np_var*>(lhs) * *static_cast<np_var*>(rhs));
    }
    void* np_rt_var_div(void* lhs, void* rhs) {
        return new np_var(*static_cast<np_var*>(lhs) / *static_cast<np_var*>(rhs));
    }
    void* np_rt_var_mod(void* lhs, void* rhs) {
        return new np_var(*static_cast<np_var*>(lhs) % *static_cast<np_var*>(rhs));
    }
    void* np_rt_var_pow(void* lhs, void* rhs) {
        return new np_var(*static_cast<np_var*>(lhs) ^ *static_cast<np_var*>(rhs));
    }
    bool np_rt_var_gt(void* lhs, void* rhs) {
        return *static_cast<np_var*>(lhs) > *static_cast<np_var*>(rhs);
    }
    bool np_rt_var_lt(void* lhs, void* rhs) {
        return *static_cast<np_var*>(lhs) < *static_cast<np_var*>(rhs);
    }
    bool np_rt_var_ge(void* lhs, void* rhs) {
        return *static_cast<np_var*>(lhs) >= *static_cast<np_var*>(rhs);
    }
    bool np_rt_var_le(void* lhs, void* rhs) {
        return *static_cast<np_var*>(lhs) <= *static_cast<np_var*>(rhs);
    }
    bool np_rt_var_eq(void* lhs, void* rhs) {
        return *static_cast<np_var*>(lhs) == *static_cast<np_var*>(rhs);
    }
    bool np_rt_var_ne(void* lhs, void* rhs) {
        return *static_cast<np_var*>(lhs) != *static_cast<np_var*>(rhs);
    }

    // Prints
    void np_rt_print_int(int64_t v) {
        std::cout << v << std::endl;
    }
    void np_rt_print_float(double v) {
        std::cout << v << std::endl;
    }
    void np_rt_print_bool(bool v) {
        std::cout << (v ? "true" : "false") << std::endl;
    }
    void np_rt_print_string(void* s) {
        std::cout << *static_cast<np_string*>(s) << std::endl;
    }
    void np_rt_print_var(void* v) {
        std::cout << *static_cast<np_var*>(v) << std::endl;
    }

    // Conversions
    int64_t np_rt_to_int_string(void* s) {
        return np_to_int(*static_cast<np_string*>(s));
    }
    int64_t np_rt_to_int_var(void* v) {
        return np_to_int(*static_cast<np_var*>(v));
    }
    double np_rt_to_float_string(void* s) {
        return np_to_float(*static_cast<np_string*>(s));
    }
    double np_rt_to_float_var(void* v) {
        return np_to_float(*static_cast<np_var*>(v));
    }
    void* np_rt_to_string_int(int64_t v) {
        return new np_string(np_to_string(v));
    }
    void* np_rt_to_string_float(double v) {
        return new np_string(np_to_string(v));
    }
    void* np_rt_to_string_var(void* v) {
        return new np_string(np_to_string(*static_cast<np_var*>(v)));
    }

    // Inputs
    int64_t np_rt_input_int() {
        int64_t val;
        std::cin >> val;
        std::cin.ignore(10000, '\n');
        return val;
    }
    double np_rt_input_float() {
        double val;
        std::cin >> val;
        std::cin.ignore(10000, '\n');
        return val;
    }
    void* np_rt_input_string() {
        std::string s;
        std::getline(std::cin, s);
        return new np_string(s);
    }

    // Module functions
    void np_rt_sys_init_args(int argc, char* argv[]) {
        np_init_args(argc, argv);
    }
    void* np_rt_sys_get_argv() {
        return new np_var(np_sys_argv);
    }
    double np_rt_time_now() {
        return np_time_now();
    }
    void np_rt_time_sleep(double secs) {
        np_time_sleep(secs);
    }
    void* np_rt_time_format(double ts, void* fmt) {
        return new np_string(np_time_format(ts, *static_cast<np_string*>(fmt)));
    }
    void* np_rt_json_stringify(void* v) {
        return new np_string(np_json_stringify(*static_cast<np_var*>(v)));
    }
    void* np_rt_json_parse(void* s) {
        return new np_var(np_json_parse(*static_cast<np_string*>(s)));
    }
    bool np_rt_regex_match(void* pattern, void* text) {
        return np_regex_match(*static_cast<np_string*>(pattern), *static_cast<np_string*>(text));
    }
    void* np_rt_regex_find(void* pattern, void* text) {
        return new np_string(np_regex_find(*static_cast<np_string*>(pattern), *static_cast<np_string*>(text)));
    }
    void* np_rt_regex_replace(void* pattern, void* repl, void* text) {
        return new np_string(np_regex_replace(*static_cast<np_string*>(pattern), *static_cast<np_string*>(repl), *static_cast<np_string*>(text)));
    }
    void* np_rt_type_var(void* v) {
        return new np_string(np_type(*static_cast<np_var*>(v)));
    }

    // Networking Socket API
    int64_t np_rt_net_listen(int64_t port) {
        #ifdef _WIN32
        ensure_winsock();
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) return -1;
        #else
        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) return -1;
        #endif

        int opt = 1;
        #ifdef _WIN32
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
        #else
        setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        #endif

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        #ifdef _WIN32
        if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(s);
            return -1;
        }
        if (listen(s, SOMAXCONN) == SOCKET_ERROR) {
            closesocket(s);
            return -1;
        }
        #else
        if (bind(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(s);
            return -1;
        }
        if (listen(s, 128) < 0) {
            close(s);
            return -1;
        }
        #endif

        return (int64_t)s;
    }

    int64_t np_rt_net_accept(int64_t server_fd) {
        #ifdef _WIN32
        SOCKET client = accept((SOCKET)server_fd, nullptr, nullptr);
        if (client == INVALID_SOCKET) return -1;
        #else
        int client = accept((int)server_fd, nullptr, nullptr);
        if (client < 0) return -1;
        #endif
        return (int64_t)client;
    }

    int64_t np_rt_net_connect(void* host_ptr, int64_t port) {
        if (!host_ptr) return -1;
        const std::string& host = *static_cast<np_string*>(host_ptr);

        #ifdef _WIN32
        ensure_winsock();
        SOCKET s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
        if (s == INVALID_SOCKET) return -1;
        #else
        int s = socket(AF_INET, SOCK_STREAM, 0);
        if (s < 0) return -1;
        #endif

        struct hostent* he = gethostbyname(host.c_str());
        if (!he) {
            #ifdef _WIN32
            closesocket(s);
            #else
            close(s);
            #endif
            return -1;
        }

        sockaddr_in addr;
        addr.sin_family = AF_INET;
        addr.sin_port = htons(port);
        std::memcpy(&addr.sin_addr, he->h_addr_list[0], he->h_length);

        #ifdef _WIN32
        if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) == SOCKET_ERROR) {
            closesocket(s);
            return -1;
        }
        #else
        if (connect(s, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
            close(s);
            return -1;
        }
        #endif

        return (int64_t)s;
    }

    int64_t np_rt_net_send(int64_t socket_fd, void* data_ptr) {
        if (!data_ptr) return -1;
        const std::string& data = *static_cast<np_string*>(data_ptr);

        #ifdef _WIN32
        int res = send((SOCKET)socket_fd, data.data(), (int)data.size(), 0);
        if (res == SOCKET_ERROR) return -1;
        #else
        int res = send((int)socket_fd, data.data(), data.size(), 0);
        if (res < 0) return -1;
        #endif
        return (int64_t)res;
    }

    void* np_rt_net_recv(int64_t socket_fd, int64_t max_bytes) {
        if (max_bytes <= 0) return new np_string("");
        std::vector<char> buffer(max_bytes);

        #ifdef _WIN32
        int bytes_read = recv((SOCKET)socket_fd, buffer.data(), (int)max_bytes, 0);
        if (bytes_read == SOCKET_ERROR || bytes_read <= 0) {
            return new np_string("");
        }
        #else
        int bytes_read = recv((int)socket_fd, buffer.data(), max_bytes, 0);
        if (bytes_read <= 0) {
            return new np_string("");
        }
        #endif

        return new np_string(buffer.data(), bytes_read);
    }

    void np_rt_net_close(int64_t socket_fd) {
        #ifdef _WIN32
        closesocket((SOCKET)socket_fd);
        #else
        close((int)socket_fd);
        #endif
    }
}
