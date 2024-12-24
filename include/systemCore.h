#ifndef SYSTEMCORE_H
#define SYSTEMCORE_H

#include <string>
#include <iostream>
#include <3ds.h>
#include <citro2d.h>
#include "menuSystem.h"
#include "types/menuItemsType.h"
#include "networkSystem.h"
#include "configManager.h"
#include "directoryMenu.h"

class SystemCore
{
public:
    SystemCore();

    void handleFunction(menuFunctions function, unsigned int key = 0);
    void handleInput();

    std::string openKeyboard(std::string placeholder = "", std::string initialText = "");
    std::string getVersion();

    void sceneRender();
    void cleanExit();
    void checkServerConnection();

    bool isHalted();

private:
    MenuSystem menuSystem;
    NetworkSystem networkSystem;
    ConfigManager configManager;
    DirectoryMenu directoryMenu;
    
    C2D_TextBuf selectorBuf;
    
    menuItems* currentMenuItems;
    std::vector<menuItems *> previousMenus;
    int zoom = 0;
    bool dontUseVolumeBar = true;
    
    C3D_RenderTarget *screen;
    UploadTypeEnum currentUploadType;
    std::string currentGameID;

    bool isServerAccessible = false;
    int selection;
    float size;
    bool halt;
    bool shouldKeyboardBeOpen;
};

#endif // SYSTEMCORE_H