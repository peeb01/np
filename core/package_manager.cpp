#include "../include/package_manager.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <llvm/Support/SHA256.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>

static std::string computeDirectoryHash(const std::string& dir_path) {
    llvm::SHA256 hasher;
    std::vector<std::filesystem::path> paths;
    
    // Gather all files recursively
    for (const auto& entry : std::filesystem::recursive_directory_iterator(dir_path)) {
        if (entry.is_regular_file()) {
            std::string path_str = entry.path().string();
            // Exclude the .git metadata directory from the hash calculation
            if (path_str.find(".git") != std::string::npos) {
                continue;
            }
            paths.push_back(entry.path());
        }
    }
    
    // Sort paths alphabetically to guarantee a deterministic hash
    std::sort(paths.begin(), paths.end());
    
    for (const auto& p : paths) {
        // Hash the relative path of the file
        std::string rel_path = std::filesystem::relative(p, dir_path).string();
        hasher.update(llvm::StringRef(rel_path));
        
        // Hash the contents of the file
        std::ifstream file(p, std::ios::binary);
        if (file.is_open()) {
            std::stringstream buffer;
            buffer << file.rdbuf();
            std::string content = buffer.str();
            hasher.update(llvm::StringRef(content));
        }
    }
    
    std::array<uint8_t, 32> hash_result = hasher.final();
    
    // Convert to hex string (64 characters)
    std::string hex_str;
    hex_str.reserve(64);
    for (uint8_t b : hash_result) {
        hex_str.push_back("0123456789abcdef"[b >> 4]);
        hex_str.push_back("0123456789abcdef"[b & 0xf]);
    }
    return hex_str;
}

static std::string trim(const std::string& str) {
    size_t first = str.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return "";
    size_t last = str.find_last_not_of(" \t\r\n");
    return str.substr(first, (last - first + 1));
}

void getPackages() {
    std::ifstream req_file("np.req");
    if (!req_file.is_open()) {
        std::cerr << "Error: np.req file not found! Please create np.req in the project root.\n";
        exit(1);
    }
    
    // 1. Read existing hashes from np.req.log (go.sum style)
    std::map<std::pair<std::string, std::string>, std::string> logged_hashes;
    std::ifstream log_file("np.req.log");
    if (log_file.is_open()) {
        std::string line;
        while (std::getline(log_file, line)) {
            line = trim(line);
            if (line.empty() || line[0] == '#') continue;
            
            std::stringstream ss(line);
            std::string pkg, ver, hash;
            if (ss >> pkg >> ver >> hash) {
                logged_hashes[{pkg, ver}] = hash;
            }
        }
        log_file.close();
    }
    
    // 2. Parse np.req and download/verify packages
    std::vector<std::pair<std::string, std::string>> packages;
    std::string line;
    while (std::getline(req_file, line)) {
        line = trim(line);
        if (line.empty() || line[0] == '#') continue;
        
        std::stringstream ss(line);
        std::string pkg, ver;
        if (ss >> pkg >> ver) {
            packages.push_back({pkg, ver});
        }
    }
    req_file.close();
    
    if (packages.empty()) {
        std::cout << "No dependencies listed in np.req.\n";
        return;
    }
    
    std::cout << "Downloading and verifying dependencies...\n";
    std::filesystem::create_directories(".np_packages");
    
    for (const auto& [pkg, ver] : packages) {
        std::filesystem::path pkg_path = ".np_packages/" + pkg;
        
        // Remove existing files for a clean download
        if (std::filesystem::exists(pkg_path)) {
            std::filesystem::remove_all(pkg_path);
        }
        
        std::filesystem::create_directories(pkg_path.parent_path());
        
        std::cout << "  Fetching " << pkg << " (" << ver << ")...\n";
        
        // Clone from Git
        std::string git_cmd = "git clone --depth 1 --branch " + ver + " https://" + pkg + " " + pkg_path.string();
        int clone_res = std::system(git_cmd.c_str());
        if (clone_res != 0) {
            std::cerr << "Error: Failed to clone package " << pkg << " at version " << ver << "\n";
            exit(1);
        }
        
        // Compute checksum hash
        std::string computed_hash = "h1:" + computeDirectoryHash(pkg_path.string());
        
        // Verify against np.req.log if it exists
        auto it = logged_hashes.find({pkg, ver});
        if (it != logged_hashes.end()) {
            if (it->second != computed_hash) {
                std::cerr << "\nSECURITY ERROR: Hash mismatch for package " << pkg << " (" << ver << ")!\n"
                          << "Expected: " << it->second << "\n"
                          << "Got:      " << computed_hash << "\n"
                          << "This might indicate that the repository was modified or tampered with.\n";
                std::filesystem::remove_all(pkg_path);
                exit(1);
            } else {
                std::cout << "  Verified " << pkg << " (" << ver << ") - checksum matches\n";
            }
        } else {
            std::cout << "  Recorded new checksum for " << pkg << " (" << ver << ")\n";
            logged_hashes[{pkg, ver}] = computed_hash;
        }
    }
    
    // 3. Write out updated np.req.log
    std::ofstream out_log_file("np.req.log");
    if (!out_log_file.is_open()) {
        std::cerr << "Error: Could not write to np.req.log\n";
        exit(1);
    }
    
    out_log_file << "# Auto-generated by np compiler package manager. DO NOT EDIT.\n";
    for (const auto& [key, hash] : logged_hashes) {
        out_log_file << key.first << " " << key.second << " " << hash << "\n";
    }
    out_log_file.close();
    
    std::cout << "\nSuccess: Dependencies installed and verified successfully.\n";
}
