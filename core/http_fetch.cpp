#include "../include/http_fetch.hpp"
#include <cstdlib>
#include <iostream>
#include <sstream>

static bool hasCommand(const std::string& cmd) {
#ifdef _WIN32
    std::string check = "where " + cmd + " >nul 2>nul";
#else
    std::string check = "which " + cmd + " >/dev/null 2>&1";
#endif
    return std::system(check.c_str()) == 0;
}

bool httpDownload(const std::string& url, const std::string& dest_path) {
    std::string cmd;
    if (hasCommand("curl")) {
        // -L follows redirects, -s is silent, -S shows error if fails, -o specifies output
        cmd = "curl -L -s -S -o \"" + dest_path + "\" \"" + url + "\"";
    } else if (hasCommand("wget")) {
        cmd = "wget -q -O \"" + dest_path + "\" \"" + url + "\"";
    } else {
        std::cerr << "Error: Neither curl nor wget was found on the system path. Cannot download remote package.\n";
        return false;
    }

    int res = std::system(cmd.c_str());
    return res == 0;
}
