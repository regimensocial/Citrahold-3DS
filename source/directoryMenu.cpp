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
    // loop through all DIRECTORIES only in currentDirectory

    menuItems directoryMenuItems = {
        {std::filesystem::path(".. "), menuFunctions::noAction},
    };

    for (const auto &dirEntry : std::filesystem::directory_iterator(currentDirectory))
    {
        // if this is a directory.
        if (std::filesystem::is_directory(dirEntry.path()))
        {
            std::filesystem::path relativePath = std::filesystem::relative(dirEntry, currentDirectory);
            directoryMenuItems.push_back({relativePath, menuFunctions::noAction});
        }
    }

    return directoryMenuItems;
}

std::filesystem::path DirectoryMenu::getGamePathFromGameID(std::string gameID, nlohmann::json &gameIDJSON)
{
    // we need to find the gameID in the json file
    for (auto &gameIDJSONEntry : gameIDJSON["gameID"])
    {
        if (gameIDJSONEntry[0] == gameID)
        {
            return gameIDJSONEntry[1];
        }
    }
    return "";
}

menuItems DirectoryMenu::getGameIDDirectoryMenuItems(nlohmann::json &gameIDJSON)
{

    menuItems directoryMenuItems = {};

    // for of gameIDJSON["gameID"]
    for (auto &gameID : gameIDJSON["gameID"])
    {
        directoryMenuItems.push_back({gameID[0], menuFunctions::saveSelectionMenuItems});
    }

    return directoryMenuItems;
}

menuItems DirectoryMenu::getSaveSelectionMenuItems(std::filesystem::path gamePath)
{

    // loop through all DIRECTORIES only in currentDirectory

    menuItems saveSelectionMenuItems;

    for (const auto &dirEntry : std::filesystem::directory_iterator(gamePath))
    {
        // if this is a directory.
        if (std::filesystem::is_directory(dirEntry.path()))
        {
            std::filesystem::path relativePath = std::filesystem::relative(dirEntry, gamePath);
            saveSelectionMenuItems.push_back({relativePath, menuFunctions::uploadSave});
        }
    }

    return saveSelectionMenuItems;
}

void DirectoryMenu::setCurrentDirectory(std::filesystem::path newDirectory)
{
    currentDirectory = newDirectory;
}
