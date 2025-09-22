#include "FileAccess.h"

#include <fstream>
#include <ios>
#include <iosfwd>
#include <mutex>
#include <nlohmann/json.hpp>

#include "apiParser/apiParser.h"

bool SaveDataAt(const std::string& data, const std::string& path, const std::ios_base::openmode mode) {
    std::ofstream outfile(path, mode);
    if (!outfile) {
        return false;
    }
    outfile << data;
    outfile.close();
    return true;
}

//locking LogThis() to make sure it can be called from background thread safely
std::mutex logMutex;

void LogThis(const std::string& str) {
    std::lock_guard lock(logMutex);
    SaveDataAt(str + "\n", logPath, std::ios::out | std::ios::app);
}

std::string ReadFile(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) {
        LogThis("Could not open file " + path);
        return {};
    }
    std::string content( (std::istreambuf_iterator(file)),
                         std::istreambuf_iterator<char>() );
    return content;
}

// Converts std::string to shared_ptr<const std::vector<uint8_t>>
std::shared_ptr<const std::vector<uint8_t>> MakeByteVector(const std::string& data) {
    return std::make_shared<const std::vector<uint8_t>>(data.begin(), data.end());
}

// Helper to sanitize filename (remove everything after '!' if present)
std::string SanitizeFilename(const std::string& image) {
    size_t excl = image.find('!');
    std::string result = (excl != std::string::npos) ? image.substr(0, excl) : image;

    std::string replacement = "_"; //because we cant save images like this
    size_t pos = 0;
    while ((pos = result.find('/', pos)) != std::string::npos) {
        result.replace(pos, 1, replacement);
        pos += replacement.length();
    }

    return result;
}


std::shared_ptr<const std::vector<uint8_t>> GetImageByFileOrDownload(const std::string& image) {
    std::string sanitized = SanitizeFilename(image);
    std::string fullPath = downloadPath + sanitized;

    // Try loading from disk
    std::string fileData = ReadFile(fullPath);
    if (!fileData.empty()) {
        return MakeByteVector(fileData);
    }

    // File not found locally, attempt download
    auto downloaded = fetchUrlCached(image, FetchType::PNG);
    if (downloaded && !downloaded->empty()) {
        // Save downloaded data to disk
        std::string dataStr(downloaded->begin(), downloaded->end());
        if (!SaveDataAt(dataStr, fullPath, std::ios::binary)) {
            LogThis("Failed to save downloaded image to " + fullPath);
        }
    }

    return downloaded;
}

Settings LoadSettings() {
    Settings settings{};
    std::ifstream file(settingPath);
    if (!file.is_open()) {
        // Create parent directories if needed
        std::filesystem::create_directories(std::filesystem::path(settingPath).parent_path());
        // Create the file with defaults
        WriteSettings(settings);

        // Now reopen to check if it worked
        file.open(settingPath);
        if (!file.is_open()) {
            LogThis("Could not create or open settings file: " + settingPath);
            return settings;  // Return defaults
        }
    }

    std::string line;
    while (std::getline(file, line)) {
        // Remove comments
        size_t commentPos = line.find('#');
        if (commentPos != std::string::npos)
            line = line.substr(0, commentPos);

        if (line.empty())
            continue;

        size_t eqPos = line.find('=');
        if (eqPos == std::string::npos)
            continue;

        std::string key = line.substr(0, eqPos);
        std::string value = line.substr(eqPos + 1);

        // Trim whitespace
        auto trim = [](std::string& s) {
            size_t start = s.find_first_not_of(" \t");
            size_t end   = s.find_last_not_of(" \t");
            if (start == std::string::npos) { s.clear(); return; }
            s = s.substr(start, end - start + 1);
        };
        trim(key);
        trim(value);

        // Parse known settings
        if (key == "fullItems") settings.fullItems = (value == "true" || value == "1");
        else if (key == "hideFounder") settings.hideFounder = (value == "true" || value == "1");
        else if (key == "saveDownload") settings.saveDownload = (value == "true" || value == "1");
        else if (key == "authTokenGetterRead") settings.authTokenGetterRead = (value == "true" || value == "1");
        else if (key == "startWithSystem") settings.startWithSystem = (value == "true" || value == "1");
        else if (key == "autoSync") settings.autoSync = (value == "true" || value == "1");
        else if (key == "syncTime") {
            try { settings.syncTime = std::stoi(value); }
            catch (...) { LogThis("Invalid integer for syncTime in settings"); }
        }
        else if (key == "syncOnMissionFinish") settings.syncOnMissionFinish = (value == "true" || value == "1");
    }

    return settings;
}

