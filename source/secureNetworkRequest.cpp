#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include <3ds.h>
#include <curl/curl.h>
#include <malloc.h>
#include <unistd.h>
#include <stdarg.h>
#include <3ds.h>
#include <exception>
#include <filesystem>
#include "secureNetworkRequest.h"
#include "lets_encrypt_rootca.h"

#define SOC_ALIGN 0x1000
#define SOC_BUFFERSIZE 0x100000
bool socketsOpenWithoutError = true;
static u32 *SOC_buffer = NULL;
s32 sock = -1, csock = -1;

bool DEBUG = false;

struct CallbackData
{
	FILE *fp;
	int is_binary;
	std::string jsonResponse;
};

size_t header_callback(char *buffer, size_t size, size_t nitems, void *userdata)
{
	size_t total_size = size * nitems;
	struct CallbackData *data = (struct CallbackData *)userdata;

	// Check if the Content-Type header is present
	if (strncasecmp(buffer, "Content-Type:", 13) == 0)
	{
		char *content_type = buffer + 13;

		if (DEBUG)
			printf("Content-Type: %s\n", content_type);

		if (std::string(content_type).find("application/json") != std::string::npos)
			data->is_binary = 0;
		else
			data->is_binary = 1;
	}

	return total_size;
}

size_t write_data(void *ptr, size_t size, size_t nmemb, void *userdata)
{
	struct CallbackData *data = (struct CallbackData *)userdata;

	if (!data->is_binary)
	{
		if (DEBUG)
			printf("not writing data\n");

		data->jsonResponse.append(static_cast<char *>(ptr), size * nmemb);

		return nmemb * size;
	}

	data->jsonResponse = "{}";

	printf("Writing data!\n");

	if (data->fp != nullptr)
	{
		size_t written = fwrite(ptr, size, nmemb, data->fp);
		return written;
	}

	return 0;
}

void network_init()
{
	int ret;

	httpcInit(4 * 1024 * 1024);

	SOC_buffer = (u32 *)memalign(SOC_ALIGN, SOC_BUFFERSIZE);

	if (SOC_buffer == NULL)
	{
		socketsOpenWithoutError = false;
		printf("memalign: failed to allocate\n");
	}

	if ((ret = socInit(SOC_buffer, SOC_BUFFERSIZE)) != 0 && socketsOpenWithoutError)
	{
		socketsOpenWithoutError = false;
		printf("socInit: 0x%08X\n", (unsigned int)ret);
	}
}

void network_exit()
{
	socExit();
	httpcExit();
}

responsePair network_request(std::string *address, std::string *jsonData, std::string *downloadPath)
{

	if (!socketsOpenWithoutError)
	{
		if (DEBUG)
			printf("socketsOpenWithoutError is false\n");
		return std::make_pair(0, "");
	}

	CURL *curl;
	CURLcode res;

	if (DEBUG)
		printf("curl init\n");

	curl_global_init(CURL_GLOBAL_DEFAULT);

	curl = curl_easy_init();
	if (curl)
	{
		try
		{

			struct CallbackData data;

			if (downloadPath != nullptr)
			{

				std::filesystem::path path = *downloadPath;
				std::filesystem::path parentPath = path.parent_path();

				if (!std::filesystem::exists(parentPath))
				{
					std::filesystem::create_directories(parentPath);
				}

				data.fp = fopen(downloadPath->c_str(), "wb");
				if (!data.fp)
				{
					if (DEBUG)
						printf("fopen failed\n");
					return std::make_pair(0, "");
				}
			}
			else
			{
				data.fp = nullptr;
			}

			struct curl_blob blob;
			blob.data = (char *)__lets_encrypt_rootca_pem;
			blob.len = sizeof(__lets_encrypt_rootca_pem);
			blob.flags = CURL_BLOB_COPY;
			curl_easy_setopt(curl, CURLOPT_CAINFO_BLOB, &blob);

			curl_easy_setopt(curl, CURLOPT_URL, (*address).c_str());
			curl_easy_setopt(curl, CURLOPT_USERAGENT, "Citrahold 3DS Client (libcurl)/1.0");

			curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 5); // Max time to spend on connection setup

			if (jsonData == nullptr)
			{
				jsonData = new std::string("{}");
			}

			struct curl_slist *headers = NULL;
			headers = curl_slist_append(headers, "Content-Type: application/json");
			curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
			curl_easy_setopt(curl, CURLOPT_POSTFIELDS, jsonData->c_str());

			curl_easy_setopt(curl, CURLOPT_HEADERFUNCTION, header_callback);
			curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
			curl_easy_setopt(curl, CURLOPT_WRITEDATA, &data);
			curl_easy_setopt(curl, CURLOPT_WRITEHEADER, &data);

			if (DEBUG)
				printf("curl_easy_perform\n");

			res = curl_easy_perform(curl);
			int httpStatusCode = 0;

			if (res != CURLE_OK)
			{
				if (DEBUG)
				{
					printf("curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
					printf("\n");
					printf(address->c_str());
				}
			}
			else
			{
				curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpStatusCode);
			}

			if (data.fp != nullptr)
				fclose(data.fp);

			curl_easy_cleanup(curl);
			curl_global_cleanup();

			return std::make_pair(httpStatusCode, data.jsonResponse);
		}
		catch (const std::exception &e)
		{
			printf("Exception: %s\n", e.what());
		}
	}

	return std::make_pair(0, "");
}