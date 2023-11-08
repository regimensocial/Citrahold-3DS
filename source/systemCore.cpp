#include "systemCore.h"
#include <3ds.h>
#include <citro2d.h>
#include "menuSystem.h"
#include "menus.h"
#include "networkSystem.h"
#include "configManager.h"
#include "directoryMenu.h"
#include "json.hpp"
#include <sstream>


SystemCore::SystemCore() : networkSystem()
{
    // Initialise the libs
    gfxInitDefault();

    consoleInit(GFX_BOTTOM, NULL);

    std::cout << "\n- - - - - - - - - - - - - - - - -\n\nWelcome to Citrahold." << std::endl;
    std::cout << "Press START at any point to exit.\n\nPlease use the D-pad to navigate, \nand use A to confirm actions.\n\nYou can ignore this console for \nmost intents and purposes.\n\nIf above text is illegible, please \nreduce volume now.\n\n- - - - - - - - - - - - - - - - -"
              << std::endl;

    configManager = ConfigManager();

    checkServerConnection();

    if (isServerAccessible && configManager.loggedIn())
    {
        std::cout << "Game saves on server: " << networkSystem.getGamesMenuItems(UploadTypeEnum::SAVES).size() << std::endl;
    }

    menuSystem = MenuSystem();

    directoryMenu = DirectoryMenu();

    selectorBuf = C2D_TextBufNew(4096);

    C3D_Init(C3D_DEFAULT_CMDBUF_SIZE);
    C2D_Init(C2D_DEFAULT_MAX_OBJECTS);
    C2D_Prepare();

    // menuSystem = MenuSystem();
    // selectorBuf = C2D_TextBufNew(4096);

    // Create screen
    screen = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    size = 0.6f;
    selection = 0;
    halt = false;
    shouldKeyboardBeOpen = false;
    currentUploadType = UploadTypeEnum::SAVES;

    menuSystem.changeMenu(selection, currentMenuItems, mainMenuItems, previousMenus);
}

void SystemCore::checkServerConnection()
{
    std::cout << "\nGetting server status...\nThis should be INSTANT!\n"
              << std::endl;
    responsePair serverStatus = networkSystem.init(configManager.getConfig()["serverAddress"], configManager.getToken());
    std::cout << "Reported server status: HTTP " << serverStatus.first << std::endl;

    // at some point, we should check if they're connected to the internet
    std::cout << "This is " << (serverStatus.first == 200 ? "OK" : "NOT OK\nTHIS MAKES NO SENSE AT ALL") << std::endl
              << std::endl;

    isServerAccessible = (serverStatus.first == 200);

    if (isServerAccessible)
    {
        if (configManager.getToken() != "" && configManager.getToken() != "unknown")
        {
            std::string userID = networkSystem.verifyTokenToSetUserID(configManager.getToken());
            if (userID != "invalid")
            {
                std::cout << "Successfully authenticated as user\n"
                          << userID << std::endl;
            }
            else
            {
                std::cout << "Invalid token currently saved." << std::endl;
            }
        }
        else
        {
            std::cout << "Please go to Settings and input\na shorthand token!" << std::endl;
        }
    }
}

