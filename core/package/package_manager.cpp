#include "package_manager.hpp"
#include "http_fetch.hpp"
#include "miniz.h"
#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <unordered_set>
#include <filesystem>
#include <cstdlib>
#include <algorithm>
#include <cstring>
#include <llvm/Support/SHA256.h>
#include <llvm/ADT/ArrayRef.h>
#include <llvm/ADT/StringRef.h>
#include <llvm/Config/llvm-config.h>

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
    
#if LLVM_VERSION_MAJOR >= 15
    std::array<uint8_t, 32> hash_result = hasher.final();
#else
    llvm::StringRef hash_ref = hasher.final();
    std::array<uint8_t, 32> hash_result;
    std::memcpy(hash_result.data(), hash_ref.data(), 32);
#endif
    
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

bool isRemotePath(const std::string& path) {
    return path.rfind("github.com/", 0) == 0 ||
           path.rfind("gitlab.com/", 0) == 0 ||
           path.rfind("bitbucket.org/", 0) == 0;
}

static std::string getRepoRoot(const std::string& pkg_path) {
    std::stringstream ss(pkg_path);
    std::string item;
    std::vector<std::string> parts;
    while (std::getline(ss, item, '/')) {
        parts.push_back(item);
    }
    if (parts.size() >= 3) {
        return parts[0] + "/" + parts[1] + "/" + parts[2];
    }
    return pkg_path;
}

static bool extractZip(const std::string& zip_file, const std::string& out_dir) {
    mz_zip_archive zipArchive;
    std::memset(&zipArchive, 0, sizeof(zipArchive));

    if (!mz_zip_reader_init_file(&zipArchive, zip_file.c_str(), 0)) {
        std::cerr << "Failed to open ZIP archive: " << zip_file << "\n";
        return false;
    }

    int fileCount = (int)mz_zip_reader_get_num_files(&zipArchive);
    for (int i = 0; i < fileCount; ++i) {
        mz_zip_archive_file_stat fileStat;
        if (!mz_zip_reader_file_stat(&zipArchive, i, &fileStat)) {
            std::cerr << "Failed to get ZIP file stat at index: " << i << "\n";
            continue;
        }

        std::string raw_name = fileStat.m_filename;
        size_t slash_pos = raw_name.find('/');
        if (slash_pos == std::string::npos) {
            continue;
        }
        
        std::string rel_path = raw_name.substr(slash_pos + 1);
        if (rel_path.empty()) {
            continue;
        }

        std::string outputFilePath = out_dir + "/" + rel_path;
        
        if (rel_path.back() == '/' || rel_path.back() == '\\') {
            std::filesystem::create_directories(outputFilePath);
            continue;
        }

        std::filesystem::create_directories(std::filesystem::path(outputFilePath).parent_path());
        
        if (!mz_zip_reader_extract_to_file(&zipArchive, i, outputFilePath.c_str(), 0)) {
            std::cerr << "Failed to extract ZIP entry: " << raw_name << " to " << outputFilePath << "\n";
        }
    }

    mz_zip_reader_end(&zipArchive);
    return true;
}

bool downloadRemotePackage(const std::string& pkg_path) {
    std::string repo_root = getRepoRoot(pkg_path);
    static std::unordered_set<std::string> downloaded;
    if (downloaded.count(repo_root) > 0) {
        return false;
    }
    downloaded.insert(repo_root);

    std::filesystem::path dest_dir = ".np_packages/" + repo_root;
    std::filesystem::create_directories(".np_packages/.download_cache");
    std::string temp_zip = ".np_packages/.download_cache/temp_pkg.zip";

    size_t first_slash = repo_root.find('/');
    if (first_slash == std::string::npos) return false;
    std::string path_after_domain = repo_root.substr(first_slash + 1);
    size_t second_slash = path_after_domain.find('/');
    if (second_slash == std::string::npos) return false;
    
    std::string user = path_after_domain.substr(0, second_slash);
    std::string repo = path_after_domain.substr(second_slash + 1);

    std::cout << "Downloading package " << repo_root << " ...\n";
    
    std::string domain = repo_root.substr(0, first_slash);
    std::string url;
    bool success = false;
    
    if (domain == "github.com") {
        url = "https://github.com/" + user + "/" + repo + "/archive/refs/heads/main.zip";
        success = httpDownload(url, temp_zip);
        if (!success) {
            url = "https://github.com/" + user + "/" + repo + "/archive/refs/heads/master.zip";
            success = httpDownload(url, temp_zip);
        }
    } else if (domain == "gitlab.com") {
        url = "https://gitlab.com/" + user + "/" + repo + "/-/archive/main/" + repo + "-main.zip";
        success = httpDownload(url, temp_zip);
        if (!success) {
            url = "https://gitlab.com/" + user + "/" + repo + "/-/archive/master/" + repo + "-master.zip";
            success = httpDownload(url, temp_zip);
        }
    } else {
        std::cerr << "Error: Unsupported remote repository hosting: " << domain << "\n";
        return false;
    }

    if (!success) {
        std::cerr << "Error: Failed to download remote package from " << repo_root << "\n";
        return false;
    }

    std::filesystem::create_directories(dest_dir);
    std::filesystem::remove_all(dest_dir);
    std::filesystem::create_directories(dest_dir);

    std::cout << "Extracting " << temp_zip << " to " << dest_dir.string() << " ...\n";
    if (!extractZip(temp_zip, dest_dir.string())) {
        std::cerr << "Error: Failed to extract ZIP archive for " << repo_root << "\n";
        std::filesystem::remove(temp_zip);
        return false;
    }

    std::filesystem::remove(temp_zip);
    std::cout << "Successfully installed " << repo_root << "\n";

    // Compute checksum hash and update np.req.log if possible
    std::string computed_hash = "h1:" + computeDirectoryHash(dest_dir.string());
    
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
    
    logged_hashes[{repo_root, "main"}] = computed_hash;

    std::ofstream out_log_file("np.req.log");
    if (out_log_file.is_open()) {
        out_log_file << "# Auto-generated by np compiler package manager. DO NOT EDIT.\n";
        for (const auto& [key, hash] : logged_hashes) {
            out_log_file << key.first << " " << key.second << " " << hash << "\n";
        }
        out_log_file.close();
    }

    return true;
}
