#include "logParser.h"
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>

namespace fs = std::filesystem;

const std::string LogInIdLine = "Sys [Info]: Logged in";
static std::string PlayerId = "";
static std::string PlayerName = "";

std::string findLog()
{
    std::vector<fs::path> candidates;

#ifdef _WIN32
    // On Windows: %LOCALAPPDATA%\Warframe\EE.log
    if (auto* local = std::getenv("LOCALAPPDATA")) {
        candidates.emplace_back(fs::path(local) / "Warframe" / "EE.log");
    }
#else
    // On Linux under Proton:
    // ~/.local/share/Steam/steamapps/compatdata/230410/pfx/drive_c/...
    // (and for Flatpak users, adjust the base path accordingly)
    if (auto* home = std::getenv("HOME")) {
        fs::path steamPrefix = fs::path(home)
            / ".local/share/Steam/steamapps/compatdata/230410/pfx"
            / "drive_c/users/steamuser/Local Settings/Application Data/Warframe/EE.log";
        candidates.push_back(steamPrefix);
        // if you want to support Flatpak:
        fs::path flatpakPrefix = fs::path(home)
            / ".var/app/com.valvesoftware.Steam/data/Steam/steamapps/compatdata/230410/pfx"
            / "drive_c/users/steamuser/Local Settings/Application Data/Warframe/EE.log";
        candidates.push_back(flatpakPrefix);
    }
#endif

    // Return the first one that actually exists:
    for (auto& p : candidates) {
        if (fs::exists(p))
            return p.string();
    }

    // Fallback: return an empty string
    return {};
}

std::string ParseBetween(const std::string& EELogPath, const std::string& identifier, const std::string& startDelimiter, const std::string& endDelimiter) {
    std::ifstream file(EELogPath);
    if (!file.is_open()) {
        std::cerr << "Failed to open the file.\n";
        return {};
    }
    std::string line;
    while (std::getline(file, line)) {
        if (line.find(identifier) != std::string::npos) {
            size_t start = line.find(startDelimiter);
            if (start == std::string::npos) continue;
            size_t end = line.find(endDelimiter, start + startDelimiter.size());
            if (end == std::string::npos) continue;

            size_t substrStart = start + startDelimiter.size();
            if (end > substrStart) {
                return line.substr(substrStart, end - substrStart);
            }
        }
    }
    return {};
}

std::string getLastPlayerId(bool reload) {
    if (reload && !PlayerId.empty()) {
        return PlayerId;
    }

    auto logPath = findLog();
    if (logPath.empty()) {
        std::cerr << "Couldn’t find EE.log!\n";
        return {};
    }
    PlayerId = ParseBetween(logPath, LogInIdLine, "(", ")");
    return PlayerId;
}

std::string getLastPlayerName(bool reload) {
    if (reload && !PlayerName.empty()) {
        return PlayerName;
    }
    auto logPath = findLog();
    if (logPath.empty()) {
        std::cerr << "Couldn’t find EE.log!\n";
        return {};
    }
    PlayerName = ParseBetween(logPath, LogInIdLine, "in ", " (");
    return PlayerName;
}