#ifndef SECURE_NETWORK_REQUEST_H
#define SECURE_NETWORK_REQUEST_H

#include <string>
#include <utility>

using responsePair = std::pair<int, std::string>;

responsePair network_request(std::string *address, std::string *jsonData = nullptr, std::string *downloadPath = nullptr);
void network_init();
void network_exit();
std::string url_encode(const std::string& decoded);
std::string url_decode(const std::string& encoded);

#endif // SECURE_NETWORK_REQUEST_H