#ifndef MENUSYSTEM_H
#define MENUSYSTEM_H

#include <string>
#include <iostream>
#include "types/menuItemsType.h"
#include <3ds.h>
#include <citro2d.h>

class MenuSystem
{

public:

    MenuSystem();
    
    void changeMenu(int &selection, menuItems *&oldMenuItems, menuItems &newMenuItems, std::vector<menuItems *> &previousMenus, int headerSpace = 0, bool dontAddToPreviousMenus = false);
    void goToPreviousMenu(int &selection, menuItems *&currentMenuItems, std::vector<menuItems *> &previousMenus);
    void handleExit();
    
    std::string getMenuString(menuItems &items);
    C2D_TextBuf* getMenuTextBuf();
    C2D_Text* getMenuText();
    menuItems* getCurrentMenuItems();

private:
    C2D_TextBuf menuTextBuf;
    C2D_Text menuText;

    menuItems* currentMenuItems;

};

#endif // MENU_H