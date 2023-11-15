#include <iostream>
#include <string>
#include <cctype>

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
