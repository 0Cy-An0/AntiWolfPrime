#include "nonceShared.h"
#include <cctype>
#include <cstring>
#include <iostream>

// Shared pattern search implementation
bool searchBufferForPattern(const char* buffer, size_t size, 
                           const std::string& pattern,
                           std::unordered_map<std::string, int>& candidates,
                           std::string& foundNonce) {
    const size_t patternLen = pattern.size();
    for (size_t i = 0; i < size - patternLen; ++i) {
        if (std::memcmp(buffer + i, pattern.c_str(), patternLen) == 0) {
            size_t cur = i + patternLen;
            std::string nonce;
            
            while (cur < size && std::isdigit(static_cast<unsigned char>(buffer[cur]))) {
                nonce.push_back(buffer[cur]);
                ++cur;
            }
            
            if (!nonce.empty()) {
                std::cout << ".";
                auto it = candidates.find(nonce);
                if (it != candidates.end()) {
                    if (++it->second == 3) {
                        foundNonce = nonce;
                        return true;
                    }
                } else {
                    candidates[nonce] = 1;
                }
            }
        }
    }
    return false;
}
