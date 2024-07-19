#include "configManager.h"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include "types/uploadTypeEnumType.h"

ConfigManager::ConfigManager()
{
    std::string directoryPath = "/3ds/Citrahold";

    if (!std::filesystem::exists(directoryPath) || !std::filesystem::is_directory(directoryPath))
    {
        if (std::filesystem::create_directories(directoryPath))
        {
            std::cout << "Directory created successfully." << std::endl;
        }
        else
        {
            std::cerr << "Failed to create the directory." << std::endl;
        }
    }

    std::fstream file("/3ds/Citrahold/config.json", std::ios::in | std::ios::out);
    if (!file)
    {
        file.open("/3ds/Citrahold/config.json", std::ios::out);

        nlohmann::json data = {
            {"serverAddress", "https://api.citrahold.com"},
            {"token", ""},
            {"deleteSaveAfterUpload", "false"},
            {"_note", "keep your token private!"}

        };

        file << data.dump();

        config = data;
    }
    else
    {
        file.seekp(0);

        // check all needed keys are present

        bool updateNeeded = false;
        if (!config.contains("serverAddress"))
        {
            config["serverAddress"] = "https://api.citrahold.com";
            updateNeeded = true;
        }
        if (!config.contains("token"))
        {
            config["token"] = "";
            updateNeeded = true;
        }
        if (!config.contains("deleteSaveAfterUpload"))
        {
            config["deleteSaveAfterUpload"] = "false";
            updateNeeded = true;
        }

        std::string fileContents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        config = nlohmann::json::parse(fileContents);

        if (updateNeeded)
        {
            updateConfigFile(config);
        }
        }

    configDirectory = directoryPath;

    // this may not be correct
    gamesDirectory = "/3ds/Checkpoint/";

    getGameIDFile(UploadTypeEnum::SAVES);
    getGameIDFile(UploadTypeEnum::EXTDATA);
}

std::filesystem::path ConfigManager::getConfigDirectory() const
{
    return configDirectory;
}

std::filesystem::path ConfigManager::getGamesDirectory(UploadTypeEnum type) const
{
    return gamesDirectory;
}

void ConfigManager::updateConfigFile(nlohmann::json newConfig)
{
    std::filesystem::path filePath = configDirectory / "config.json";
    std::ofstream file(filePath);

    config = newConfig;

    if (file.is_open())
    {

        std::string jsonString = newConfig.dump();
        file << jsonString;
        file.close();
    }
    else
    {
        std::cerr << "Failed to open the file for writing.";
    }
}

void ConfigManager::setToken(std::string token)
{
    config["token"] = token;
    updateConfigFile(config);
}

nlohmann::json ConfigManager::getGameIDFile(UploadTypeEnum type)
{

    std::filesystem::path filePath = configDirectory / (type == UploadTypeEnum::SAVES ? "gameIDSaves.json" : "gameIDExtdata.json");

    std::fstream file(filePath, std::ios::in | std::ios::out);
    if (!file)
    {
        file.open(filePath, std::ios::out);

        nlohmann::json data;
        data["gameID"] = nlohmann::json::array();
        nlohmann::json jsonDoc(data);
        std::string jsonString = jsonDoc.dump();

        file << jsonString;
        file.close();

        return jsonDoc;
    }
    else
    {
        file.seekp(0);
        std::string fileContents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        file.close();
        return nlohmann::json::parse(fileContents);
    }
}

void ConfigManager::resetBothGameIDFiles()
{
    std::filesystem::path filePath = configDirectory / "gameIDSaves.json";
    std::ofstream file(filePath);
    if (file.is_open())
    {
        nlohmann::json data;
        data["gameID"] = nlohmann::json::array();
        nlohmann::json jsonDoc(data);
        std::string jsonString = jsonDoc.dump();

        file << jsonString;
        file.close();
    }
    else
    {
        std::cerr << "Failed to open the file for writing.";
    }

    filePath = configDirectory / "gameIDExtdata.json";
    file.open(filePath);
    if (file.is_open())
    {
        nlohmann::json data;
        data["gameID"] = nlohmann::json::array();
        nlohmann::json jsonDoc(data);
        std::string jsonString = jsonDoc.dump();

        file << jsonString;
        file.close();
    }
    else
    {
        std::cerr << "Failed to open the file for writing.";
    }

    getGameIDFile(UploadTypeEnum::SAVES);
    getGameIDFile(UploadTypeEnum::EXTDATA);
}

