#ifndef NONCE_SHARED_H
#define NONCE_SHARED_H

#include <string>
#include <vector>
#include <unordered_map>

// Shared declarations for both platforms
bool searchBufferForPattern(const char* buffer, size_t size,
                           const std::string& pattern,
                           std::unordered_map<std::string, int>& candidates,
                           std::string& foundNonce);

#endif
