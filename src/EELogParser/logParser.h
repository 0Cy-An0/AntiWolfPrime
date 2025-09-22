// Parser.h
#pragma once

#include <string>

// Returns the last player ID found in the log file.
// If the log file can't be found, it returns an empty string.
std::string getLastPlayerId(bool reload=false);
std::string getLastPlayerName(bool reload=false);
//needed for monitoring because its better to keep thread stuff there alone
std::string findLog();