void SystemCore::handleFunction(menuFunctions function, unsigned int key)
{
    switch (key)
    {

    case (KEY_B):
    {
        if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
        {

            handleFunction(menuFunctions::specialB_goPreviousDirectory);
        }
        else if (menuSystem.getCurrentMenuItems() == &existingGameIDsMenuItems)
        {
            handleFunction(menuFunctions::specialB_exitExistingGameIDSelection);
        }
        else
        {
            menuSystem.goToPreviousMenu(selection, currentMenuItems, previousMenus);
        }
        break;
    }

    case (KEY_X):
    {
        if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
        {
            handleFunction(menuFunctions::specialX_exitDirectorySelection);
        }
        else if (menuSystem.getCurrentMenuItems() == &existingGameIDsMenuItems)
        {

            handleFunction(menuFunctions::specialX_redirectGameID);
        }
        else
        {
            menuSystem.goToPreviousMenu(selection, currentMenuItems, previousMenus);
        }
        break;
    }

    case (KEY_Y):
    {
        if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
        {
            handleFunction(menuFunctions::specialY_enterGameID);
        }
        else if (menuSystem.getCurrentMenuItems() == &existingGameIDsMenuItems)
        {
            handleFunction(menuFunctions::specialY_deleteGameID);
        }
        break;
    }

    case (KEY_LEFT):
    {
        if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
        {
            handleFunction(menuFunctions::specialB_goPreviousDirectory);
        }
        break;
    }

    case (KEY_RIGHT):
    {
        if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
        {
            handleFunction(menuFunctions::traverseDirectory);
        }
        break;
    }

    default:
    {
        switch (function)
        {

        case menuFunctions::specialX_exitDirectorySelection:
        {
            std::cout << currentGameID << std::endl;
            if (currentGameID == "")
            {
                menuSystem.changeMenu(selection, currentMenuItems, mainMenuItems, previousMenus);
            }
            else
            {
                handleFunction(currentUploadType == UploadTypeEnum::SAVES ? menuFunctions::existingGameIDSavesMenuItems : menuFunctions::existingGameIDExtdataMenuItems);
            }

            break;
        }

        case menuFunctions::specialB_exitExistingGameIDSelection:
        {

            menuSystem.changeMenu(selection, currentMenuItems, gameIDDirectoryMenuItems, previousMenus);
            break;
        }

        case menuFunctions::specialB_goPreviousDirectory:
        {
            std::filesystem::path newDirectory = directoryMenu.getCurrentDirectory().parent_path();
            directoryMenu.setCurrentDirectory(newDirectory);
            uploadDirectoryMenuItems = directoryMenu.getCurrentDirectoryMenuItems();
            menuSystem.changeMenu(selection, currentMenuItems, uploadDirectoryMenuItems, previousMenus, true);
            break;
        }

        case menuFunctions::specialY_enterGameID:
        {
            if (directoryMenu.getCurrentDirectory() != "/" && directoryMenu.getCurrentDirectory() != "" && directoryMenu.getCurrentDirectory() != "/3ds" && directoryMenu.getCurrentDirectory() != "/Nintendo 3DS")
            {

                std::string gameID = (currentGameID == "") ? openKeyboard("Enter a game ID (this must EXACTLY match your PC one)") : currentGameID;

                if (gameID != "")
                {
                    // we need to add [gameID, path] to "gameID" of configManager.getGameIDFile()

                    UploadTypeEnum uploadType = UploadTypeEnum::SAVES;
                    if (directoryMenu.getCurrentDirectory().string().find("extdata") != std::string::npos)
                    {
                        uploadType = UploadTypeEnum::EXTDATA;
                    }

                    configManager.addGameIDToFile(uploadType, gameID, directoryMenu.getCurrentDirectory());

                    handleFunction(menuFunctions::specialX_exitDirectorySelection);

                    currentGameID = "";
                }
            }
            break;
        }

        case menuFunctions::specialX_redirectGameID:
        {
            currentGameID = std::get<0>(existingGameIDsMenuItems[selection]);
            directoryMenu.setCurrentDirectory(configManager.getGamePathFromID(currentUploadType, currentGameID));
            uploadDirectoryMenuItems = directoryMenu.getCurrentDirectoryMenuItems();
            menuSystem.changeMenu(selection, currentMenuItems, uploadDirectoryMenuItems, previousMenus, true);

            break;
        }

        case menuFunctions::specialY_deleteGameID:
        {
            configManager.removeGameIDFromFile(currentUploadType, std::get<0>(existingGameIDsMenuItems[selection]));

            if (configManager.getNumberOfGameIDs(currentUploadType) == 0)
            {
                handleFunction(menuFunctions::specialB_exitExistingGameIDSelection);
            }
            else
            {

                handleFunction(currentUploadType == UploadTypeEnum::SAVES ? menuFunctions::existingGameIDSavesMenuItems : menuFunctions::existingGameIDExtdataMenuItems);
            }

            break;
        }

        case menuFunctions::renameGameID:
        {
            std::string newGameID = openKeyboard("Enter a new ID to replace " + std::get<0>(existingGameIDsMenuItems[selection]), std::get<0>(existingGameIDsMenuItems[selection]));
            if (newGameID != "")
            {
                configManager.renameGameIDInFile(currentUploadType, std::get<0>(existingGameIDsMenuItems[selection]), newGameID);
                handleFunction(currentUploadType == UploadTypeEnum::SAVES ? menuFunctions::existingGameIDSavesMenuItems : menuFunctions::existingGameIDExtdataMenuItems);
            }

            break;
        }

        case menuFunctions::gameIDDirectoryMenuItems:
        {
            currentGameID = "";
            menuSystem.changeMenu(selection, currentMenuItems, gameIDDirectoryMenuItems, previousMenus);
            break;
        }

        case menuFunctions::uploadMenuItems:
            menuSystem.changeMenu(selection, currentMenuItems, uploadMenuItems, previousMenus);
            break;

        case menuFunctions::downloadMenuItems:
            menuSystem.changeMenu(selection, currentMenuItems, downloadMenuItems, previousMenus);
            break;

        case menuFunctions::settingsMenuItems:
            menuSystem.changeMenu(selection, currentMenuItems, settingMenuItems, previousMenus);
            break;

        case menuFunctions::mainMenuMenuItems:
            menuSystem.changeMenu(selection, currentMenuItems, mainMenuItems, previousMenus);
            break;

        case menuFunctions::changeToPreviousMenu:
            // changeMenu(selection, items, *previousMenu);

            menuSystem.goToPreviousMenu(selection, currentMenuItems, previousMenus);
            break;

        case menuFunctions::inputToken:
        {
            std::string token = openKeyboard("Enter a shorthand token");

            if (token != "")
            {
                if (token.length() > 6)
                {
                    // FULL TOKEN
                    std::string userID = networkSystem.verifyTokenToSetUserID(token);
                    if (userID != "invalid")
                    {
                        configManager.userID = userID;
                        configManager.setToken(token);
                        std::cout << "Successfully authenticated as user" << userID << std::endl;
                    }
                    else
                    {
                        std::cout << "User inputted invalid token" << std::endl;
                    }
                }
                else
                {
                    // SHORTHAND TOKEN
                    std::string fullToken = networkSystem.getTokenFromShorthandToken(token);
                    if (fullToken != "invalid")
                    {
                        std::string userID = networkSystem.verifyTokenToSetUserID(fullToken);
                        if (userID != "invalid")
                        {
                            configManager.userID = userID;
                            configManager.setToken(fullToken);
                            std::cout << "Successfully authenticated as user\n"
                                      << userID << std::endl;
                        }
                    }
                    else
                    {
                        std::cout << "User inputted invalid token" << std::endl;
                    }
                }
            }
            break;
        }

        case menuFunctions::checkServerConnection:
        {

            checkServerConnection();

            break;
        }

        case menuFunctions::openKeyboard:
            std::cout << openKeyboard() << std::endl;
            break;

        case menuFunctions::directoryMenuItems:
        {

            uploadDirectoryMenuItems = directoryMenu.getCurrentDirectoryMenuItems();

            menuSystem.changeMenu(selection, currentMenuItems, uploadDirectoryMenuItems, previousMenus, false);

            break;
        }

        case menuFunctions::gameIDSavesMenuItems:
        {

            nlohmann::json gameIDJSON = configManager.getGameIDFile(UploadTypeEnum::SAVES);
            gameIDMenuItems = directoryMenu.getGameIDDirectoryMenuItems(gameIDJSON, menuFunctions::saveSelectionMenuItems);

            if (gameIDMenuItems.size() == 0)
            {
                std::cout << "No game IDs found, please add one" << std::endl;
                break;
            }

            currentUploadType = UploadTypeEnum::SAVES;
            menuSystem.changeMenu(selection, currentMenuItems, gameIDMenuItems, previousMenus, false);

            break;
        }

        case menuFunctions::gameIDExtdataMenuItems:
        {

            nlohmann::json gameIDJSON = configManager.getGameIDFile(UploadTypeEnum::EXTDATA);
            gameIDMenuItems = directoryMenu.getGameIDDirectoryMenuItems(gameIDJSON, menuFunctions::saveSelectionMenuItems);

            if (gameIDMenuItems.size() == 0)
            {
                std::cout << "No game IDs found, please add one" << std::endl;
                break;
            }

            currentUploadType = UploadTypeEnum::EXTDATA;
            menuSystem.changeMenu(selection, currentMenuItems, gameIDMenuItems, previousMenus, false);

            break;
        }

        case menuFunctions::traverseDirectory:
        {
            std::filesystem::path newDirectory = directoryMenu.getCurrentDirectory() / std::get<0>(uploadDirectoryMenuItems[selection]);
            if (selection == 0)
            {
                newDirectory = directoryMenu.getCurrentDirectory().parent_path();
            }

            directoryMenu.setCurrentDirectory(newDirectory);
            uploadDirectoryMenuItems = directoryMenu.getCurrentDirectoryMenuItems();
            menuSystem.changeMenu(selection, currentMenuItems, uploadDirectoryMenuItems, previousMenus, true);
            break;
        }

        case menuFunctions::saveSelectionMenuItems:
        {
            nlohmann::json gameIDJSON = configManager.getGameIDFile(currentUploadType);
            currentGameID = std::get<0>(gameIDMenuItems[selection]);
            std::filesystem::path gamePath = directoryMenu.getGamePathFromGameID(currentGameID, gameIDJSON);
            saveSelectionMenuItems = directoryMenu.getSaveSelectionMenuItems(gamePath);
            menuSystem.changeMenu(selection, currentMenuItems, saveSelectionMenuItems, previousMenus, false);
            break;
        }

        case menuFunctions::uploadGame:
        {

            nlohmann::json gameIDJSON = configManager.getGameIDFile(currentUploadType);
            std::filesystem::path gamePath = directoryMenu.getGamePathFromGameID(currentGameID, gameIDJSON);
            std::filesystem::path savePath = gamePath / std::get<0>(saveSelectionMenuItems[selection]);

            try
            {
                int prevStatus = 0;
                // get GAME PATH not save path
                std::filesystem::path gamePath = directoryMenu.getGamePathFromGameID(currentGameID, gameIDJSON);

                for (const auto &dirEntry : std::filesystem::recursive_directory_iterator(savePath))
                {

                    if (std::filesystem::is_regular_file(dirEntry) && (prevStatus == 0 || prevStatus == 400)) // change to if not 201/200
                    {
                        std::filesystem::path fullPath = dirEntry.path();
                        std::filesystem::path relativePath = std::filesystem::relative(dirEntry, savePath);

                        prevStatus = networkSystem.upload(currentUploadType, currentGameID / relativePath, networkSystem.getBase64StringFromFile(fullPath.string(), relativePath.string()));
                        std::cout << "HTTP " << prevStatus << std::endl;
                    }
                }

                if (prevStatus == 201 || prevStatus == 200)
                {
                    std::cout << "Upload successful" << std::endl;
                }
                else if (prevStatus == 0)
                {
                    std::cout << "Upload failed. Is there anything to upload? (or could network be offline?)" << std::endl;
                }
                else
                {
                    std::cout << "Upload failed" << std::endl;
                }
            }
            catch (std::filesystem::filesystem_error &e)
            {
                std::cout << e.what() << std::endl;
            }

            break;
        }

        case menuFunctions::downloadSavesMenuItems:
        {
            currentUploadType = UploadTypeEnum::SAVES;
            downloadGameMenuItems = networkSystem.getGamesMenuItems(UploadTypeEnum::SAVES);
            menuSystem.changeMenu(selection, currentMenuItems, downloadGameMenuItems, previousMenus, false);
            break;
        }

        case menuFunctions::downloadExtdataMenuItems:
        {
            currentUploadType = UploadTypeEnum::EXTDATA;
            downloadGameMenuItems = networkSystem.getGamesMenuItems(UploadTypeEnum::EXTDATA);
            menuSystem.changeMenu(selection, currentMenuItems, downloadGameMenuItems, previousMenus, false);
            break;
        }

        case menuFunctions::downloadGame:
        {
            nlohmann::json gameIDJSON = configManager.getGameIDFile(currentUploadType);

            currentGameID = std::get<0>(downloadGameMenuItems[selection]);

            std::filesystem::path gamePath = directoryMenu.getGamePathFromGameID(currentGameID, gameIDJSON);
            if (gamePath == "")
            {
                std::cout << "You need to register this game as\n" << currentGameID << " before you download this save" << std::endl;
                break;
            }

            std::cout << "Downloading " << currentGameID << " to " << gamePath << std::endl;
            bool downloadSuccessful = networkSystem.download(currentUploadType, currentGameID, gamePath);
            std::cout << (downloadSuccessful ? "Download successful" : "Download failed") << std::endl;

            break;
        }

        case menuFunctions::existingGameIDExtdataMenuItems:
        {
            nlohmann::json gameIDJSON = configManager.getGameIDFile(UploadTypeEnum::EXTDATA);
            gameIDMenuItems = directoryMenu.getGameIDDirectoryMenuItems(gameIDJSON, menuFunctions::renameGameID);

            if (gameIDMenuItems.size() == 0)
            {
                std::cout << "No game IDs found" << std::endl;
                break;
            }

            currentUploadType = UploadTypeEnum::EXTDATA;
            existingGameIDsMenuItems = gameIDMenuItems;
            menuSystem.changeMenu(selection, currentMenuItems, existingGameIDsMenuItems, previousMenus);

            break;
        }

        case menuFunctions::existingGameIDSavesMenuItems:
        {
            nlohmann::json gameIDJSON = configManager.getGameIDFile(UploadTypeEnum::SAVES);
            gameIDMenuItems = directoryMenu.getGameIDDirectoryMenuItems(gameIDJSON, menuFunctions::renameGameID);

            if (gameIDMenuItems.size() == 0)
            {
                std::cout << "No game IDs found" << std::endl;
                break;
            }

            currentUploadType = UploadTypeEnum::SAVES;
            existingGameIDsMenuItems = gameIDMenuItems;
            menuSystem.changeMenu(selection, currentMenuItems, existingGameIDsMenuItems, previousMenus);

            break;
        }

        case menuFunctions::resetGameIDFiles:
        {
            currentGameID = "";
            configManager.resetBothGameIDFiles();

            break;
        }

        default:
            
            break;
        }

        break;
    }
    }
}

