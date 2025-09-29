#ifndef APIPARSER_H
#define APIPARSER_H

#include <memory>
#include <string>
#include <vector>
#include <curl/curl.h>

enum class FetchType {
    STRING,
    PNG,
    LZMA
};

// Fetches url content
std::vector<uint8_t> fetchUrl(const std::string& effective_url, FetchType fetchType);
std::shared_ptr<const std::vector<uint8_t>>  fetchUrlCached(const std::string& effective_url, FetchType fetchType);

void FetchGameUpdate(); //as self explanatory as the one above ;). saves to data/warframe as .json
bool UpdatePlayerData(std::string extra = "");

//for the url fetch cache
const size_t CACHE_LIMIT = 30;


#endif //APIPARSER_H
