#ifndef FILEACCESS_H
#define FILEACCESS_H
#include <ios>
#include <string>
#include <nlohmann/json.hpp>

enum class DataType {
    Warframes,
    Blueprints,
    Customs,
    Drones,
    Flavour,
    FusionsBundles,
    Gear,
    Keys,
    Images,
    Regions,
    Relics,
    Resources,
    Sentinels,
    SortieRewards,
    Mods,
    Weapons,
    Player,
    Nodes
};

struct Settings {
    bool fullItems = true;
    bool hideFounder = true;
    bool saveDownload = false;
    bool authTokenGetterRead = false;
    bool startWithSystem = false;
    bool autoSync = false;
    int syncTime = 10;
    bool syncOnMissionFinish = false;
    std::string nonce{};
};

static const std::string logPath = "../data/Log/Log.txt";
static const std::string settingPath = "../data/Settings/Settings.cfg";
static const std::string downloadPath = "../data/Download/";
void LogThis(const std::string& str);
bool SaveDataAt(const std::string& data, const std::string& path, std::ios_base::openmode mode = std::ios::out);
std::shared_ptr<const std::vector<uint8_t>> GetImageByFileOrDownload(const std::string& image);
Settings LoadSettings();
void WriteSettings(const Settings& settings);
bool SettingsFileExists();

nlohmann::json ReadData(DataType type);
nlohmann::ordered_json ReadDataOrdered(DataType type);

#endif //FILEACCESS_H
