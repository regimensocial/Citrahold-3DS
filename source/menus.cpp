#include "menus.h"

// Define the menu items in this source file
menuItems mainMenuItems = {
    {"Game IDs", menuFunctions::gameIDDirectoryMenuItems},
    {"Upload", menuFunctions::uploadMenuItems},
    {"Download", menuFunctions::downloadMenuItems},
    {"Settings", menuFunctions::settingsMenuItems}};

menuItems gameIDDirectoryMenuItems = {
    {"New Game ID", menuFunctions::directoryMenuItems},
    {"Current saves Game IDs", menuFunctions::existingGameIDSavesMenuItems},
    {"Current extdata Game IDs", menuFunctions::existingGameIDExtdataMenuItems},
    {"Back ", menuFunctions::changeToPreviousMenu}
};

menuItems existingGameIDsMenuItems = {
    {"loading", menuFunctions::noAction},
    {"Cancel ", menuFunctions::changeToPreviousMenu}
};

menuItems uploadMenuItems = {
    {"Upload Game saves", menuFunctions::gameIDSavesMenuItems},
    {"Upload Game extdata", menuFunctions::gameIDExtdataMenuItems},
    {"Back ", menuFunctions::changeToPreviousMenu}};

menuItems downloadMenuItems = {
    {"Download Game saves", menuFunctions::downloadSavesMenuItems},
    {"Download Game extdata", menuFunctions::downloadExtdataMenuItems},
    {"Back ", menuFunctions::changeToPreviousMenu}};

menuItems settingMenuItems = {
    {"Input a shorthand token", menuFunctions::inputToken},
    {"Check server connection", menuFunctions::checkServerConnection},
    {"Reset game ID files", menuFunctions::resetGameIDFiles},
    {"Back ", menuFunctions::changeToPreviousMenu}};

menuItems uploadDirectoryMenuItems = {
    {"loading", menuFunctions::noAction},
    {"Cancel ", menuFunctions::changeToPreviousMenu}};

menuItems gameIDMenuItems = {
    {"loading", menuFunctions::noAction},
    {"Cancel ", menuFunctions::changeToPreviousMenu}};

menuItems saveSelectionMenuItems = {
    {"loading", menuFunctions::noAction},
    {"Cancel ", menuFunctions::changeToPreviousMenu}
};

menuItems downloadGameMenuItems = {
    {"loading", menuFunctions::noAction},
    {"Cancel ", menuFunctions::changeToPreviousMenu}
};