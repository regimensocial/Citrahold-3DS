#ifndef HELPERS_H
#define HELPERS_H

#include <string>
#include <iostream>
#include <filesystem>

std::string trim(const std::string& str);
void safeDirectoryRemove(const std::filesystem::path &path);

#endif // HELPERS_H