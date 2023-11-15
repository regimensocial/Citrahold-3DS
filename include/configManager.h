#ifndef CONFIGMANAGER_H
#define CONFIGMANAGER_H

#include <filesystem>
#include <string>
#include "types/uploadTypeEnumType.h"
#include "json.hpp"

class ConfigManager  {
public:
    ConfigManager();

    std::filesystem::path getConfigDirectory() const;
    std::filesystem::path getGamesDirectory(UploadTypeEnum type = UploadTypeEnum::SAVES) const;

    void updateConfigFile(nlohmann::json newConfig);
    void setToken(std::string token);

    nlohmann::json getGameIDFile(UploadTypeEnum type);

    // consider addGameIDToFile

    std::string getGamePathFromID(UploadTypeEnum type, std::string gameID);
    std::string getGameIDFromPath(UploadTypeEnum type, std::string gamePath);
    int getNumberOfGameIDs(UploadTypeEnum type);

    void addGameIDToFile(UploadTypeEnum type, std::string gameID, std::string gamePath);
    void removeGameIDFromFile(UploadTypeEnum type, std::string gameID);
    void renameGameIDInFile(UploadTypeEnum type, std::string oldGameID, std::string newGameID);
    void redirectGameIDInFile(UploadTypeEnum type, std::string gameID, std::string newPath);

    nlohmann::json getConfig() const;
    std::string getToken() const;
    bool getDeleteSaveAfterUpload() const;
    void setDeleteSaveAfterUpload(bool deleteSaveAfterUpload);

    std::string userID;
    void resetBothGameIDFiles();

    bool loggedIn();

private:
    void updateGameIDFile(UploadTypeEnum type, nlohmann::json newFile);

    std::filesystem::path configDirectory;
    std::filesystem::path gamesDirectory;
    nlohmann::json config;
};

#endif // CONFIGMANAGER_H
