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
    void* np_rt_var_create_list_size(int64_t size) {
        return new np_var(std::vector<np_var>(size, np_var()));
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
        if (!val_var) {
            static_cast<np_var*>(list_var)->append(np_var());
        } else {
            static_cast<np_var*>(list_var)->append(*static_cast<np_var*>(val_var));
        }
    }
    void* np_rt_var_pop(void* list_var) {
        return new np_var(static_cast<np_var*>(list_var)->pop());
    }
    void np_rt_var_clear(void* v) {
        static_cast<np_var*>(v)->clear();
    }
    void* np_rt_var_keys(void* v) {
        return new np_var(static_cast<np_var*>(v)->keys());
    }
    void* np_rt_var_values(void* v) {
        return new np_var(static_cast<np_var*>(v)->values());
    }
    void np_rt_var_sort(void* v) {
        static_cast<np_var*>(v)->sort();
    }
    void np_rt_var_reverse(void* v) {
        static_cast<np_var*>(v)->reverse();
    }
    bool np_rt_var_contains(void* v, void* val) {
        return static_cast<np_var*>(v)->contains(*static_cast<np_var*>(val));
    }
    void* np_rt_var_split(void* v, void* delim) {
        return new np_var(static_cast<np_var*>(v)->split(*static_cast<np_var*>(delim)));
    }
    void* np_rt_var_join(void* v, void* arr) {
        return new np_var(static_cast<np_var*>(v)->join(*static_cast<np_var*>(arr)));
    }
    void* np_rt_var_trim(void* v) {
        return new np_var(static_cast<np_var*>(v)->trim());
    }
    double np_rt_min(double a, double b) {
        return std::min(a, b);
    }
    double np_rt_max(double a, double b) {
        return std::max(a, b);
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
    void* np_rt_var_shape(void* v) {
        return new np_var(static_cast<np_var*>(v)->shape());
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
        if (!lhs && !rhs) return true;
        if (!lhs || !rhs) return false;
        return *static_cast<np_var*>(lhs) == *static_cast<np_var*>(rhs);
    }
    bool np_rt_var_ne(void* lhs, void* rhs) {
        if (!lhs && !rhs) return false;
        if (!lhs || !rhs) return true;
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
        if (!v) {
            std::cout << "nil" << std::endl;
            return;
        }
        std::cout << *static_cast<np_var*>(v) << std::endl;
    }
    void np_rt_assert_fail(int64_t line, void* msg_str) {
        std::cerr << "AssertionError on line " << line << ": "
                  << *static_cast<np_string*>(msg_str) << std::endl;
        exit(1);
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
    void* np_rt_read_file(void* filename_str) {
        std::string filename = *static_cast<np_string*>(filename_str);
        std::string content = np_read_file(filename);
        return new np_string(content);
    }
    int64_t np_rt_write_file(void* filename_str, void* content_str) {
        std::string filename = *static_cast<np_string*>(filename_str);
        std::string content = *static_cast<np_string*>(content_str);
        return np_write_file(filename, content);
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

    // Concurrency Channels & Goroutines API
    void* np_rt_chan_create(int64_t capacity) {
        return new NPChannel(capacity > 0 ? capacity : 0);
    }
    void np_rt_chan_send(void* ch, void* val) {
        if (!ch) return;
        static_cast<NPChannel*>(ch)->send(val ? *static_cast<np_var*>(val) : np_var());
    }
    void* np_rt_chan_recv(void* ch) {
        if (!ch) return nullptr;
        return new np_var(static_cast<NPChannel*>(ch)->recv());
    }
    void np_rt_chan_close(void* ch) {
        if (!ch) return;
        static_cast<NPChannel*>(ch)->close();
    }
    void np_rt_go_spawn(void (*fn)(void*), void* arg) {
        std::thread([fn, arg]() {
            fn(arg);
        }).detach();
    }
    void* np_rt_var_create_ptr(void* ptr) {
        return new np_var(reinterpret_cast<int64_t>(ptr));
    }
    void* np_rt_to_ptr_var(void* v) {
        if (!v) return nullptr;
        return reinterpret_cast<void*>(static_cast<int64_t>(*static_cast<np_var*>(v)));
    }

    // OS & Crypto API
    void* np_rt_os_exec(void* cmd_ptr) {
        if (!cmd_ptr) return new np_string("");
        std::string cmd = static_cast<np_string*>(cmd_ptr)->c_str();
        FILE* pipe = popen(cmd.c_str(), "r");
        if (!pipe) return new np_string("");
        char buffer[256];
        std::string result = "";
        while (fgets(buffer, sizeof(buffer), pipe) != NULL) {
            result += buffer;
        }
        pclose(pipe);
        // Trim trailing newline if present
        if (!result.empty() && result.back() == '\n') {
            result.pop_back();
        }
        return new np_string(result);
    }

    int64_t np_rt_os_system(void* cmd_ptr) {
        if (!cmd_ptr) return -1;
        std::string cmd = static_cast<np_string*>(cmd_ptr)->c_str();
        return static_cast<int64_t>(std::system(cmd.c_str()));
    }

    void* np_rt_os_getenv(void* name_ptr) {
        if (!name_ptr) return new np_string("");
        const char* val = std::getenv(static_cast<np_string*>(name_ptr)->c_str());
        return new np_string(val ? val : "");
    }

    namespace sha256_detail {
        inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }
        inline uint32_t ch(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (~x & z); }
        inline uint32_t maj(uint32_t x, uint32_t y, uint32_t z) { return (x & y) ^ (x & z) ^ (y & z); }
        inline uint32_t sig0(uint32_t x) { return rotr(x, 2) ^ rotr(x, 13) ^ rotr(x, 22); }
        inline uint32_t sig1(uint32_t x) { return rotr(x, 6) ^ rotr(x, 11) ^ rotr(x, 25); }
        inline uint32_t theta0(uint32_t x) { return rotr(x, 7) ^ rotr(x, 18) ^ (x >> 3); }
        inline uint32_t theta1(uint32_t x) { return rotr(x, 17) ^ rotr(x, 19) ^ (x >> 10); }

        static const uint32_t K[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };

        inline std::string compute(const std::string& input) {
            uint32_t H[8] = {
                0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a,
                0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
            };

            std::vector<uint8_t> msg(input.begin(), input.end());
            uint64_t bit_len = static_cast<uint64_t>(msg.size()) * 8;

            msg.push_back(0x80);
            while ((msg.size() % 64) != 56) {
                msg.push_back(0x00);
            }

            for (int i = 7; i >= 0; --i) {
                msg.push_back(static_cast<uint8_t>((bit_len >> (i * 8)) & 0xff));
            }

            for (size_t chunk = 0; chunk < msg.size(); chunk += 64) {
                uint32_t W[64];
                for (int t = 0; t < 16; ++t) {
                    W[t] = (static_cast<uint32_t>(msg[chunk + t * 4]) << 24) |
                           (static_cast<uint32_t>(msg[chunk + t * 4 + 1]) << 16) |
                           (static_cast<uint32_t>(msg[chunk + t * 4 + 2]) << 8) |
                           (static_cast<uint32_t>(msg[chunk + t * 4 + 3]));
                }
                for (int t = 16; t < 64; ++t) {
                    W[t] = theta1(W[t - 2]) + W[t - 7] + theta0(W[t - 15]) + W[t - 16];
                }

                uint32_t a = H[0], b = H[1], c = H[2], d = H[3];
                uint32_t e = H[4], f = H[5], g = H[6], h = H[7];

                for (int t = 0; t < 64; ++t) {
                    uint32_t T1 = h + sig1(e) + ch(e, f, g) + K[t] + W[t];
                    uint32_t T2 = sig0(a) + maj(a, b, c);
                    h = g;
                    g = f;
                    f = e;
                    e = d + T1;
                    d = c;
                    c = b;
                    b = a;
                    a = T1 + T2;
                }

                H[0] += a; H[1] += b; H[2] += c; H[3] += d;
                H[4] += e; H[5] += f; H[6] += g; H[7] += h;
            }

            char hex_str[65];
            snprintf(hex_str, sizeof(hex_str),
                     "%08x%08x%08x%08x%08x%08x%08x%08x",
                     H[0], H[1], H[2], H[3], H[4], H[5], H[6], H[7]);
            return std::string(hex_str);
        }
    }

    void* np_rt_crypto_sha256(void* data_ptr) {
        if (!data_ptr) return new np_string("");
        std::string input = static_cast<np_string*>(data_ptr)->c_str();
        return new np_string(sha256_detail::compute(input));
    }

    int64_t np_rt_threads_num_cpu() {
        unsigned int n = std::thread::hardware_concurrency();
        return n > 0 ? static_cast<int64_t>(n) : 1;
    }

    void* np_rt_threads_run(void* (*thunk)(void*), void* argList, bool isolated) {
        np_var* targetArgs = nullptr;
        if (isolated) {
            // Shared-Nothing: Deep-clone all arguments to eliminate race conditions
            if (argList) {
                targetArgs = new np_var(static_cast<np_var*>(argList)->deep_clone());
            } else {
                targetArgs = new np_var(std::vector<np_var>{});
            }
        } else {
            // High-Performance / C-Speed: Share pointer without copying overhead
            if (argList) {
                targetArgs = static_cast<np_var*>(argList);
            } else {
                targetArgs = new np_var(std::vector<np_var>{});
            }
        }

        auto prom = std::make_shared<std::promise<np_var>>();
        auto fut = prom->get_future().share();

        std::thread([thunk, targetArgs, prom, isolated]() {
            try {
                void* ret = thunk(targetArgs);
                if (ret) {
                    prom->set_value(*static_cast<np_var*>(ret));
                } else {
                    prom->set_value(np_var());
                }
            } catch (...) {
                prom->set_value(np_var());
            }
            if (isolated) {
                delete targetArgs;
            }
        }).detach();

        return new NPTask(fut);
    }

    void* np_rt_task_wait(void* task_ptr) {
        if (!task_ptr) return new np_var();
        auto* task = static_cast<NPTask*>(task_ptr);
        return new np_var(task->wait());
    }
}
