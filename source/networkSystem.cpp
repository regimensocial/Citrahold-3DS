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

	sendRequest(this->serverAddress + "/areyouawake");
}

responsePair NetworkSystem::init(std::string serverAddress, std::string token)
{
	this->serverAddress = serverAddress;
	this->token = token;

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

bool NetworkSystem::download(UploadTypeEnum type, std::string gameID, std::filesystem::path gamePath)
{
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
				if (filename.find("citraholdDirectoryDummy") == std::string::npos)
				{
					std::cout << "[" << itemNumber << "/" << numberOfItems << "] "
							  << "Writing " << filename << "\n";
					file.write((char *)base64DataDecoded.data(), base64DataDecoded.size());
					file.close();
				}
				else
				{
					std::cout << "[" << itemNumber << "/" << numberOfItems << "] "
							  << "Ignoring dummy file " << filename << "\n";
				}
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
	responsePair response = sendRequest(this->serverAddress + (uploadType == UploadTypeEnum::SAVES ? "/uploadMultiSaves" : "/uploadMultiExtdata"), &jsonObject);
	return response.first;
}

int NetworkSystem::upload(UploadTypeEnum uploadType, std::string filePath, std::string base64Data)
{
	nlohmann::json data;

	data["data"] = base64Data;
	data["filename"] = filePath;
	data["token"] = this->token;

	// this will return a full token
	responsePair response = sendRequest(this->serverAddress + (uploadType == UploadTypeEnum::SAVES ? "/uploadSaves" : "/UploadExtdata"), &data);

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
