#ifndef NONCE_FINDER_H
#define NONCE_FINDER_H

#include <string>

// Platform-specific function declaration
std::string findNonceInProcess(const std::string& processName);
static int maxScanRegion = 5000;

#endif