void SystemCore::handleInput()
{
    hidScanInput();

    // get volume slider value
    u8 volume;
    HIDUSER_GetSoundVolume(&volume);
    size = 0.6f * (1.0f - volume / 63.0f);
    // volume can be used to increase/decrease text size

    // Respond to user input
    u32 kDown = hidKeysDown();
    // u32 kHeld = hidKeysHeld();
    if (kDown & KEY_START)
        halt = true; // break in order to return to hbmenu

    if (kDown & KEY_UP)
        if (selection <= 0)
            selection = (*currentMenuItems).size() - 1;
        else
            selection--;
    else if (kDown & KEY_DOWN)
    {
        if (selection + 1 >= (int)(*currentMenuItems).size())
            selection = 0;
        else
            selection++;
    }

    if (kDown & KEY_A)
    {
        handleFunction(std::get<1>((*currentMenuItems)[selection]), KEY_A);
    }
    else if (kDown & KEY_B)
    {
        handleFunction(std::get<1>((*currentMenuItems)[selection]), KEY_B);
    }
    else if (kDown & KEY_X)
    {
        handleFunction(std::get<1>((*currentMenuItems)[selection]), KEY_X);
    }
    else if (kDown & KEY_Y)
    {
        handleFunction(std::get<1>((*currentMenuItems)[selection]), KEY_Y);
    }
    else if (kDown & KEY_RIGHT)
    {
        handleFunction(std::get<1>((*currentMenuItems)[selection]), KEY_RIGHT);
    }
    else if (kDown & KEY_LEFT)
    {
        handleFunction(std::get<1>((*currentMenuItems)[selection]), KEY_LEFT);
    }
}

