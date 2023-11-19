#include "systemCore.h"
#include <3ds.h>
#include <citro2d.h>
#include <sstream>
#include "menuSystem.h"
#include "menus.h"
#include "networkSystem.h"
#include "configManager.h"
#include "directoryMenu.h"
#include "json.hpp"
#include "helpers.h"
#include "base64.h"

SystemCore::SystemCore() : networkSystem()
{
    gfxInitDefault();

    consoleInit(GFX_BOTTOM, NULL);

    std::cout << "\n- - - - - - - - - - - - - - - - -\n\nWelcome to Citrahold." << std::endl;
    std::cout << "Press START at any point to exit.\n\nPlease use the D-pad to navigate, \nand use A to confirm actions.\n\nYou can ignore this console for \nmost intents and purposes.\n\n- - - - - - - - - - - - - - - - -"
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
    std::cout << "\nGetting server status...\nThis should be (fairly) INSTANT!\n"
              << std::endl;
    responsePair serverStatus = networkSystem.init(configManager.getConfig()["serverAddress"], configManager.getToken());
    std::cout << "Reported server status: HTTP " << serverStatus.first << std::endl;

    // at some point, we should check if they're connected to the internet???
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

        case menuFunctions::deleteSavesAfterUpload:
        {

            configManager.setDeleteSaveAfterUpload(!configManager.getDeleteSaveAfterUpload());
            configManager.getDeleteSaveAfterUpload() ? std::cout << "Saves will now be deleted after\nupload" << std::endl : std::cout << "Saves will no longer be deleted\nafter upload" << std::endl;

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
            if (saveSelectionMenuItems.size() == 0)
            {
                std::cout << "No saves found" << std::endl;
                break;
            }
            menuSystem.changeMenu(selection, currentMenuItems, saveSelectionMenuItems, previousMenus, false);
            break;
        }

        case menuFunctions::uploadGame:
        {

            nlohmann::json gameIDJSON = configManager.getGameIDFile(currentUploadType);
            std::filesystem::path gamePath = directoryMenu.getGamePathFromGameID(currentGameID, gameIDJSON);
            std::filesystem::path savePath = gamePath / std::get<0>(saveSelectionMenuItems[selection]);

            std::cout << "Upload started..." << std::endl;

            // make jsonArray called multiUpload
            // nlohmann::json multiUpload;
            // multiUpload["token"] = configManager.getToken();
            // multiUpload["game"] = currentGameID;
            // multiUpload["multi"] = nlohmann::json::array();

            try
            {
                int prevStatus = -1;
                std::filesystem::path gamePath = directoryMenu.getGamePathFromGameID(currentGameID, gameIDJSON);
                std::vector<std::filesystem::path> filesToUpload;

                for (const auto &dirEntry : std::filesystem::recursive_directory_iterator(savePath))
                {
                    // std::cout << std::filesystem::relative(dirEntry, savePath) << std::endl;

                    if (std::filesystem::is_regular_file(dirEntry)) // change to if not 201/200 in future???
                    {
                        std::filesystem::path fullPath = dirEntry.path();
                        filesToUpload.push_back(fullPath);
                    }

                    else if (std::filesystem::is_directory(dirEntry))
                    {
                        std::filesystem::path fullPath = dirEntry.path();
                        std::filesystem::path relativePath = std::filesystem::relative(dirEntry, savePath);

                        // std::cout << "Creating directory " << (currentGameID / relativePath) << std::endl;
                        filesToUpload.push_back((currentGameID / relativePath / "citraholdDirectoryDummy"));

                        // prevStatus = networkSystem.upload(currentUploadType, (currentGameID / relativePath / "citraholdDirectoryDummy"), "citraholdDirectoryDummy");
                        // std::cout << "HTTP " << prevStatus << std::endl;
                    }
                }

                // FOR LOOP THROUGH filesToUpload

                for (unsigned int i = 0; i < filesToUpload.size(); i++)
                {
                    if (prevStatus != -1 && prevStatus != 201 && prevStatus != 200)
                    {
                        std::cout << "Upload failed. HTTP " << prevStatus << std::endl;
                        break;
                    }

                    std::string base64Data = "";
                    std::string filePath = "";

                    if (filesToUpload[i].string().find("citraholdDirectoryDummy") != std::string::npos)
                    {

                        base64Data = "citraholdDirectoryDummy";
                        filePath = filesToUpload[i];

                        std::cout << "[" << (i + 1) << "/" << filesToUpload.size() << "]"
                                  << " UP " << (filePath) << std::endl;
                    }
                    else
                    {
                        std::filesystem::path relativePath = std::filesystem::relative(filesToUpload[i], savePath);

                        std::cout << "[" << (i + 1) << "/" << filesToUpload.size() << "]"
                                  << " UP " << (currentGameID / relativePath) << std::endl;

                        base64Data = networkSystem.getBase64StringFromFile(filesToUpload[i], relativePath);
                        filePath = (currentGameID / relativePath).string();
                    }

                    int newStatus = -1;
                    int attempts = 0;

                    while (newStatus != 201 && newStatus != 200 && attempts <= 3)
                    {
                        newStatus = networkSystem.upload(currentUploadType, filePath, base64Data);

                        if (attempts > 0)
                        {
                            std::cout << "[" << (i + 1) << "/" << filesToUpload.size() << "]"
                                      << " Retrying... (attempt " << attempts << ")" << std::endl;
                        }

                        std::cout << "[" << (i + 1) << "/" << filesToUpload.size() << "]"
                                  << " HTTP " << newStatus << std::endl;

                        attempts++;
                    }

                    prevStatus = newStatus;

                    if ((prevStatus == 201 || prevStatus == 200) && configManager.getDeleteSaveAfterUpload())
                    {
                        try
                        {

                            std::filesystem::remove(filesToUpload[i]);
                        }
                        catch (std::exception &e)
                        {
                            std::cout << e.what() << std::endl;
                        }
                        catch (...)
                        {
                            std::cout << "Unknown error" << std::endl;
                        }
                    }
                }

                std::cout << "All files processed." << std::endl;

                if (prevStatus == 201 || prevStatus == 200)
                {
                    if (configManager.getDeleteSaveAfterUpload())
                    {
                        std::cout << "Deleting save..." << std::endl;
                        std::cout << savePath << std::endl;

                        // Attempt to remove the directory
                        // std::filesystem::remove_all(savePath);

                        // Check if the removal was successful
                        try
                        {
                            if (std::filesystem::exists(savePath))
                            {
                                for (const auto &dirEntry : std::filesystem::recursive_directory_iterator(savePath))
                                {
                                    std::filesystem::path fullPath = dirEntry.path();
                                    std::filesystem::remove(fullPath);
                                }

                                std::filesystem::remove(savePath);
                            }
                        }
                        catch (std::exception &e)
                        {
                            std::cout << e.what() << std::endl;
                        }
                        catch (...)
                        {
                            std::cout << "Unknown error" << std::endl;
                        }
                    }

                    std::cout << "Upload fully successful (complete!)" << std::endl;

                    menuSystem.changeMenu(selection, currentMenuItems, mainMenuItems, previousMenus);
                }
                else if (prevStatus == 0 || prevStatus == -1)
                {
                    if (filesToUpload.size() == 0)
                    {
                        std::cout << "This is an empty directory" << std::endl;
                        if (configManager.getDeleteSaveAfterUpload())
                        {
                            std::cout << "Deleting save..." << std::endl;
                            std::cout << savePath << std::endl;

                            // Attempt to remove the directory
                            // std::filesystem::remove_all(savePath);

                            // Check if the removal was successful
                            try
                            {
                                if (std::filesystem::exists(savePath))
                                {

                                    for (const auto &dirEntry : std::filesystem::recursive_directory_iterator(savePath))
                                    {
                                        std::filesystem::path fullPath = dirEntry.path();

                                        // check if exists first
                                        if (std::filesystem::exists(fullPath))
                                            std::filesystem::remove(fullPath);
                                    }

                                    if (std::filesystem::exists(savePath))
                                        std::filesystem::remove(savePath);
                                }
                            }
                            catch (std::exception &e)
                            {
                                std::cout << e.what() << std::endl;
                            }
                            catch (...)
                            {
                                std::cout << "Unknown error" << std::endl;
                            }
                        }
                    }
                    else
                    {
                        std::cout << "HTTP " << prevStatus << std::endl;
                        std::cout << "Upload failed. Is there anything to upload? (or could network be offline?)" << std::endl;
                    }
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
            if (downloadGameMenuItems.size() == 0)
            {
                std::cout << "No saves found" << std::endl;
                break;
            }

            menuSystem.changeMenu(selection, currentMenuItems, downloadGameMenuItems, previousMenus, false);
            break;
        }

        case menuFunctions::downloadExtdataMenuItems:
        {
            currentUploadType = UploadTypeEnum::EXTDATA;
            downloadGameMenuItems = networkSystem.getGamesMenuItems(UploadTypeEnum::EXTDATA);
            if (downloadGameMenuItems.size() == 0)
            {
                std::cout << "No saves found" << std::endl;
                break;
            }

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
                std::cout << "You need to register this game as\n"
                          << currentGameID << " before you download this save" << std::endl;
                break;
            }

            std::cout << "Downloading " << currentGameID << " to " << gamePath << std::endl;
            bool downloadSuccessful = networkSystem.download(currentUploadType, currentGameID, gamePath);
            std::cout << (downloadSuccessful ? "Download fully successful (complete!)" : "Download failed") << std::endl;

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

    // volume can be used to increase/decrease text size
    u8 volume;
    HIDUSER_GetSoundVolume(&volume);
    if (!dontUseVolumeBar)
    {
        size = 0.6f * (1.0f - volume / 63.0f);
    }
    else
    {
        size = 0.6f * (1.0f - zoom / 63.0f);
    }

    u32 kDown = hidKeysDown();
    u32 kHeld = hidKeysHeld();

    if (kDown & KEY_START)
        halt = true; // break in order to return to hbmenu

    if (kDown & KEY_SELECT)
    {
        dontUseVolumeBar = !dontUseVolumeBar;
        dontUseVolumeBar ? std::cout << "Use left and right shoulder to zoom." << std::endl : std::cout << "Use volume bar to zoom." << std::endl;
    }

    if (kHeld & KEY_L)
    {
        zoom++;
        if (zoom > 63)
        {
            zoom = 63;
        }
    }
    else if (kHeld & KEY_R)
    {
        zoom--;
        if (zoom < 0)
        {
            zoom = 0;
        }
    }

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

    C2D_TextBufClear(*menuSystem.getMenuTextBuf());
    C2D_TextBufClear(selectorBuf);
    C2D_DrawText(menuSystem.getMenuText(), 0, 38.0f, 16.0f, 0.5f, size, size);

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
        ss << ((selection == 0) ? "/" : "") << " Prev Directory    " + (selection == 0 ? "" : " Open " + std::get<0>((renderMenuItems)[selection])) + "\n Cancel     Confirm " + std::string(directoryMenu.getCurrentDirectory());
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
    return trim(mybufAsString);
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