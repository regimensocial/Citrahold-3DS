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
        // Iterate over each entry in the directory
        for (const auto &entry : std::filesystem::directory_iterator(path))
        {
            if (std::filesystem::is_regular_file(entry.path()))
            {
                // Remove the file
                std::filesystem::remove(entry.path());
                std::cout << "Deleted: " << entry.path() << std::endl;
            }
            else if (std::filesystem::is_directory(entry.path()))
            {
                // Recursive call for subdirectories
                safeDirectoryRemove(entry.path());
            }
        }

        for (const auto &entry : std::filesystem::directory_iterator(path))
        {
            // Check if it is a regular file
            if (std::filesystem::is_regular_file(entry.path()))
            {
                // Remove the file
                std::filesystem::remove(entry.path());
                std::cout << "Deleted: " << entry.path() << std::endl;
            }
        }

        std::filesystem::remove(path);
    }
    catch (const std::exception &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}
