#include "nonceFinder.h"
#include "nonceShared.h"
#include <windows.h>
#include <tlhelp32.h>
#include <vector>
#include <string>
#include <iostream>

#include "FileAccess/FileAccess.h"

// Windows-specific implementation
std::vector<DWORD> getPidsByName(const std::string& name) {
    std::wstring wname(name.begin(), name.end()); // ASCII-safe conversion from std::string to std::wstring
    std::vector<DWORD> pids;
    HANDLE snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE) return pids;

    PROCESSENTRY32W pe;
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (!Process32FirstW(snapshot, &pe)) {
        CloseHandle(snapshot);
        return pids;
    }

    do {
        std::wstring exeName(pe.szExeFile);
        if (exeName.find(wname) != std::wstring::npos) {
            pids.push_back(pe.th32ProcessID);
        }
    } while (Process32NextW(snapshot, &pe));

    CloseHandle(snapshot);
    return pids;
}


std::string readNonce(DWORD pid) {
    const std::string pattern = "nonce=";
    std::unordered_map<std::string, int> candidates;
    std::string foundNonce;

    HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!hProcess) return "";

    SYSTEM_INFO sysInfo;
    GetSystemInfo(&sysInfo);

    MEMORY_BASIC_INFORMATION mbi;
    BYTE* addr = reinterpret_cast<BYTE*>(sysInfo.lpMinimumApplicationAddress);
    const BYTE* maxAddr = reinterpret_cast<BYTE*>(sysInfo.lpMaximumApplicationAddress);

    size_t totalRegionsScanned = 0;
    size_t totalBytesRead = 0;

    while (addr < maxAddr) {
        if (!VirtualQueryEx(hProcess, addr, &mbi, sizeof(mbi))) {
            addr += sysInfo.dwPageSize;
            continue;
        }
        ++totalRegionsScanned;
        if (totalRegionsScanned > maxScanRegion) {
            CloseHandle(hProcess);
            LogThis("early abort due to scanning " + std::to_string(totalRegionsScanned) + " Regions");
            //TODO: add something for the options when it didnt find it in within max
            return "";
        }
        if (mbi.State == MEM_COMMIT &&
            (mbi.Protect == PAGE_READONLY ||
             mbi.Protect == PAGE_READWRITE ||
             mbi.Protect == PAGE_EXECUTE_READ ||
             mbi.Protect == PAGE_EXECUTE_READWRITE)) {

            std::vector<BYTE> buffer(mbi.RegionSize);
            SIZE_T bytesRead;
            if (ReadProcessMemory(hProcess, mbi.BaseAddress, buffer.data(), mbi.RegionSize, &bytesRead)) {
                totalBytesRead += bytesRead;
                if (searchBufferForPattern(reinterpret_cast<const char*>(buffer.data()),
                                          bytesRead, pattern, candidates, foundNonce)) {
                    LogThis("found nonce: Regions scanned: " + std::to_string(totalRegionsScanned) + ", bytes read: " + std::to_string(totalBytesRead));
                    CloseHandle(hProcess);
                    return foundNonce;
                }
            }
        }
        addr += mbi.RegionSize;
    }

    CloseHandle(hProcess);
    return "";
}

std::string findNonceInProcess(const std::string& processName) {
    auto pids = getPidsByName(processName);
    if (pids.empty()) return "";
    return readNonce(pids[0]);
}
