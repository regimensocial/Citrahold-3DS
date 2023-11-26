#ifndef SECURE_NETWORK_REQUEST_H
#define SECURE_NETWORK_REQUEST_H

#include <string>
#include <utility>

using responsePair = std::pair<int, std::string>;

responsePair network_request(std::string *address, std::string *jsonData = nullptr, std::string *downloadPath = nullptr);
bool valid_certificate();
void network_init();
void network_exit();

#endif // SECURE_NETWORK_REQUEST_H