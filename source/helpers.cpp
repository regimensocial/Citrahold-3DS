#include <iostream>
#include <string>
#include <cctype>
#include <filesystem>
#include "helpers.h"

std::string trim(const std::string &str)
{
    size_t first = str.find_first_not_of(' ');
    size_t last = str.find_last_not_of(' ');

    if (first == std::string::npos)
    {
        return "";
    }

    return str.substr(first, last - first + 1);
}

void safeDirectoryRemove(const std::filesystem::path &path)
{
    if (!std::filesystem::exists(path))
    {
        return;
    }

    try
    {
        for (const auto &entry : std::filesystem::directory_iterator(path))
        {
            if (std::filesystem::is_regular_file(entry.path()))
            {
                std::filesystem::remove(entry.path());
            }
            else if (std::filesystem::is_directory(entry.path()))
            {
                safeDirectoryRemove(entry.path());
            }
        }

        for (const auto &entry : std::filesystem::directory_iterator(path))
        {
            if (std::filesystem::is_regular_file(entry.path()))
            {
                std::filesystem::remove(entry.path());
            }
        }

        std::filesystem::remove(path);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
