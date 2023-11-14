#include "networkSystem.h"
#include "json.hpp"
#include <3ds.h>
#include <stdio.h>
#include <stdlib.h>
#include <iostream>
#include <filesystem>
#include <fstream>
#include <inttypes.h>
#include "json.hpp"
#include "base64.h"
#include "secureNetworkRequest.h"

NetworkSystem::NetworkSystem() // std::string serverAddress, std::string token
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

	responsePair response = sendRequest(this->serverAddress + (type == UploadTypeEnum::EXTDATA ? "/downloadExtdata" : "/downloadSaves"), &data);
	if (response.first == 200)
	{
		bool successfulSoFar = true;

		nlohmann::json responseJSON = nlohmann::json::parse(response.second);
		for (const auto &element : responseJSON["files"].items())
		{
			if (successfulSoFar)
			{
				std::cout << "Downloading file " << (element.value()) << std::endl;
				responsePair newResponse;
				data["file"] = element.value();
				// http_post(
				// 	(this->serverAddress + (type == UploadTypeEnum::EXTDATA ? "/downloadExtdata" : "/downloadSaves")).c_str(),
				// 	data.dump().c_str(),
				// 	&newResponse,
				// 	(gamePath / "Citrahold-Download") / element.value());

				std::string address = this->serverAddress + (type == UploadTypeEnum::EXTDATA ? "/downloadExtdata" : "/downloadSaves");
				std::string jsonData = data.dump();
				std::string downloadPath = (gamePath / "Citrahold-Download") / element.value();

				newResponse = network_request(&address, &jsonData, &downloadPath);

				std::cout << ((gamePath / "Citrahold-Download") / element.value()).string() << std::endl;
				std::cout << newResponse.first << std::endl;
				if (newResponse.first != 200)
				{
					std::cout << "Failed to download file " << (element.value()) << std::endl;
					std::cout << "HTTP Response: " << newResponse.first << std::endl;
					
					successfulSoFar = false;
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
	// we need to make {token: token}
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
		// we got a token
		// debug output
		// untested
		this->token = fullToken;
		return nlohmann::json::parse(response.second)["userID"];
	}
	else
	{
		std::cout << "Failed to verify token\n";
		return "invalid";
	}
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
