#ifndef DIRECTORYMENU_H
#define DIRECTORYMENU_H
#include <string>
#include <iostream>
#include "types/menuItemsType.h"
#include "types/uploadTypeEnumType.h"
#include <3ds.h>
#include <citro2d.h>
#include <filesystem>
#include "json.hpp"

class DirectoryMenu
{

public:

    DirectoryMenu();
    std::filesystem::path getCurrentDirectory();
    menuItems getCurrentDirectoryMenuItems();
    
    menuItems getGameIDDirectoryMenuItems(nlohmann::json &json);
    menuItems getSaveSelectionMenuItems(std::filesystem::path gamePath);

    void setCurrentDirectory(std::filesystem::path newDirectory);
    std::filesystem::path getGamePathFromGameID(std::string gameID, nlohmann::json &gameIDJSON);
    

private:

    std::filesystem::path currentDirectory = "/3ds/Checkpoint";

};

#endif // DIRECTORYMENU_H