#include "systemCore.h"
#include <3ds.h>
#include <citro2d.h>
#include "menuSystem.h"
#include "menus.h"
#include "networkSystem.h"
#include "configManager.h"
#include "directoryMenu.h"
#include "json.hpp"

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

void SystemCore::handleInput()
{
    hidScanInput();

    // get volume slider value
    u8 volume;
    HIDUSER_GetSoundVolume(&volume);
    size = 0.6f * (1.0f - volume / 63.0f);

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
        switch (std::get<1>((*currentMenuItems)[selection]))
        {
        case menuFunctions::upload:
            menuSystem.changeMenu(selection, currentMenuItems, uploadMenuItems, previousMenus);
            break;

        case menuFunctions::download:
            menuSystem.changeMenu(selection, currentMenuItems, downloadMenuItems, previousMenus);
            break;

        case menuFunctions::settings:
            menuSystem.changeMenu(selection, currentMenuItems, settingMenuItems, previousMenus);
            break;

        case menuFunctions::changeToMainMenu:
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

            menuSystem.changeMenu(selection, currentMenuItems, uploadDirectoryMenuItems, previousMenus, 1, false);

            break;
        }

        case menuFunctions::gameIDSavesMenuItems:
        {

            nlohmann::json gameIDJSON = configManager.getGameIDFile(UploadTypeEnum::SAVES);
            gameIDMenuItems = directoryMenu.getGameIDDirectoryMenuItems(gameIDJSON);

            if (gameIDMenuItems.size() == 0)
            {
                std::cout << "No game IDs found, please add one" << std::endl;
                break;
            }

            currentUploadType = UploadTypeEnum::SAVES;
            menuSystem.changeMenu(selection, currentMenuItems, gameIDMenuItems, previousMenus, 1, false);

            break;
        }

        case menuFunctions::gameIDExtdataMenuItems:
        {

            nlohmann::json gameIDJSON = configManager.getGameIDFile(UploadTypeEnum::EXTDATA);
            gameIDMenuItems = directoryMenu.getGameIDDirectoryMenuItems(gameIDJSON);

            if (gameIDMenuItems.size() == 0)
            {
                std::cout << "No game IDs found, please add one" << std::endl;
                break;
            }

            currentUploadType = UploadTypeEnum::EXTDATA;
            menuSystem.changeMenu(selection, currentMenuItems, gameIDMenuItems, previousMenus, 1, false);

            break;
        }

        case menuFunctions::noAction:
            if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
            {
                std::filesystem::path newDirectory = directoryMenu.getCurrentDirectory() / std::get<0>(uploadDirectoryMenuItems[selection]);
                if (selection == 0)
                {
                    newDirectory = directoryMenu.getCurrentDirectory().parent_path();
                }

                directoryMenu.setCurrentDirectory(newDirectory);
                uploadDirectoryMenuItems = directoryMenu.getCurrentDirectoryMenuItems();
                menuSystem.changeMenu(selection, currentMenuItems, uploadDirectoryMenuItems, previousMenus, 1, true);
            }
            else if (menuSystem.getCurrentMenuItems() == &gameIDMenuItems)
            {
            }

            break;

        case menuFunctions::saveSelectionMenuItems:
        {
            nlohmann::json gameIDJSON = configManager.getGameIDFile(currentUploadType);
            currentGameID = std::get<0>(gameIDMenuItems[selection]);
            std::filesystem::path gamePath = directoryMenu.getGamePathFromGameID(currentGameID, gameIDJSON);
            saveSelectionMenuItems = directoryMenu.getSaveSelectionMenuItems(gamePath);
            menuSystem.changeMenu(selection, currentMenuItems, saveSelectionMenuItems, previousMenus, 1, false);
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

                for (const auto &dirEntry : std::filesystem::recursive_directory_iterator(savePath)) // this does ALL saves, not specific ones
                {

                    if (std::filesystem::is_regular_file(dirEntry) && (prevStatus == 0 || prevStatus == 400)) // change to if not 201/200
                    {
                        // This entry is a file, process it
                        std::filesystem::path fullPath = dirEntry.path();
                        std::filesystem::path relativePath = std::filesystem::relative(dirEntry, savePath);

                        responsePair theResponse;

                        networkSystem.upload(currentUploadType, currentGameID / relativePath, networkSystem.getBase64StringFromFile(fullPath.string(), relativePath.string()));

                        prevStatus = theResponse.first;
                    }
                }

                printf("Upload has finished...?\n");
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
            menuSystem.changeMenu(selection, currentMenuItems, downloadGameMenuItems, previousMenus, 1, false);
            break;
        }
        
        case menuFunctions::downloadExtdataMenuItems:
        {
            currentUploadType = UploadTypeEnum::EXTDATA;
            downloadGameMenuItems = networkSystem.getGamesMenuItems(UploadTypeEnum::EXTDATA);
            menuSystem.changeMenu(selection, currentMenuItems, downloadGameMenuItems, previousMenus, 1, false);
            break;
        }


        case menuFunctions::downloadGame: {
            nlohmann::json gameIDJSON = configManager.getGameIDFile(currentUploadType);
            
            currentGameID = std::get<0>(downloadGameMenuItems[selection]);

            std::filesystem::path gamePath = directoryMenu.getGamePathFromGameID(currentGameID, gameIDJSON);
            if (gamePath == "")
            {
                std::cout << "I need you to register this game as " << currentGameID << " before you download this save" << std::endl;
                break;
            }
            networkSystem.download(currentUploadType, currentGameID, gamePath);
            
            break;
        }

        case menuFunctions::resetGameIDFiles: {
            currentGameID = "";
            configManager.resetBothGameIDFiles();

            break;
        }

        default:
            // Handle unhandled values or provide a default behavior
            break;

            // Add more cases for other menu functions if needed
        }
    }

    if (kDown & KEY_X)
    {
        if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
        {
            menuSystem.changeMenu(selection, currentMenuItems, uploadMenuItems, previousMenus, 0, true);
        }
    }

    if (kDown & KEY_Y)
    {
        if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
        {
            if (directoryMenu.getCurrentDirectory() != "/" && directoryMenu.getCurrentDirectory() != "" && directoryMenu.getCurrentDirectory() != "/3ds" && directoryMenu.getCurrentDirectory() != "/Nintendo 3DS")
            {

                std::string gameID = openKeyboard("Enter a game ID (this must EXACTLY match your PC one)", std::get<0>(uploadDirectoryMenuItems[selection]));
                if (gameID != "")
                {
                    // we need to add [gameID, path] to "gameID" of configManager.getGameIDFile()

                    UploadTypeEnum uploadType = UploadTypeEnum::SAVES;
                    if (directoryMenu.getCurrentDirectory().string().find("extdata") != std::string::npos)
                    {
                        uploadType = UploadTypeEnum::EXTDATA;
                    }

                    nlohmann::json oldGameIDFile = configManager.getGameIDFile(uploadType);
                    nlohmann::json newEntry = nlohmann::json::array();
                    newEntry.push_back(gameID);
                    newEntry.push_back(directoryMenu.getCurrentDirectory());
                    oldGameIDFile["gameID"].push_back(newEntry);

                    // check if that directory contains extdata or save data

                    configManager.updateGameIDFile(uploadType, oldGameIDFile);
                }
            }
        }
    }

    if (kDown & KEY_B)
    {
        if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
        {

            menuSystem.changeMenu(selection, currentMenuItems, loadingMenuItems, previousMenus, 1, true);
            std::filesystem::path newDirectory = directoryMenu.getCurrentDirectory().parent_path();
            directoryMenu.setCurrentDirectory(newDirectory);
            uploadDirectoryMenuItems = directoryMenu.getCurrentDirectoryMenuItems();
            menuSystem.changeMenu(selection, currentMenuItems, uploadDirectoryMenuItems, previousMenus, 1, true);
        }
        else
        {
            menuSystem.goToPreviousMenu(selection, currentMenuItems, previousMenus);
        }
    }
}

