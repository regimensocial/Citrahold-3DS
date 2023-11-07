#include "menus.h"

// Define the menu items in this source file
menuItems mainMenuItems = {
    {"Upload", menuFunctions::upload},
    {"Download", menuFunctions::download},
    {"Settings", menuFunctions::settings}};

menuItems uploadMenuItems = {
    {"New Game Directory", menuFunctions::directoryMenuItems},
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

menuItems loadingMenuItems = {
    {"Loading...", menuFunctions::noAction},
};

menuItems saveSelectionMenuItems = {
    {"loading", menuFunctions::noAction},
    {"Cancel ", menuFunctions::changeToPreviousMenu}
};

menuItems downloadGameMenuItems = {
    {"loading", menuFunctions::noAction},
    {"Cancel ", menuFunctions::changeToPreviousMenu}
};