#include "apiParser.h"

#define STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image.h"
#include "stb_image_write.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <curl/curl.h>
#include <vector>
#include <lzma.h>
#include <sstream>
#include <cstdint>
#include <list>
#include <shared_mutex>
#include <unordered_map>
#include <nlohmann/json.hpp>

#include "../EELogParser/logParser.h"
#include "../nonceGetter/nonceFinder.h"
#include "FileAccess/FileAccess.h"

//cache stuff; Max is in the header
std::list<std::string> cache_order;
std::unordered_map<std::string, std::pair<std::shared_ptr<std::vector<uint8_t>>, std::list<std::string>::iterator>> url_cache;
std::shared_mutex cache_mutex;

static size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    size_t realSize = size * nmemb;
    auto* mem = static_cast<std::vector<uint8_t>*>(userp);
    size_t oldSize = mem->size();
    mem->resize(oldSize + realSize);
    std::memcpy(mem->data() + oldSize, contents, realSize);
    return realSize;
}

//checking if the first 8 bytes show png
bool isValidPng(const std::vector<uint8_t>& data) {
    static const uint8_t sig[4] = {0x89, 0x50, 0x4E, 0x47};
    return data.size() > 4 && std::equal(sig, sig+4, data.begin());
}

bool isValidJpg(const std::vector<uint8_t>& data) {
    static const uint8_t sig[2] = {0xFF, 0xD8};
    return data.size() > 2 && std::equal(sig, sig+2, data.begin());
}

//should be a 50x50 red square
std::shared_ptr<const std::vector<uint8_t>> getFallbackDefaultPng() {
    static const std::vector<uint8_t> png_data = {
        0x89, 0x50, 0x4e, 0x47, 0x0d, 0x0a, 0x1a, 0x0a, 0x00, 0x00, 0x00, 0x0d,
        0x49, 0x48, 0x44, 0x52, 0x00, 0x00, 0x00, 0x32, 0x00, 0x00, 0x00, 0x32,
        0x08, 0x02, 0x00, 0x00, 0x00, 0x91, 0x5d, 0x1f, 0xe6, 0x00, 0x00, 0x00,
        0x66, 0x49, 0x44, 0x41, 0x54, 0x78, 0x9c, 0xcd, 0xce, 0x31, 0x01, 0x00,
        0x30, 0x0c, 0x80, 0x30, 0x86, 0x7f, 0xcf, 0x9d, 0x81, 0xfe, 0x25, 0x0a,
        0xf2, 0x86, 0x22, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
        0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
        0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
        0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
        0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
        0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24, 0x49, 0x92, 0x24,
        0x49, 0x92, 0x24, 0xaf, 0x03, 0xbb, 0x0f, 0x41, 0xaa, 0x01, 0x63, 0xdc,
        0xab, 0xbf, 0x07, 0x00, 0x00, 0x00, 0x00, 0x49, 0x45, 0x4e, 0x44, 0xae,
        0x42, 0x60, 0x82,
    };
    static const std::shared_ptr<const std::vector<uint8_t>> fallback_ptr =
        std::make_shared<const std::vector<uint8_t>>(png_data);

    return fallback_ptr;
}

//using alone decompression for .lzma format (Important reminder!!: stream would be .xz)
bool decompressLzmaAlone(
    const uint8_t* input, size_t input_len,
    std::vector<uint8_t>& output
) {
    lzma_stream strm = LZMA_STREAM_INIT;
    if (lzma_alone_decoder(&strm, UINT64_MAX) != LZMA_OK)
        return false;

    strm.next_in = input;
    strm.avail_in = input_len;
    strm.next_out = output.data();
    strm.avail_out = output.size();

    while (true) {
        lzma_ret ret = lzma_code(&strm, LZMA_FINISH);
        if (ret == LZMA_STREAM_END) {
            output.resize(strm.total_out);
            lzma_end(&strm);
            return true;
        }
        if (ret != LZMA_OK) {
            lzma_end(&strm);
            return false;
        }
        // Need more output space
        size_t prev_size = output.size();
        output.resize(prev_size * 2);
        strm.next_out = output.data() + prev_size;
        strm.avail_out = prev_size;
    }
}

std::vector<uint8_t> lzmaDecode(const std::vector<uint8_t>& input, size_t expected_size = 0) {
    LogThis("starting lzma decode");
    if (expected_size == 0) expected_size = 4 * input.size();
    std::vector<uint8_t> output(expected_size);
    if (!decompressLzmaAlone(input.data(), input.size(), output)){
        LogThis("could not decode lzma. returning empty instead");
        return {};
    }
    return output;
}

//TODO: better caching
std::shared_ptr<const std::vector<uint8_t>>  fetchUrlCached(const std::string& url, FetchType fetchType) {
    //shared Read Lock (allows multiple reads); Update: cant do that because LRU
    {
        std::unique_lock read_lock(cache_mutex);
        auto it = url_cache.find(url);
        if (it != url_cache.end()) {
            cache_order.erase(it->second.second);
            cache_order.push_front(url);
            it->second.second = cache_order.begin();
            return it->second.first;
        }
    }

    // Not cached, so fetch
    std::vector<uint8_t> raw_data = fetchUrl(url, fetchType);
    if (raw_data.empty()) {
        return getFallbackDefaultPng();
    }
    auto result = std::make_shared<std::vector<uint8_t>>(std::move(raw_data));

    //Locks when writing
    {
        std::unique_lock write_lock(cache_mutex);
        // If cache limit reached, remove least recently used (at the back)
        if (url_cache.size() >= CACHE_LIMIT) {
            const std::string& lru_key = cache_order.back();
            url_cache.erase(lru_key);
            cache_order.pop_back();
        }

        // Insert new entry at front
        cache_order.push_front(url);
        url_cache[url] = {result, cache_order.begin()};
    }

    return result;
}

