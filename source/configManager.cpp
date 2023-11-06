#include "configManager.h"
#include "json.hpp"
#include <fstream>
#include <iostream>
#include <filesystem>
#include "types/uploadTypeEnumType.h"

ConfigManager::ConfigManager()
{
    std::string directoryPath = "/3ds/Citrahold";

    // Check if the directory exists, and create it if it doesn't
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
            {"serverAddress", "http://192.168.1.152:3000"},
            {"token", ""},
            {"_note", "keep your token private!"}};

        file << data.dump();

        config = data;
    }
    else
    {
        file.seekp(0);

        std::string fileContents((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
        config = nlohmann::json::parse(fileContents);
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
    // TO BE IMPLEMENTED

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
    // clear both gameID files
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
    // TO BE IMPLEMENTED

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

nlohmann::json ConfigManager::getConfig() const
{
    return config;
}

std::string ConfigManager::getToken() const
{
    return config["token"];
}

bool ConfigManager::loggedIn()
{
    return userID != "invalid";
}