void ConfigManager::updateGameIDFile(UploadTypeEnum type, nlohmann::json newFile)
{

    std::filesystem::path filePath = configDirectory / (type == UploadTypeEnum::SAVES ? "gameIDSaves.json" : "gameIDExtdata.json");

    std::ofstream file(filePath);

    if (file.is_open())
    {

        std::string jsonString = newFile.dump();
        file << jsonString;
        file.close();
    }
    else
    {
        std::cerr << "Failed to open the file for writing.";
    }
}

std::string ConfigManager::getGamePathFromID(UploadTypeEnum type, std::string gameID)
{
    nlohmann::json gameIDFile = getGameIDFile(type);
    for (auto &entry : gameIDFile["gameID"])
    {
        if (entry[0] == gameID)
        {
            return entry[1];
        }
    }
    return "";
}

std::string ConfigManager::getGameIDFromPath(UploadTypeEnum type, std::string gamePath)
{
    nlohmann::json gameIDFile = getGameIDFile(type);
    for (auto &entry : gameIDFile["gameID"])
    {
        if (entry[1] == gamePath)
        {
            return entry[0];
        }
    }
    return "";
}

int ConfigManager::getNumberOfGameIDs(UploadTypeEnum type)
{
    nlohmann::json gameIDFile = getGameIDFile(type);
    return gameIDFile["gameID"].size();
}

void ConfigManager::addGameIDToFile(UploadTypeEnum type, std::string gameID, std::string gamePath)
{
    removeGameIDFromFile(type, gameID);

    nlohmann::json oldGameIDFile = getGameIDFile(type);
    nlohmann::json newEntry = nlohmann::json::array();
    newEntry.push_back(gameID);
    newEntry.push_back(gamePath);
    oldGameIDFile["gameID"].push_back(newEntry);
    updateGameIDFile(type, oldGameIDFile);
}

void ConfigManager::removeGameIDFromFile(UploadTypeEnum type, std::string gameID)
{
    nlohmann::json oldGameIDFile = getGameIDFile(type);
    nlohmann::json newGameIDFile = nlohmann::json::array();
    for (auto &entry : oldGameIDFile["gameID"])
    {
        if (entry[0] != gameID)
        {
            newGameIDFile.push_back(entry);
        }
    }
    oldGameIDFile["gameID"] = newGameIDFile;
    updateGameIDFile(type, oldGameIDFile);
}

void ConfigManager::renameGameIDInFile(UploadTypeEnum type, std::string oldGameID, std::string newGameID)
{
    nlohmann::json oldGameIDFile = getGameIDFile(type);
    for (auto &entry : oldGameIDFile["gameID"])
    {
        if (entry[0] == oldGameID)
        {
            entry[0] = newGameID;
        }
    }
    updateGameIDFile(type, oldGameIDFile);
}

void ConfigManager::redirectGameIDInFile(UploadTypeEnum type, std::string gameID, std::string newPath)
{
    nlohmann::json oldGameIDFile = getGameIDFile(type);
    for (auto &entry : oldGameIDFile["gameID"])
    {
        if (entry[0] == gameID)
        {
            entry[1] = newPath;
        }
    }
    updateGameIDFile(type, oldGameIDFile);
}

nlohmann::json ConfigManager::getConfig() const
{
    return config;
}

std::string ConfigManager::getToken() const
{
    return config["token"];
}

bool ConfigManager::getDeleteSaveAfterUpload() const
{
    if (!config.contains("deleteSaveAfterUpload"))
    {
        return false;
    }

    return config["deleteSaveAfterUpload"].dump() == "true";
}

void ConfigManager::setDeleteSaveAfterUpload(bool deleteSaveAfterUpload)
{
    config["deleteSaveAfterUpload"] = deleteSaveAfterUpload;
    updateConfigFile(config);
}

bool ConfigManager::loggedIn()
{
    return userID != "invalid";
}
