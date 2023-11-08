#include "menuSystem.h"
#include <stdio.h>
#include <iostream>
#include <string>
#include "types/menuItemsType.h"
#include <3ds.h>
#include <citro2d.h>
#include "menus.h"

MenuSystem::MenuSystem()
{
	this->menuTextBuf = C2D_TextBufNew(4096);
	C2D_TextParse(&this->menuText, this->menuTextBuf, "");

	headers = {
		{&gameIDMenuItems, "Select a game ID"},
		{&saveSelectionMenuItems, "Select a save"},
		{&downloadGameMenuItems, "Select a game ID"},
		{&gameIDDirectoryMenuItems, "Select a game ID"},
		// {&uploadDirectoryMenuItems, "Select a directory"},
		{&mainMenuItems, "Main Menu"},
		{&uploadMenuItems, "Upload Menu"},
		{&downloadMenuItems, "Download Menu"},
		{&settingMenuItems, "Settings Menu"},
	};

	//  ABXY
	footers = {
		// {&uploadDirectoryMenuItems, " Open Directory   Prev Directory\n Confirm Current     Cancel"},
		{&existingGameIDsMenuItems, " Rename ID        Change Directory\n Delete ID           Cancel"}};
}

C2D_TextBuf *MenuSystem::getMenuTextBuf()
{
	return &this->menuTextBuf;
}

C2D_Text *MenuSystem::getMenuText()
{
	return &this->menuText;
}

std::string MenuSystem::getMenuString(menuItems &items)
{
	std::string str = "";
	for (auto &item : items)
		str += std::get<0>(item) + "\n";

	return str;
}

menuItems *MenuSystem::getCurrentMenuItems()
{
	return currentMenuItems;
}

std::string *MenuSystem::getFooter(menuItems *footer)
{
	return &footers[footer];
}

std::string *MenuSystem::getHeader(menuItems *header)
{
	return &headers[header];
}

void MenuSystem::handleExit()
{
	// Delete the text buffers
	if (this->menuTextBuf != nullptr)
	{
		C2D_TextBufDelete(this->menuTextBuf);
		this->menuTextBuf = nullptr;
	}
}

void MenuSystem::changeMenu(int &selection, menuItems *&oldMenuItems, menuItems &newMenuItems, std::vector<menuItems *> &previousMenus, bool dontAddToPreviousMenus)
{

	// This is used to do specific things depending on the menu
	currentMenuItems = &newMenuItems;

	if (!dontAddToPreviousMenus && &newMenuItems != &uploadDirectoryMenuItems && &newMenuItems != &existingGameIDsMenuItems)
	{
		previousMenus.push_back(&newMenuItems);
	}

	oldMenuItems = &newMenuItems;
	C2D_TextBufClear(this->menuTextBuf);

	std::string headerSpaceString = "";
	if (getHeader(&newMenuItems) != nullptr)
	{
		headerSpaceString += "\n";
	}

	C2D_TextParse(&this->menuText, this->menuTextBuf, (headerSpaceString + getMenuString(newMenuItems)).c_str());
	C2D_TextOptimize(&this->menuText);

	selection = 0;
}

void MenuSystem::goToPreviousMenu(int &selection, menuItems *&currentMenuItems, std::vector<menuItems *> &previousMenus)
{
	if (!previousMenus.empty() && previousMenus.size() >= 2)
	{
		menuItems *secondLastItem = previousMenus[previousMenus.size() - 2];

		previousMenus.pop_back();

		previousMenus.pop_back();

		if (secondLastItem)
		{
			changeMenu(selection, currentMenuItems, *secondLastItem, previousMenus);
		}
	}
}