void SystemCore::sceneRender()
{
    // if (menuSystem.getCurrentMenuItems() == &uploadMenuItems)
    // {
    //     std::cout << "Upload menu" << std::endl;
    // }

    std::string str = "";
    if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
    {
        str += std::string(directoryMenu.getCurrentDirectory()) + "\n";
    }
    else if (menuSystem.getCurrentMenuItems() == &gameIDMenuItems)
    {
        str += "Select a game ID\n";
    }
    else if (menuSystem.getCurrentMenuItems() == &saveSelectionMenuItems)
    {
        str += "Select a save to upload\n";
    }
    else if (menuSystem.getCurrentMenuItems() == &downloadGameMenuItems)
    {
        str += "Select a game to download\n";
    }
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

    // TODO: maximum 7 shown at once before scrolling

    if (selection > 0)
    {
        for (int i = 0; i < selection; i++)
        {
            str += "\n";
        }
    }

    str += "";

    for (size_t i = 0; i < ((renderMenuItems).size() - selection); i++)
    {
        str += "\n";
    }

    if (selection > (int)(renderMenuItems).size() - 1)
    {
        selection = (int)(renderMenuItems).size() - 1;
    }
    str += "Selected \"" + std::get<0>((renderMenuItems)[selection]) + "\"";

    //  ABXY

    if (menuSystem.getCurrentMenuItems() == &uploadDirectoryMenuItems)
    {
        str += "\n Open Directory   Prev Directory\n Confirm             Cancel";
    }

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

std::string SystemCore::openKeyboard(std::string placeholder, std::string initialText) // SwkbdState &swkbd, SwkbdButton &button, char *mybuf
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
            break; // An actual error happened, display it

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

    // previousMenus used to be a global variable, it's not now
    // the comment below might be redundant, but it does no harm to keep it

    // THIS LINE IS VITAL, WITHOUT IT, THE SYSTEM WILL CRASH
    // DO NOT REMOVE
    previousMenus.shrink_to_fit();

    // Deinitialise the scene
    selection = 0;

    menuSystem.handleExit();

    // safely remove mainMenuItems

    (*currentMenuItems).clear();
    (*currentMenuItems).shrink_to_fit();

    mainMenuItems.clear();
    uploadMenuItems.clear();
    downloadMenuItems.clear();
    settingMenuItems.clear();
    uploadDirectoryMenuItems.clear();
    loadingMenuItems.clear();
    gameIDMenuItems.clear();
    saveSelectionMenuItems.clear();
    downloadGameMenuItems.clear();

    networkSystem.cleanExit();

    C2D_Fini();
    C3D_Fini();
    gfxExit();
}