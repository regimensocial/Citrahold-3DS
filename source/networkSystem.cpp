#include "networkSystem.h"
#include "json.hpp"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <inttypes.h>
#include <filesystem>
#include "json.hpp"
#include "base64.h"
#include "secureNetworkRequest.h"
#include "helpers.h"

NetworkSystem::NetworkSystem()
{

	network_init();

}

responsePair NetworkSystem::init(std::string serverAddress, std::string token)
{
	this->serverAddress = serverAddress;

	if (!valid_certificate())
	{
		printf("\nWARNING: The TLS certificate is going to be/is expired (soon). Please update your build!\nWe will not verify the certificate, this could be insecure!\n");
	}

	return sendRequest(this->serverAddress + "/areyouawake");
}

std::string NetworkSystem::getBase64StringFromFile(std::string fullFilePath, std::string filename)
{

	std::ifstream file(fullFilePath, std::ios::binary);
	if (!file)
	{
		std::cout << "Failed to open file (" + fullFilePath + ")\n";
	}
	else
	{
		file.seekg(0, std::ios::end);
		size_t size = file.tellg();
		file.seekg(0);

		std::vector<uint8_t> buffer(size);
		file.read((char *)buffer.data(), size);

		file.close();

		return base64_encode(&buffer[0], buffer.size());
	}
	return "";
}

void NetworkSystem::checkVersion(std::string currentVersion)
{
	std::cout << "\nChecking for updates...";

	responsePair response = sendRequest(this->serverAddress + ("/softwareVersion"));
	nlohmann::json responseJSON = nlohmann::json::parse(response.second);
	nlohmann::json::array_t responseArray = responseJSON["3ds"];

	bool found = false;
	for (const auto &element : responseArray)
	{
		if (element == currentVersion)
		{
			found = true;
			break;
		}
	}

	std::cout << "\r                                   \r";
    if (found) {
        std::cout << "You are running the latest version!\n";
    } else {
        std::cout << "There is an update available ("
                  << responseJSON["3ds"][0].dump() << ")\n";
        std::cout << "Please visit the website for help.\n";
    }

    if (responseJSON.contains("motd3ds")) {
		for (const auto &element : responseJSON["motd3ds"]) {
			std::cout << element.get<std::string>() << "\n";
		}
	}

	if (responseJSON.contains("motd")) {
		for (const auto &element : responseJSON["motd"]) {
			std::cout << element.get<std::string>() << "\n";
		}
	}
}

bool NetworkSystem::download(UploadTypeEnum type, std::string gameID, std::filesystem::path gamePath)
{
	if (!loggedIn)
	{
		std::cout << "Not logged in\n";
		return 0;
	}

	nlohmann::json data;
	data["token"] = this->token;
	data["game"] = gameID;

	safeDirectoryRemove(gamePath / "Citrahold-Download");

	responsePair response = sendRequest(this->serverAddress + (type == UploadTypeEnum::EXTDATA ? "/downloadMultiExtdata" : "/downloadMultiSaves"), &data);
	if (response.first == 200)
	{
		bool successfulSoFar = true;

		nlohmann::json responseJSON = nlohmann::json::parse(response.second);
		int numberOfItems = responseJSON["files"].size();
		int itemNumber = 0;

		std::cout << "Retrieved " << numberOfItems << " files\n";

		for (const auto &element : responseJSON["files"].items())
		{
			if (!successfulSoFar)
			{
				break;
			}

			// element [0] = filename
			// element [1] = base64 data
			itemNumber++;
			std::string filename = element.value()[0];
			std::string base64Data = element.value()[1];

			std::filesystem::path downloadPath = (gamePath / "Citrahold-Download" / filename).string();
			std::vector<uint8_t> base64DataDecoded = base64_decode(base64Data);

			// write the file
			std::filesystem::path parentPath = downloadPath.parent_path();

			if (!std::filesystem::exists(parentPath))
			{
				std::filesystem::create_directories(parentPath);
			}

			if (filename.find("citraholdDirectoryDummy") == std::string::npos)
			{

				std::ofstream file(downloadPath, std::ios::binary);

				if (!file)
				{
					std::cout << "[" << itemNumber << "/" << numberOfItems << "] "
							  << "Failed to open file (" << downloadPath << ")\n";
					successfulSoFar = false;
				}
				else
				{
					// check if filename contains "citraholdDirectoryDummy"

					std::cout << "[" << itemNumber << "/" << numberOfItems << "] "
							  << "Writing " << filename << "\n";
					file.write((char *)base64DataDecoded.data(), base64DataDecoded.size());
					file.close();
				}
			}
			else
			{
				std::cout << "[" << itemNumber << "/" << numberOfItems << "] "
						  << "Ignoring dummy file " << filename << "\n";
			}
		}

		return successfulSoFar;
	}
	return false;
}

menuItems NetworkSystem::getGamesMenuItems(UploadTypeEnum type)
{

	nlohmann::json data;
	data["token"] = this->token;

	responsePair response = sendRequest(this->serverAddress + (type == UploadTypeEnum::EXTDATA ? "/getExtdata" : "/getSaves"), &data);
	if (response.first == 200)
	{

		nlohmann::json responseJSON = nlohmann::json::parse(response.second);
		menuItems gamesMenuItems = {};
		for (const auto &element : responseJSON["games"].items())
		{
			gamesMenuItems.push_back(
				{element.value(), menuFunctions::downloadGame});
		}

		return gamesMenuItems;
	}

	return {};
}

std::string NetworkSystem::getTokenFromShorthandToken(std::string shorthandToken)
{
	nlohmann::json data;

	data["shorthandToken"] = shorthandToken;

	// this will return a full token
	responsePair responseAsString = sendRequest(this->serverAddress + "/getToken", &data);
	if (responseAsString.first == 200)
	{
		nlohmann::json response = nlohmann::json::parse(responseAsString.second);
		this->token = response["token"];
		return response["token"];
	}
	else
	{
		std::cout << "Failed to get token from shorthand token\n";
		return "invalid";
	}
}

std::string NetworkSystem::verifyTokenToSetUserID(std::string fullToken)
{

	nlohmann::json data;

	data["token"] = fullToken;

	responsePair response = sendRequest(this->serverAddress + "/getUserID", &data);
	if (response.first == 200)
	{
		this->token = fullToken;
		return nlohmann::json::parse(response.second)["userID"];
	}
	else
	{
		std::cout << "Failed to verify token\n";
		return "invalid";
	}
}

int NetworkSystem::uploadMultiple(UploadTypeEnum uploadType, nlohmann::json jsonObject)
{
	if (!loggedIn)
	{
		std::cout << "Not logged in\n";
		return 0;
	}

	responsePair response = sendRequest(this->serverAddress + (uploadType == UploadTypeEnum::SAVES ? "/uploadMultiSaves" : "/uploadMultiExtdata"), &jsonObject);
	return response.first;
}

void NetworkSystem::setTokenFromString(std::string token)
{
	this->token = token;
}

responsePair NetworkSystem::sendRequest(std::string address, nlohmann::json *dataToSend)
{

	responsePair response;

	std::string data = "";
	if (dataToSend != nullptr)
	{
		data = dataToSend->dump();
	}

	return network_request(&address, &data);
}

void NetworkSystem::cleanExit()
{
	network_exit();
}
