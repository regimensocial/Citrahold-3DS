#ifndef NETWORKSYSTEM_H
#define NETWORKSYSTEM_H

#include <iostream>
#include <string>
#include <types/uploadTypeEnumType.h>
#include <types/responsePairType.h>
#include "json.hpp"
#include <3ds.h>
#include <types/menuItemsType.h>

class NetworkSystem
{
public:
    NetworkSystem(); //

    std::string getTokenFromShorthandToken(std::string shorthandToken);
    std::string verifyTokenToSetUserID(std::string fullToken);

    int upload(UploadTypeEnum uploadType, std::string filePath, std::string base64Data);

    void cleanExit();

    responsePair init(std::string serverAddress, std::string token);
    menuItems getGamesMenuItems(UploadTypeEnum type);
    std::string getBase64StringFromFile(std::string fullFilePath, std::string filename);
    bool download(UploadTypeEnum type, std::string gameID, std::filesystem::path gamePath);
    

private:
    std::string serverAddress;
    std::string token;

    responsePair sendRequest(std::string address, nlohmann::json *dataToSend = nullptr); //

    void setTokenFromString(std::string token);
};

#endif // NETWORKSYSTEM_H