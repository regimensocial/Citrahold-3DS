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

NetworkSystem::NetworkSystem() // std::string serverAddress, std::string token
{

	httpcInit(4 * 1024 * 1024);

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
				std::cout << (element.value()) << std::endl;
				responsePair newResponse;
				data["file"] = element.value();
				http_post(
					(this->serverAddress + (type == UploadTypeEnum::EXTDATA ? "/downloadExtdata" : "/downloadSaves")).c_str(),
					data.dump().c_str(),
					&newResponse,
					(gamePath / "Citrahold-Download") / element.value());

				std::cout << ((gamePath / "Citrahold-Download") / element.value()).string() << std::endl;
				std::cout << newResponse.first << std::endl;
				if (newResponse.first != 200)
				{
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

Result NetworkSystem::http_post(const char *url, const char *data, responsePair *response, std::filesystem::path downloadPath)
{
	Result ret = 0;
	httpcContext context;
	char *newurl = NULL;
	u32 statuscode = 0;
	u32 contentsize = 0, readsize = 0, size = 0;
	u8 *buf, *lastbuf;
	bool DEBUG = false;

	if (response != nullptr)
	{
		// by default, set the response to 408 because that's what we'll return if we don't get a response

		*response = responsePair(408, "");
	}

	if (DEBUG)
		printf("POSTing %s\n", url);

	do
	{
		ret = httpcOpenContext(&context, HTTPC_METHOD_POST, url, 0);

		if (DEBUG)
			printf("return from httpcOpenContext: %" PRIx32 "\n", ret);

		// This disables SSL cert verification, so https:// will be usable
		ret = httpcSetSSLOpt(&context, SSLCOPT_DisableVerify);

		if (DEBUG)
			printf("return from httpcSetSSLOpt: %" PRIx32 "\n", ret);

		// Enable Keep-Alive connections
		ret = httpcSetKeepAlive(&context, HTTPC_KEEPALIVE_ENABLED);

		if (DEBUG)
			printf("return from httpcSetKeepAlive: %" PRIx32 "\n", ret);

		// Set a User-Agent header so websites can identify your application
		ret = httpcAddRequestHeaderField(&context, "User-Agent", "httpc-example/1.0.0");

		if (DEBUG)
			printf("return from httpcAddRequestHeaderField: %" PRIx32 "\n", ret);

		// Set a Content-Type header so websites can identify the format of our raw body data.
		// If you want to send form data in your request, use:
		// ret = httpcAddRequestHeaderField(&context, "Content-Type", "multipart/form-data");
		// If you want to send raw JSON data in your request, use:
		ret = httpcAddRequestHeaderField(&context, "Content-Type", "application/json");

		if (DEBUG)
			printf("return from httpcAddRequestHeaderField: %" PRIx32 "\n", ret);

		// Post specified data.
		// If you want to add a form field to your request, use:
		// ret = httpcAddPostDataAscii(&context, "data", value);
		// If you want to add a form field containing binary data to your request, use:
		// ret = httpcAddPostDataBinary(&context, "field name", yourBinaryData, length);
		// If you want to add raw data to your request, use:
		ret = httpcAddPostDataRaw(&context, (u32 *)data, strlen(data));

		if (DEBUG)
			printf("return from httpcAddPostDataRaw: %" PRIx32 "\n", ret);

		ret = httpcBeginRequest(&context);
		if (ret != 0)
		{
			httpcCloseContext(&context);
			if (newurl != NULL)
				free(newurl);
			return ret;
		}

		ret = httpcGetResponseStatusCode(&context, &statuscode);
		if (ret != 0)
		{
			httpcCloseContext(&context);
			if (newurl != NULL)
				free(newurl);
			return ret;
		}

		if ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308))
		{
			if (newurl == NULL)
				newurl = static_cast<char *>(malloc(0x1000));
			if (newurl == NULL)
			{
				httpcCloseContext(&context);
				return -1;
			}
			ret = httpcGetResponseHeader(&context, "Location", newurl, 0x1000);
			url = newurl; // Change pointer to the url that we just learned

			if (DEBUG)
				printf("redirecting to url: %s\n", url);

			httpcCloseContext(&context); // Close this context before we try the next
		}
	} while ((statuscode >= 301 && statuscode <= 303) || (statuscode >= 307 && statuscode <= 308));

	// if (statuscode != 200)
	// {

	//     if (DEBUG)
	//         printf("URL returned status: %" PRIx32 "\n", statuscode);

	//     httpcCloseContext(&context);
	//     if (newurl != NULL)
	//         free(newurl);
	//     return -2;
	// }

	// ^^^ this is commented because we want 400 errors to be handled

	// This relies on an optional Content-Length header and may be 0
	ret = httpcGetDownloadSizeState(&context, NULL, &contentsize);
	if (ret != 0)
	{
		httpcCloseContext(&context);
		if (newurl != NULL)
			free(newurl);
		return ret;
	}

	if (DEBUG)
		printf("reported size: %" PRIx32 "\n", contentsize);

	// Start with a single page buffer
	buf = (u8 *)malloc(0x1000);
	if (buf == NULL)
	{
		httpcCloseContext(&context);
		if (newurl != NULL)
			free(newurl);
		return -1;
	}

	do
	{
		// This download loop resizes the buffer as data is read.
		ret = httpcDownloadData(&context, buf + size, 0x1000, &readsize);
		size += readsize;
		if (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING)
		{
			lastbuf = buf; // Save the old pointer, in case realloc() fails.
			buf = static_cast<u8 *>(realloc(buf, size + 0x1000));
			if (buf == NULL)
			{
				httpcCloseContext(&context);
				free(lastbuf);
				if (newurl != NULL)
					free(newurl);
				return -1;
			}
		}
	} while (ret == (s32)HTTPC_RESULTCODE_DOWNLOADPENDING);

	if (ret != 0)
	{
		httpcCloseContext(&context);
		if (newurl != NULL)
			free(newurl);
		free(buf);
		return -1;
	}

	// Resize the buffer back down to our actual final size
	lastbuf = buf;
	buf = static_cast<u8 *>(realloc(buf, size));
	if (buf == NULL)
	{ // realloc() failed.
		httpcCloseContext(&context);
		free(lastbuf);
		if (newurl != NULL)
			free(newurl);
		return -1;
	}

	if (DEBUG)
		printf("response size: %" PRIx32 "\n", size);

	// check if buffer exists
	if (downloadPath != "")
	{
		// write the buffer to a file

		std::filesystem::path parentPath = std::filesystem::path(downloadPath).parent_path();

		if (!std::filesystem::exists(parentPath))
		{
			std::filesystem::create_directories(parentPath);
		}

		std::ofstream file(downloadPath, std::ios::binary | std::ios::out);

		if (!file)
		{
			std::cout << "Failed to open or create file (" + downloadPath.string() + ")\n";
		}
		else
		{
			file.write((char *)buf, size);
			file.close();
		}

		std::pair<int, std::string> responsePair(statuscode, "");
		*response = responsePair;
	}

	// Print result

	// this doesn't work properly, buffer size is iffy
	// causes weird symbols to appear in the console

	// printf((char *)buf);
	// printf("\n");

	if (response != nullptr && downloadPath == "")
	{

		// printf("%.*s\n", (int)size, buf);

		// response->assign((char *)buf);

		std::string result(reinterpret_cast<char *>(buf), size);

		std::pair<int, std::string> responsePair(statuscode, result);

		*response = responsePair;
	}

	// if (response != nullptr)
	// {
	//     size_t dataSize = size;
	//     while (dataSize > 0 && buf[dataSize - 1] == '\n')
	//     {
	//         dataSize--;
	//     }
	//     response->assign(reinterpret_cast<const char *>(buf), dataSize);
	// }

	gfxFlushBuffers();
	gfxSwapBuffers();

	httpcCloseContext(&context);
	free(buf);
	if (newurl != NULL)
		free(newurl);

	return 0;
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
		// we got a token
		// debug output
		nlohmann::json response = nlohmann::json::parse(responseAsString.second);

		this->token = response["token"];
		return response["token"];
	}
	else
	{
		// we didn't get a token
		// TODO: handle this
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
		// we didn't get a token
		// TODO: handle this
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

	this->http_post(address.c_str(), data.c_str(), &response);
	return response;
}

void NetworkSystem::cleanExit()
{
	httpcExit();
}
