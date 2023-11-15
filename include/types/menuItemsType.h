#ifndef MENU_ITEMS_TYPE_H
#define MENU_ITEMS_TYPE_H

#include <vector>
#include <tuple>
#include <string>

enum class menuFunctions
{
    noAction,

    mainMenuMenuItems,
    
    gameIDDirectoryMenuItems,
    directoryMenuItems,
    existingGameIDSavesMenuItems,
    existingGameIDExtdataMenuItems,
    uploadMenuItems,
    downloadMenuItems,
    settingsMenuItems,

    saveSelectionMenuItems, // for upload
    gameIDSavesMenuItems,
    gameIDExtdataMenuItems,
    
    downloadSavesMenuItems,
    downloadExtdataMenuItems,

    changeToPreviousMenu,

    // Settings menu items
    inputToken,
    checkServerConnection,
    resetGameIDFiles,
    deleteSavesAfterUpload,

    // Upload and download a game
    uploadGame,
    downloadGame,

    // Not used currently
    openKeyboard,
    changeServerAddress,


    traverseDirectory,
    
    specialB_goPreviousDirectory,
    specialX_exitDirectorySelection, 
    specialY_enterGameID,

    renameGameID,
    specialX_redirectGameID,
    specialY_deleteGameID,
    specialB_exitExistingGameIDSelection,
};

using menuItems = std::vector<std::tuple<std::string, menuFunctions>>;

#endif // MENU_ITEMS_TYPE_H