void SystemCore::sceneRender()
{
    std::string pointerSymbol = "";
    std::string str = "";
    if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
    {
        pointerSymbol = ">";
        str += std::string(directoryMenu.getCurrentDirectory()) + "\n";
    }
    else if (menuSystem.getCurrentMenuItems() == &existingGameIDsMenuItems)
    {
        pointerSymbol = ">";
        std::string currentType = (currentUploadType == UploadTypeEnum::EXTDATA) ? "extdata" : "saves";
        str += "Current " + currentType + " game IDs" + "\n";
    }
    else if (menuSystem.getHeader(currentMenuItems) != nullptr)
        str += *(menuSystem.getHeader(currentMenuItems)) + "\n";

    menuItems renderMenuItems = *currentMenuItems;

    C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
    C2D_TargetClear(screen, C2D_Color32(0xFF, 0xDA, 0x75, 0xFF));
    C2D_SceneBegin(screen);

    // Clear the dynamic text buffer
    C2D_TextBufClear(*menuSystem.getMenuTextBuf());
    C2D_TextBufClear(selectorBuf);

    // Draw static text strings

    // C2D_DrawText(&g_staticText[1], 0, 16.0f, 215.0f, 0.5f, size, size);
    C2D_DrawText(menuSystem.getMenuText(), 0, 38.0f, 16.0f, 0.5f, size, size);
    // Generate and draw dynamic text

    C2D_Text dynText;

    if (selection > 0)
    {
        for (int i = 0; i < selection; i++)
        {
            str += "\n";
        }
    }

    str += pointerSymbol;

    for (size_t i = 0; i < ((renderMenuItems).size() - selection); i++)
    {
        str += "\n";
    }

    if (selection > (int)(renderMenuItems).size() - 1)
    {
        selection = (int)(renderMenuItems).size() - 1;
    }

    if (menuSystem.getCurrentMenuItems() != &uploadDirectoryMenuItems)
    {
        str += "Selected \"" + std::get<0>((renderMenuItems)[selection]) + "\"";
    }

    //  ABXY

    if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
    {
        std::stringstream ss;
        ss << ((selection == 0) ? "/" : "") << " Prev Directory " + (selection == 0 ? "" : " Open " + std::get<0>((renderMenuItems)[selection])) + "\n Cancel     Confirm " + std::string(directoryMenu.getCurrentDirectory());
        str += ss.str();
    }
    else if (menuSystem.getFooter(currentMenuItems) != nullptr)
        str += "\n" + *(menuSystem.getFooter(currentMenuItems));

    C2D_TextParse(&dynText, selectorBuf, str.c_str());
    C2D_TextOptimize(&dynText);
    C2D_DrawText(&dynText, 0, 16.0f, 16.0f, 0.5f, size, size);
    C3D_FrameEnd(0);
}

