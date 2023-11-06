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
    void updateGameIDFile(UploadTypeEnum type, nlohmann::json newFile);

    nlohmann::json getConfig() const;
    std::string getToken() const;

    std::string userID;
    void resetBothGameIDFiles();

    bool loggedIn();

private:
    std::filesystem::path configDirectory;
    std::filesystem::path gamesDirectory;
    nlohmann::json config;
};

#endif // CONFIGMANAGER_H
