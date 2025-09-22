#include "nonceFinder.h"
#include "nonceShared.h"
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <cctype>
#include <cstring>

// Linux-specific implementation
std::vector<pid_t> getPidsByName(const std::string& name) {
    std::vector<pid_t> pids;
    DIR* dir = opendir("/proc");
    if (!dir) return pids;

    struct dirent* entry;
    while ((entry = readdir(dir)) != nullptr) {
        if (entry->d_type != DT_DIR) continue;
        std::string dirname(entry->d_name);
        if (dirname.empty() || !std::all_of(dirname.begin(), dirname.end(), ::isdigit)) continue;

        pid_t pid = std::stoi(dirname);
        std::ifstream cmdline("/proc/" + dirname + "/cmdline");
        std::string processName;
        std::getline(cmdline, processName);

        // Handle null-delimited arguments
        size_t nullPos = processName.find('\0');
        if (nullPos != std::string::npos) {
            processName = processName.substr(0, nullPos);
        }

        size_t pos = processName.find_last_of('/');
        std::string exeName = (pos != std::string::npos) ?
                              processName.substr(pos + 1) :
                              processName;

        if (exeName.find(name) != std::string::npos) {
            pids.push_back(pid);
        }
    }
    closedir(dir);
    return pids;
}

struct MemoryRegion {
    uintptr_t start;
    uintptr_t end;
};

std::vector<MemoryRegion> getMemoryRegions(pid_t pid) {
    std::vector<MemoryRegion> regions;
    std::ifstream maps("/proc/" + std::to_string(pid) + "/maps");
    std::string line;

    while (std::getline(maps, line)) {
        std::istringstream iss(line);
        std::string addrRange, perms;
        if (!(iss >> addrRange >> perms)) continue;

        if (perms.find('r') == std::string::npos) continue;

        size_t sep = addrRange.find('-');
        if (sep == std::string::npos) continue;

        uintptr_t start = std::stoul(addrRange.substr(0, sep), nullptr, 16);
        uintptr_t end = std::stoul(addrRange.substr(sep + 1), nullptr, 16);

        regions.push_back({start, end});
    }
    return regions;
}

std::string readNonce(pid_t pid) {
    const std::string pattern = "nonce=";
    std::unordered_map<std::string, int> candidates;
    std::string foundNonce;

    std::vector<MemoryRegion> regions = getMemoryRegions(pid);
    std::string memPath = "/proc/" + std::to_string(pid) + "/mem";
    int memfd = open(memPath.c_str(), O_RDONLY);
    if (memfd == -1) return "";

    for (const auto& region : regions) {
        size_t size = region.end - region.start;
        std::vector<char> buffer(size);

        if (pread(memfd, buffer.data(), size, region.start) != static_cast<ssize_t>(size))
            continue;

        if (searchBufferForPattern(buffer.data(), size, pattern, candidates, foundNonce)) {
            close(memfd);
            return foundNonce;
        }
    }

    close(memfd);
    return "";
}

std::string findNonceInProcess(const std::string& processName) {
    auto pids = getPidsByName(processName);
    if (pids.empty()) return "";
    return readNonce(pids[0]);
}
