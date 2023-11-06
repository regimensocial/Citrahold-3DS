#ifndef MENU_ITEMS_TYPE_H
#define MENU_ITEMS_TYPE_H

#include <vector>
#include <tuple>
#include <string>

enum class menuFunctions
{
    noAction,
    upload,
    download,
    settings,
    inputToken,
    changeServerAddress,
    checkServerConnection,
    changeToMainMenu,
    changeToPreviousMenu,
    openKeyboard,
    directoryMenuItems,
    gameIDSavesMenuItems,
    gameIDExtdataMenuItems,
    saveSelectionMenuItems,
    uploadSave,
    downloadSavesMenuItems,
    downloadExtdataMenuItems,
    downloadSaveMenuItems,
    downloadSave,
    resetGameIDFiles
};

using menuItems = std::vector<std::tuple<std::string, menuFunctions>>;

// Declare other menu-related declarations, functions, and structures if needed

#endif // MENU_ITEMS_TYPE_H