void WriteSettings(const Settings& settings) {
    std::string data;
    data += "fullItems=" + std::string(settings.fullItems ? "true" : "false") + "\n";
    data += "hideFounder=" + std::string(settings.hideFounder ? "true" : "false") + "\n";
    data += "saveDownload=" + std::string(settings.saveDownload ? "true" : "false") + "\n";
    data += "authTokenGetterRead=" + std::string(settings.authTokenGetterRead ? "true" : "false") + "\n";
    data += "startWithSystem=" + std::string(settings.startWithSystem ? "true" : "false") + "\n";
    data += "autoSync=" + std::string(settings.autoSync ? "true" : "false") + "\n";
    data += "syncTime=" + std::to_string(settings.syncTime) + "\n";
    data += "syncOnMissionFinish=" + std::string(settings.syncOnMissionFinish ? "true" : "false") + "\n";

    if (!SaveDataAt(data, settingPath, std::ios::out)) {
        LogThis("Failed to write settings to file: " + settingPath);
    }
}

bool SettingsFileExists() {
    std::ifstream file(settingPath);
    return file.good();
}

template<typename FileJsonType = nlohmann::json>
FileJsonType ReadData(DataType type) {
    std::string filename;
    switch (type) {
        case DataType::Warframes:       filename = "../data/Warframe/ExportWarframes_en.json"; break;
        case DataType::Blueprints:      filename = "../data/Warframe/ExportRecipes_en.json"; break;
        case DataType::Customs:         filename = "../data/Warframe/ExportCustoms_en.json"; break;
        case DataType::Drones:          filename = "../data/Warframe/ExportDrones_en.json"; break;
        case DataType::Flavour:         filename = "../data/Warframe/ExportFlavour_en.json"; break;
        case DataType::FusionsBundles:  filename = "../data/Warframe/ExportFusionBundles_en.json"; break;
        case DataType::Gear:            filename = "../data/Warframe/ExportGear_en.json"; break;
        case DataType::Keys:            filename = "../data/Warframe/ExportKeys_en.json"; break;
        case DataType::Images:           filename = "../data/Warframe/ExportManifest.json"; break;
        case DataType::Mods:            filename = "../data/Warframe/ExportUpgrades_en.json"; break;
        case DataType::Regions:         filename = "../data/Warframe/ExportRegions_en.json"; break;
        case DataType::Resources:       filename = "../data/Warframe/ExportResources_en.json"; break;
        case DataType::Sentinels:       filename = "../data/Warframe/ExportSentinels_en.json"; break;
        case DataType::SortieRewards:   filename = "../data/Warframe/ExportSortieRewards_en.json"; break;
        case DataType::Relics:          filename = "../data/Warframe/ExportRelicArcane_en.json"; break;
        case DataType::Weapons:         filename = "../data/Warframe/ExportWeapons_en.json"; break;
        case DataType::Player:         filename = "../data/Player/player_data.json"; break;
        case DataType::Nodes:         filename = "../data/Warframe/XpValues.json"; break;
        default: throw std::runtime_error("Unimplemented DataType!");
    }
    LogThis("reading data type: " + std::to_string(static_cast<int>(type)));
    return FileJsonType::parse(ReadFile(filename));
}

template nlohmann::json ReadData<nlohmann::json>(DataType);
template nlohmann::ordered_json ReadData<nlohmann::ordered_json>(DataType);

// concrete overloads that simply call the instantiated template
nlohmann::json ReadData(DataType type) {
    return ReadData<nlohmann::json>(type);
}

nlohmann::ordered_json ReadDataOrdered(DataType type) {
    return ReadData<nlohmann::ordered_json>(type);
}