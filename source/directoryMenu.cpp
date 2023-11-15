#include "directoryMenu.h"
#include <iostream>
#include <filesystem>
#include <string>
#include <vector>
#include "types/menuItemsType.h"
#include "json.hpp"

DirectoryMenu::DirectoryMenu()
{
    if (!std::filesystem::exists(currentDirectory) || !std::filesystem::is_directory(currentDirectory))
    {
        currentDirectory = "/3ds/";
    }
}

std::filesystem::path DirectoryMenu::getCurrentDirectory()
{
    return currentDirectory;
}

menuItems DirectoryMenu::getCurrentDirectoryMenuItems()
{

    menuItems directoryMenuItems = {
        {std::filesystem::path(".. "), menuFunctions::traverseDirectory},
    };

    for (const auto &dirEntry : std::filesystem::directory_iterator(currentDirectory))
    {
        if (std::filesystem::is_directory(dirEntry.path()))
        {
            std::filesystem::path relativePath = std::filesystem::relative(dirEntry, currentDirectory);
            directoryMenuItems.push_back({relativePath, menuFunctions::traverseDirectory});
        }
    }

    return directoryMenuItems;
}

std::filesystem::path DirectoryMenu::getGamePathFromGameID(std::string gameID, nlohmann::json &gameIDJSON)
{
    for (auto &gameIDJSONEntry : gameIDJSON["gameID"])
    {
        if (gameIDJSONEntry[0] == gameID)
        {
            return gameIDJSONEntry[1];
        }
    }
    return "";
}

menuItems DirectoryMenu::getGameIDDirectoryMenuItems(nlohmann::json &gameIDJSON, menuFunctions nextAction)
{

    menuItems directoryMenuItems = {};

    for (auto &gameID : gameIDJSON["gameID"])
    {
        directoryMenuItems.push_back({gameID[0], nextAction});
    }

    return directoryMenuItems;
}

menuItems DirectoryMenu::getSaveSelectionMenuItems(std::filesystem::path gamePath)
{


    menuItems saveSelectionMenuItems;

    for (const auto &dirEntry : std::filesystem::directory_iterator(gamePath))
    {
        if (std::filesystem::is_directory(dirEntry.path()))
        {
            std::filesystem::path relativePath = std::filesystem::relative(dirEntry, gamePath);
            saveSelectionMenuItems.push_back({relativePath, menuFunctions::uploadGame});
        }
    }

    return saveSelectionMenuItems;
}

void DirectoryMenu::setCurrentDirectory(std::filesystem::path newDirectory)
{
    currentDirectory = newDirectory;
}
