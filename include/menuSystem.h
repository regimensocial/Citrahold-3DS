#ifndef MENUSYSTEM_H
#define MENUSYSTEM_H

#include <string>
#include <iostream>
#include "types/menuItemsType.h"
#include <3ds.h>
#include <citro2d.h>
#include <map>
#include <types/menuItemsType.h>

class MenuSystem
{

public:

    MenuSystem();
    
    void changeMenu(int &selection, menuItems *&oldMenuItems, menuItems &newMenuItems, std::vector<menuItems *> &previousMenus, bool dontAddToPreviousMenus = false);
    void goToPreviousMenu(int &selection, menuItems *&currentMenuItems, std::vector<menuItems *> &previousMenus);
    void handleExit();
    
    std::string getMenuString(menuItems &items);
    C2D_TextBuf* getMenuTextBuf();
    C2D_Text* getMenuText();
    menuItems* getCurrentMenuItems();

    std::string* getHeader(menuItems* header);
    std::string* getFooter(menuItems* footer);

private:
    C2D_TextBuf menuTextBuf;
    C2D_Text menuText;

    menuItems* currentMenuItems;

    
    std::map<menuItems*, std::string> headers;

    std::map<menuItems*, std::string> footers;

};

#endif // MENU_H