std::vector<uint8_t> convertToPngBuffer(const unsigned char* jpg_data, size_t jpg_size) {
    int width, height, channels;
    unsigned char* image_data = stbi_load_from_memory(jpg_data, static_cast<int>(jpg_size), &width, &height, &channels, 4); // force RGBA
    if (!image_data) {
        throw std::runtime_error("Failed to decode JPG image.");
    }

    std::vector<uint8_t> pngBuffer;
    auto writeFunc = [](void* context, void* data, int size) {
        auto* buffer = reinterpret_cast<std::vector<uint8_t>*>(context);
        buffer->insert(buffer->end(), (uint8_t*)data, (uint8_t*)data + size);
    };

    if (!stbi_write_png_to_func(writeFunc, &pngBuffer, width, height, 4, image_data, width * 4)) {
        stbi_image_free(image_data);
        throw std::runtime_error("Failed to encode PNG.");
    }

    stbi_image_free(image_data);
    return pngBuffer;
}

std::vector<uint8_t> fetchUrl(const std::string& url, FetchType fetchType) {
    //if (fetchType == FetchType::PNG) return get_fallback_default_png();

    std::string effective_url = url;
    if (fetchType == FetchType::PNG) effective_url = "https://content.warframe.com/PublicExport" + url;
    //LogThis("Fetching url: " + effective_url);
    CURL* curl = curl_easy_init();
    if (!curl) throw std::runtime_error("curl_easy_init() failed");

    std::vector<uint8_t> readBuffer;
    long httpCode = 0;

    curl_easy_setopt(curl, CURLOPT_URL, effective_url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &readBuffer);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "libcurl/1.0");

    CURLcode res = curl_easy_perform(curl);
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &httpCode);

    if (res != CURLE_OK) {
        curl_easy_cleanup(curl);
        LogThis("curl_easy_perform() failed: " + std::string(curl_easy_strerror(res)) + ", returned empty instead for url: " + effective_url);
        return {};
    }
    if (httpCode != 200) {
        curl_easy_cleanup(curl);
        LogThis("HTTP code: " + std::to_string(httpCode) + ", returned empty instead for url: " + effective_url);
        return {};
    }

    curl_easy_cleanup(curl);

    //LogThis("Successfully fetched url: " + effective_url);
    switch (fetchType) {
        case FetchType::STRING:
            return readBuffer;

        case FetchType::PNG:
            if (isValidPng(readBuffer)) {
                return readBuffer;
            }
            if  (isValidJpg(readBuffer)) {
                auto pngData = convertToPngBuffer(readBuffer.data(), readBuffer.size());
                return pngData; // PNG format now
            }
            LogThis("Image not valid for " + effective_url);
            return {};

        case FetchType::LZMA:
            return lzmaDecode(readBuffer);

        default:
            LogThis("Unsupported fetch type, returned empty instead");
            return {};
    }
}

bool UpdatePlayerData(std::string nonce) {
    //if (nonce.empty()) nonce = findNonceInProcess("Warframe.x64.exe");
    std::string accountId = getLastPlayerId();
    if (nonce.empty()) {
        return false;
    }
    std::string url = "api.warframe.com/api/inventory.php?accountId=" + accountId + "&nonce=" + nonce;
    LogThis("got url: "+  url);
    std::vector<uint8_t> vec = fetchUrl(url, FetchType::STRING);
    std::string result(vec.begin(), vec.end());
    if (result == "Log-in expired") {
        LogThis("auth expired");
        return false;
    }
    SaveDataAt(result, "../data/Player/player_data.json");
    return true;
}

void FetchGameUpdate() {
    LogThis("called FetchGameUpdate");
    std::vector<uint8_t> vec = fetchUrl("https://origin.warframe.com/PublicExport/index_en.txt.lzma", FetchType::LZMA);
    std::string index(vec.begin(), vec.end());
    std::istringstream iss(index);
    std::string line;
    std::string trimmed;
    std::vector<uint8_t> vec2;
    std::string data;
    size_t linenum = 0;
    while (std::getline(iss, line)) {
        ++linenum;
        if (!line.empty() && line.back() == '\r') //Carriage returns are my nemesis
            line.pop_back();
        LogThis("Streamline #" + std::to_string(linenum) + ": " + line);
        trimmed = line.size() > 30 ? line.substr(0, line.size() - 26) : ""; //warframe adds 26 extra characters for version but we just want till .json
        LogThis("trimmed: '" + trimmed + "'");
        vec2 = fetchUrl("https://content.warframe.com/PublicExport/Manifest/" + line, FetchType::STRING);
        LogThis("2nd fetch for " + line + " got size " + std::to_string(vec2.size()));
        data.assign(vec2.begin(), vec2.end());
        if (!data.empty()) {
            LogThis("data for " + line + " starts: '" + data.substr(0, 80) + "'");
        }
        SaveDataAt(data, "../data/Warframe/" + trimmed);
        LogThis("Data Saved");
    }

    LogThis("Done with FetchGameUpdate loop");
}


/*int main() {
    if (!SaveJsonAt("../data/")) {
        std::cout << "failed to save Json" << std::endl;
    }
    else {
        std::cout << "player data successfully saved" << std::endl;
    }
    return 0;
}
*/