static SwkbdCallbackResult KeyboardCallback(void *user, const char **ppMessage, const char *text, size_t textlen)
{
    // if (strstr(text, "lenny"))
    // {
    //     *ppMessage = "Nice try but I'm not letting you use that meme right now";
    //     return SWKBD_CALLBACK_CONTINUE;
    // }

    return SWKBD_CALLBACK_OK;
}

std::string SystemCore::openKeyboard(std::string placeholder, std::string initialText)
{

    static SwkbdState swkbd;
    static char mybuf[60];
    SwkbdButton button = SWKBD_BUTTON_NONE;

    shouldKeyboardBeOpen = true;
    swkbdInit(&swkbd, SWKBD_TYPE_WESTERN, 2, -1);
    swkbdSetValidation(&swkbd, SWKBD_NOTEMPTY_NOTBLANK, 0, 0);
    swkbdSetFeatures(&swkbd, SWKBD_DARKEN_TOP_SCREEN | SWKBD_ALLOW_HOME | SWKBD_ALLOW_RESET | SWKBD_ALLOW_POWER);
    swkbdSetFilterCallback(&swkbd, KeyboardCallback, NULL);

    if (placeholder != "")
    {
        swkbdSetHintText(&swkbd, placeholder.c_str());
    }

    if (initialText != "")
    {
        swkbdSetInitialText(&swkbd, initialText.c_str());
    }

    bool shouldQuit = false;
    mybuf[0] = 0;
    do
    {
        swkbdSetInitialText(&swkbd, mybuf);
        button = swkbdInputText(&swkbd, mybuf, sizeof(mybuf));
        if (button != SWKBD_BUTTON_NONE)
            break;

        SwkbdResult res = swkbdGetResult(&swkbd);
        if (res == SWKBD_RESETPRESSED)
        {
            shouldQuit = true;
            aptSetChainloaderToSelf();
            break;
        }
        else if (res != SWKBD_HOMEPRESSED && res != SWKBD_POWERPRESSED)
            break; 

        shouldQuit = !aptMainLoop();
    } while (!shouldQuit);

    std::string mybufAsString(mybuf);
    return mybufAsString;
}

bool SystemCore::isHalted()
{
    return halt;
}

void SystemCore::cleanExit()
{
    // Delete the text buffers
    if (selectorBuf != nullptr)
    {
        C2D_TextBufDelete(selectorBuf);
        selectorBuf = nullptr;
    }

    previousMenus.clear();

    previousMenus.shrink_to_fit();

    // Deinitialise the scene
    selection = 0;

    menuSystem.handleExit();

    // safely remove mainMenuItems

    (*currentMenuItems).clear();
    (*currentMenuItems).shrink_to_fit();

    mainMenuItems.clear();
    gameIDDirectoryMenuItems.clear();
    uploadMenuItems.clear();
    downloadMenuItems.clear();
    settingMenuItems.clear();
    uploadDirectoryMenuItems.clear();
    gameIDMenuItems.clear();
    saveSelectionMenuItems.clear();
    downloadGameMenuItems.clear();
    existingGameIDsMenuItems.clear();

    networkSystem.cleanExit();

    C2D_Fini();
    C3D_Fini();
    gfxExit();
}