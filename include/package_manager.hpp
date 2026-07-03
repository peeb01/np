#ifndef PACKAGE_MANAGER_HPP
#define PACKAGE_MANAGER_HPP

#include <string>

void getPackages();
bool isRemotePath(const std::string& path);
bool downloadRemotePackage(const std::string& pkg_path);

#endif // PACKAGE_MANAGER_HPP

