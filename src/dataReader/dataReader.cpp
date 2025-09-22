#include "dataReader.h"

#include <fstream>
#include <regex>
#include <vector>
#include <nlohmann/json.hpp>
#include <type_traits>
#include <unordered_set>

#include "FileAccess/FileAccess.h"



using json = nlohmann::json;

InventoryCategories operator|(InventoryCategories lhs, InventoryCategories rhs) {
    using U = std::underlying_type_t<InventoryCategories>;
    return static_cast<InventoryCategories>(static_cast<U>(lhs) | static_cast<U>(rhs));
}

InventoryCategories operator&(InventoryCategories lhs, InventoryCategories rhs) {
    using U = std::underlying_type_t<InventoryCategories>;
    return static_cast<InventoryCategories>(static_cast<U>(lhs) & static_cast<U>(rhs));
}

bool hasCategoryStrict(InventoryCategories value, InventoryCategories category) {
    using U = std::underlying_type_t<InventoryCategories>;
    return (static_cast<U>(value) & static_cast<U>(category)) == static_cast<U>(category);
}

bool hasCategory(InventoryCategories value, InventoryCategories category) {
    using U = std::underlying_type_t<InventoryCategories>;
    return (static_cast<U>(value) & static_cast<U>(category)) != 0;
}

bool hasNoCategory(InventoryCategories value, InventoryCategories category) {
    using U = std::underlying_type_t<InventoryCategories>;
    return (static_cast<U>(value) & static_cast<U>(category)) == 0;
}

InventoryCategories addCategory(InventoryCategories value, InventoryCategories category) {
    return value | category;
}

InventoryCategories unsetCategory(InventoryCategories value, InventoryCategories category) {
    using U = std::underlying_type_t<InventoryCategories>;
    return static_cast<InventoryCategories>(static_cast<U>(value) & ~static_cast<U>(category));
}

template<typename ReturnType, typename JsonType = nlohmann::json>
ReturnType getValueByKey(const JsonType& jsonData,
                         const std::string& key,
                         const ReturnType& default_value = ReturnType{},
                         bool suppressError = false)
{
    //LogThis("made it to getValueByKey");
    if (!jsonData.contains(key)) {
        if (!suppressError) LogThis("Key not found: " + key);
        return default_value;
    }
    //LogThis("if?");
    try {
        return jsonData.at(key).template get<ReturnType>();
    }
    catch (const std::exception& e) {
        LogThis("Exception getting key '" + key + "' : " + e.what());
        return default_value;
    }
}

std::string nameFromId(const std::string& id, bool supressError = false) {
    if (NameMap.empty()) {
        RefreshNameMap();
    }

    if (auto nameIt = NameMap.find(id); nameIt != NameMap.end()) {
        return nameIt->second;
    }
    if (!supressError) {
        LogThis("Name not found! check: " + id);
    }

    return id;
}

//Important that this functions on the final product id not the blueprint;
InventoryCategories GetItemCategoryFromId(const std::string& id) {
    InventoryCategories category = InventoryCategories::None;

    // Prime
    if (id.find("Prime") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Prime);
        //special founder exclusives
        if (id.find("/Lotus/Powersuits/Excalibur/ExcaliburPrime") != std::string::npos ||
        id.find("/Lotus/Weapons/Tenno/Pistol/LatoPrime") != std::string::npos ||
        id.find("/Lotus/Weapons/Tenno/Melee/LongSword/SkanaPrime") != std::string::npos) {
            category = addCategory(category, InventoryCategories::FounderSpecial);
            return category;
        }
    }

    // Warframe & related
    if (id.find("/Lotus/Powersuits/") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Warframe);
    }

    // Weapon & components
    if (id.find("Weapon") != std::string::npos && !hasCategory(category, InventoryCategories::Warframe)) {
        // Lich Nemesis
        if (id.find("KuvaLich") != std::string::npos) {
            category = addCategory(category, InventoryCategories::Nemesis);
        }
        //Kuva 2nd try
        else if (nameFromId(id).rfind("Kuva", 0) == 0) {
            category = addCategory(category, InventoryCategories::Nemesis);
        }
        // Sister Nemesis
        else if (id.find("BoardExec") != std::string::npos) {
            category = addCategory(category, InventoryCategories::Nemesis);
        }
        //Tenet 2nd try
        else if (nameFromId(id).rfind("Tenet", 0) == 0) {
            category = addCategory(category, InventoryCategories::Nemesis);
        }
        // Coda Nemesis
        else if (id.find("InfestedLich") != std::string::npos) {
            category = addCategory(category, InventoryCategories::Nemesis);
        }
        //Sold by Baro Ki'Teer
        else if (id.find("VoidTrader") != std::string::npos) {
            category = addCategory(category, InventoryCategories::DucatItem);
        }
        //just a weapon
        category = addCategory(category, InventoryCategories::Weapon);
    }

    // WeaponParts as Component
    if (id.find("WeaponParts") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Component | InventoryCategories::Weapon | InventoryCategories::NoMastery);
    }

    //catch all for Resources
    if (id.find("Resources") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Component | InventoryCategories::NoMastery);
    }

    // Companion
    if (id.find("Pet") != std::string::npos || id.find("Sentinel") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Companion);
    }

    // Necramech
    if (id.find("Mech") != std::string::npos || id.find("ThanoTech") != std::string::npos || id.find("NechroTech") != std::string::npos) {
        category = addCategory(category, InventoryCategories::Necramech);
    }

    // Archwing and Archweapon
    if (id.find("Archwing") != std::string::npos) {
        if (hasCategory(category, InventoryCategories::Weapon)) {
            category = addCategory(category, InventoryCategories::Archweapon);
        } else {
            category = addCategory(category, InventoryCategories::Archwing);
        }
    }

    // DojoSpecial = Research
    if (id.find("DojoSpecial") != std::string::npos || id.find("Research") != std::string::npos) {
        category = addCategory(category, InventoryCategories::DojoSpecial | InventoryCategories::NoMastery);
    }

    // LandingCraft & Ships
    if (id.find("LandingCraft") != std::string::npos || id.find("Ships") != std::string::npos) {
        category = addCategory(category, InventoryCategories::LandingCraft | InventoryCategories::NoMastery);
    }

    // Modular (for some reason the spectra Vandal is under 'CorpusModularPistol')
    if (id.find("Hoverboard") != std::string::npos || (id.find("Modular") != std::string::npos && id.find("Vandal") == std::string::npos)) {
        category = addCategory(category, InventoryCategories::Modular);
    }

    return category;
}

std::unordered_map<std::string, std::string> extractMapFromArray(
    const json& j,
    const std::string& arrayKey,
    const std::string& valueKey = "name")
{
    std::unordered_map<std::string, std::string> result;

    // Check if arrayKey exists and is an array
    if (!j.contains(arrayKey) || !j[arrayKey].is_array()) return result;

    const auto& arr = j[arrayKey];

    for (const auto& entry : arr) {
        std::string id = entry.value("uniqueName", "");
        std::string val = entry.value(valueKey, "");
        if (!id.empty()) {
            result[id] = val;
        }
    }

    return result;
}

MasteryInfo GetMasteryLevelForItem(const std::string& itemId, int Affinity) {
    InventoryCategories  cat = GetItemCategoryFromId(itemId);
    bool isWeapon = hasCategory(cat, InventoryCategories::Weapon);
    bool isOver40Capable = itemId.find("BallasSwordWeapon") != std::string::npos || hasCategory(cat, InventoryCategories::Nemesis) || (hasCategory(cat, InventoryCategories::Necramech) && !isWeapon);
    //BallasSwordWeapon = Paracesis

    int maxRank = isOver40Capable ? 40 : 30;

    auto xpForRank = [](int rank) -> int {
        return 1000 * rank * rank;
    };

    int level = 0;
    for (int rank = 1; rank <= maxRank; ++rank) {
        int threshold = xpForRank(rank);
        // Halve the threshold for weapons
        if (Affinity >= (isWeapon ? threshold / 2 : threshold)) {
            level = rank;

        } else {
            break;
        }
    }

    if (level > maxRank) level = maxRank;

    return MasteryInfo{level, maxRank, isWeapon};
}

std::string imgFromId(const std::string& id) {
    if (ImgMap.empty()) {
        RefreshImgMap();
    }

    if (auto imgIt = ImgMap.find(id); imgIt != ImgMap.end()) {
        return imgIt->second;
    }
    LogThis("Img not found! check: " + id);
    return "";
}

std::string resultFromBp(const std::string& bpId) {
    if (BpToResultMap.empty()) {
        RefreshResultBpMap();
    }
    if (auto it = BpToResultMap.find(bpId); it != BpToResultMap.end()) {
        return it->second;
    }

    return bpId;
}

std::string bpFromResult(const std::string& resultId) {
    if (ResultToBpMap.empty()) {
        RefreshResultBpMap();
    }
    if (auto it = ResultToBpMap.find(resultId); it != ResultToBpMap.end()) {
        return it->second;
    }

    return resultId;
}

void RefreshImgMap() {
    ImgMap.clear();
    const nlohmann::json images = ReadData(DataType::Images);
    ImgMap = extractMapFromArray(images, "Manifest", "textureLocation");
    LogThis("Loaded " + std::to_string(ImgMap.size()) + " image entries.");
}

void RefreshNameMap() {
    NameMap.clear();
    const nlohmann::json frames = ReadData(DataType::Warframes);
    const nlohmann::json weapons = ReadData(DataType::Weapons);
    const nlohmann::json companions = ReadData(DataType::Sentinels);
    const nlohmann::json resources = ReadData(DataType::Resources);
    const nlohmann::json mods = ReadData(DataType::Mods);
    const nlohmann::json custom = ReadData(DataType::Customs);

    std::unordered_map<std::string, std::string> idToWarframeName = extractMapFromArray(frames, "ExportWarframes");
    LogThis("Loaded " + std::to_string(idToWarframeName.size()) + " warframe entries.");
    NameMap.insert(idToWarframeName.begin(), idToWarframeName.end());

    std::unordered_map<std::string, std::string> idToWeaponName = extractMapFromArray(weapons, "ExportWeapons");
    LogThis("Loaded " + std::to_string(idToWeaponName.size()) + " weapon entries.");
    NameMap.insert(idToWeaponName.begin(), idToWeaponName.end());

    std::unordered_map<std::string, std::string> idToCompanionName = extractMapFromArray(companions, "ExportSentinels");
    LogThis("Loaded " + std::to_string(idToCompanionName.size()) + " companion entries.");
    NameMap.insert(idToCompanionName.begin(), idToCompanionName.end());

    std::unordered_map<std::string, std::string> idToComponentName = extractMapFromArray(resources, "ExportResources");
    LogThis("Loaded " + std::to_string(idToComponentName.size()) + " component entries.");
    NameMap.insert(idToComponentName.begin(), idToComponentName.end());

    std::unordered_map<std::string, std::string> idToModName = extractMapFromArray(mods, "ExportUpgrades");
    LogThis("Loaded " + std::to_string(idToModName.size()) + " mod entries.");
    NameMap.insert(idToModName.begin(), idToModName.end());

    std::unordered_map<std::string, std::string> idToCustomName = extractMapFromArray(custom, "ExportCustoms");
    LogThis("Loaded " + std::to_string(idToCustomName.size()) + " custom entries.");
    NameMap.insert(idToCustomName.begin(), idToCustomName.end());
}

void RefreshCountMap() {
    CountMap.clear();

    const auto& recipes = getValueByKey<nlohmann::json>(ReadData(DataType::Player), "Recipes");
    const auto& misc    = getValueByKey<nlohmann::json>(ReadData(DataType::Player), "MiscItems");

    auto processEntries = [&](const nlohmann::json& entries) {
        for (const auto& entry : entries) {
            std::string entryId   = entry.value("ItemType", "");
            int         itemCount = entry.value("ItemCount", 0);
            if (!entryId.empty()) {
                CountMap[entryId] = itemCount;
            }
        }
    };

    processEntries(recipes);
    processEntries(misc);

    LogThis("Loaded " + std::to_string(CountMap.size()) + " owned unique blueprint entries.");
}

void RefreshUpgradeMap() {
    RivenFingerprintMap.clear();
    UpgradeMap.clear();

    const auto& rawUpgrades = getValueByKey<nlohmann::json>(ReadData(DataType::Player), "RawUpgrades");
    const auto& upgrades    = getValueByKey<nlohmann::json>(ReadData(DataType::Player), "Upgrades");

    for (const auto& entry : rawUpgrades) {
        std::string entryId   = entry.value("ItemType", "");
        int         itemCount = entry.value("ItemCount", 0);
        if (!entryId.empty() && itemCount > 0) {
            UpgradeMap[entryId][0] += itemCount;
        }
    }

    for (const auto& entry : upgrades) {
        std::string entryId = entry.value("ItemType", "");
        if (entryId.empty()) continue;

        std::string fingerprint = entry.value("UpgradeFingerprint", "");
        if (fingerprint.empty()) continue;

        try {
            if (fingerprint.find("\"compat\"") != std::string::npos) {
                std::string rivenId;
                if (entry.contains("ItemId") && entry["ItemId"].contains("$oid")) {
                    rivenId = entry["ItemId"]["$oid"].get<std::string>();
                } else {
                    rivenId = entry.value("ItemId", "");
                }

                if (!rivenId.empty()) {
                    RivenFingerprintMap.insert({rivenId, fingerprint});
                } else {
                    LogThis("Could not get rivenId for riven: " + entryId);
                }
                continue;
            }

            nlohmann::json fpJson = nlohmann::json::parse(fingerprint);
            int lvl = fpJson.value("lvl", -1);
            if (lvl >= 0) {
                UpgradeMap[entryId][lvl] += 1;
            } else {
                LogThis("error getting level from fingerprint id: " + entryId);
            }
        } catch (...) {
            LogThis("fatal error getting level from id: " + entryId);
        }
    }

    LogThis("Loaded levelled upgrades for " + std::to_string(UpgradeMap.size()) + " unique ids.");
}


void RefreshResultBpMap() {
    BpToResultMap.clear();
    ResultToBpMap.clear();
    const auto recipes = getValueByKey<nlohmann::json>(ReadData(DataType::Blueprints), "ExportRecipes");
    for (const auto& entry : recipes) {
        std::string bp = entry.value("uniqueName", "");
        std::string result = entry.value("resultType", "");
        if (!bp.empty() && !result.empty()) {
            BpToResultMap[bp] = result;
            ResultToBpMap[result] = bp;
        }
    }
    LogThis("Loaded " + std::to_string(BpToResultMap.size()) + " blueprint ↔ result mappings.");
}

void RefreshResultBpMap(const nlohmann::json& recipes) {
    BpToResultMap.clear();
    ResultToBpMap.clear();
    for (const auto& entry : recipes) {
        std::string bp = entry.value("uniqueName", "");
        std::string result = entry.value("resultType", "");
        if (!bp.empty() && !result.empty()) {
            BpToResultMap[bp] = result;
            ResultToBpMap[result] = bp;
        }
    }
    LogThis("Loaded " + std::to_string(BpToResultMap.size()) + " blueprint ↔ result mappings.");
}

void RefreshXPMap() {
    XPMap.clear();

    nlohmann::json player = ReadData(DataType::Player);
    const auto& xpArray = getValueByKey<nlohmann::json>(player, "XPInfo");

    for (const auto& entry : xpArray) {
        std::string id = entry.value("ItemType", "");
        int xp = entry.value("XP", 0);  // default to 0 if XP is missing

        if (!id.empty()) {
            XPMap[id] = xp;
        }
    }

    LogThis("Refreshed XPMap with " + std::to_string(XPMap.size()) + " entries.");
}

int XPFromId(const std::string& id) {
    if (XPMap.empty()) {
        RefreshXPMap();
    }

    if (auto it = XPMap.find(id); it != XPMap.end()) {
        return it->second;
    }

    return 0;
}

std::vector<RankCount> GetLevelledCounts(const std::string& id, bool refresh = false) {
    if (UpgradeMap.empty() || refresh) {
        RefreshUpgradeMap();
    }

    std::vector<RankCount> result;
    if (auto it = UpgradeMap.find(id); it != UpgradeMap.end()) {
        for (auto& [rank, count] : it->second) {
            result.push_back(RankCount{rank, count});
        }
    }

    return result;
}

int CountFromId(const std::string& id, bool refresh = false) {
    if (CountMap.empty() || refresh) {
        RefreshCountMap();
    }

    if (auto bpIt = CountMap.find(id); bpIt != CountMap.end()) {
        return bpIt->second;
    }
    //LogThis("Blueprint not found! check: " + id);
    return 0;
}

void UpdateCounts(IModData& modData) {
    modData.setPossessionCount(GetLevelledCounts(modData.getId()));
}

std::string replaceLast(const std::string& str, const std::string& from, const std::string& to) {
    size_t pos = str.rfind(from);
    if (pos == std::string::npos) return str;
    return str.substr(0, pos) + to + str.substr(pos + from.length());
};

void UpdateMastery(IData& item) {
    int xpValue = XPFromId(item.getCraftedId());
    MasteryInfo info{};

    if (xpValue > 0) {
        info = GetMasteryLevelForItem(item.getCraftedId(), xpValue);
    } else {
        if (!hasCategory(item.getCategory(), InventoryCategories::NoMastery)) {
            info = GetMasteryLevelForItem(item.getCraftedId(), 0);
        }
    }

    item.setInfo(info);
}

void UpdateCounts(IDataContainer& container, bool isRelic) {
    using enum ItemPossessionType;

    IData& main = container.modifyMainData();
    UpdateMastery(main);
    const std::string& id = main.getId();

    if (isRelic) {
        main.setPossessionCount(Intact, CountFromId(id));
        main.setPossessionCount(Exceptional, CountFromId(replaceLast(id, "Bronze", "Silver")));
        main.setPossessionCount(Flawless, CountFromId(replaceLast(id, "Bronze", "Gold")));
        main.setPossessionCount(Radiant, CountFromId(replaceLast(id, "Bronze", "Platinum")));
    } else {
        const std::string resultId = resultFromBp(id);
        if (resultId != id) {
            main.setPossessionCount(Blueprint, CountFromId(id));
        }
        main.setPossessionCount(Crafted, CountFromId(resultId));
    }

    for (const IData* sub : container.getSubData()) {
        if (!sub) continue;

        const std::string& subId = sub->getId();

        const std::string resultId = resultFromBp(subId);
        if (resultId != subId) {
            const_cast<IData&>(*sub).setPossessionCount(Blueprint, CountFromId(subId));
        }
        const_cast<IData&>(*sub).setPossessionCount(Crafted, CountFromId(resultId));
    }
}

const char* CategoryToString(InventoryCategories cat) {
    switch(cat) {
        case InventoryCategories::Prime: return "Prime";
        case InventoryCategories::Warframe: return "Warframe";
        case InventoryCategories::Weapon: return "Weapon";
        case InventoryCategories::Companion: return "Companion";
        case InventoryCategories::Necramech: return "Necramech";
        case InventoryCategories::Archwing: return "Archwing";
        case InventoryCategories::Archweapon: return "Archweapon";
        case InventoryCategories::Component: return "Component";
        case InventoryCategories::Nemesis: return "Nemesis";
        case InventoryCategories::DojoSpecial: return "DojoSpecial";
        case InventoryCategories::LandingCraft: return "LandingCraft";
        case InventoryCategories::Modular: return "Modular";
        case InventoryCategories::Relic: return "Relic";
        case InventoryCategories::NoMastery: return "NoMastery";
        case InventoryCategories::Arcane: return "Arcane";
        case InventoryCategories::Mod: return "Mod";
        case InventoryCategories::FounderSpecial: return "FounderSpecial";
        case InventoryCategories::DucatItem: return "DucatItem";
        default: return "Unknown";
    }
}

std::vector<Recipe> GetRecipes()
{
    std::vector<Recipe> recipes;

    LogThis("called GetRecipes()");

    const auto frames = getValueByKey<nlohmann::json>(ReadData(DataType::Warframes), "ExportWarframes");
    const auto weapons = getValueByKey<nlohmann::json>(ReadData(DataType::Weapons), "ExportWeapons");
    const auto companions = getValueByKey<nlohmann::json>(ReadData(DataType::Sentinels), "ExportSentinels");
    const auto blueprints = getValueByKey<nlohmann::json>(ReadData(DataType::Blueprints), "ExportRecipes");
    RefreshResultBpMap(blueprints);
    // Prepare a combined list of all items from the three datasets
    std::vector allItems = { &frames, &weapons, &companions };

    // Helper lambda to find blueprint JSON by id
    auto findBlueprint = [blueprints](const std::string& bpId) -> const nlohmann::json* {
        for (const auto& bp : blueprints) {
            if (bp.contains("uniqueName") && bp["uniqueName"].is_string() && bp["uniqueName"] == bpId) {
                return &bp;
            }
        }
        return nullptr;
    };

    Recipe recipe{};
    ItemData resultItem{};
    std::string blueprintId;
    ItemData ingredientItem{};
    std::unordered_set<std::string> seenIds;

    for (const auto* dataset : allItems)
    {
        for (const auto& item : *dataset)
        {
            if (!item.contains("uniqueName") || !item["uniqueName"].is_string()) {
                LogThis("Skipping item with no valid uniqueName: " + item.dump());
                continue;
            }

            std::string craftedId = item["uniqueName"];

            if (item["productCategory"] == "SpecialItems" && craftedId.find("Kavat") == std::string::npos) { //skips exalted weapons and such
                continue; //For some reason the khora kavat does count for mr
            }

            if (seenIds.count(craftedId) > 0) {
                LogThis("Duplicate id detected, skipping: " + craftedId);
                continue;
            }

            seenIds.insert(craftedId);

            // Lookup blueprint ID from craftedId
            blueprintId = bpFromResult(craftedId);

            if (craftedId.find("Pet") != std::string::npos &&
                craftedId.find("Head") == std::string::npos &&
                craftedId.find("Weapon") == std::string::npos &&
                craftedId.find("PowerSuit") == std::string::npos) { //for some reason antigens/mutagens are in here; making sure not to filter out moa head parts and only head
                continue;
            }

            //Vulpaphyla's and Predasite's are in here 2 times
            if (craftedId.find("WoundedInfested") != std::string::npos) {
                continue;
            }

            //filters out some zaw duplicates?
            if (craftedId.find("PvPVariant") != std::string::npos) {
                continue;
            }

            //filters out what i can only assume to be Noctua summoned via dante's ability
            if (craftedId.find("Doppelganger") != std::string::npos) {
                continue;
            }
            //filters out all amps (excluding the Mr relevant part)
            if (craftedId.find("OperatorAmplifiers") != std::string::npos && craftedId.find("Barrel") == std::string::npos) {
                continue;
            }

            // Filters out Zaws
            if (craftedId.find("ModularMelee") != std::string::npos && craftedId.find("Tip") == std::string::npos) {
                continue;
            }

            // Filters out kitguns
            if (craftedId.find("SUModular") != std::string::npos && craftedId.find("Barrel") == std::string::npos) {
                continue;
            }

            // Filters out infested Kitguns
            if (craftedId.find("InfKitGun") != std::string::npos && craftedId.find("Barrel") == std::string::npos) {
                continue;
            }

            // Filters out k-drives
            if (craftedId.find("HoverboardParts") != std::string::npos && craftedId.find("Deck") == std::string::npos) {
                continue;
            }

            // Setup main item
            resultItem.craftedId = craftedId;
            resultItem.id = blueprintId;

            resultItem.name = nameFromId(craftedId, true);
            if (resultItem.name.compare(0, 11, "<ARCHWING> ") == 0) {
                resultItem.name.erase(0, 11);
            }
            // Skip if name is same as id (means no name found)
            if (resultItem.name == craftedId) {
                LogThis("Did not find Name of Item: " + item.dump());
                continue;
            }

            resultItem.image = imgFromId(craftedId);
            resultItem.category = GetItemCategoryFromId(craftedId);

            UpdateMastery(resultItem);
            recipe.mainItem = resultItem;

            // Initialize empty ingredients
            recipe.subItems.clear();

            // If blueprint exists, load ingredients
            if (blueprintId != craftedId) {
                const nlohmann::json* blueprint = findBlueprint(blueprintId);
                if (blueprint && blueprint->contains("ingredients") && (*blueprint)["ingredients"].is_array()) {
                    for (const auto& ingredientJson : (*blueprint)["ingredients"])
                    {
                        std::string ingredientId = ingredientJson.value("ItemType", "");
                        if (ingredientId.empty()) {
                            continue;
                        }

                        ingredientItem.id = bpFromResult(ingredientId);
                        ingredientItem.craftedId = ingredientId;

                        ingredientItem.name = nameFromId(ingredientId);
                        ingredientItem.image = imgFromId(ingredientId);

                        ingredientItem.category = addCategory(
                            ingredientItem.category,
                            GetItemCategoryFromId(ingredientItem.craftedId) | InventoryCategories::Component | InventoryCategories::NoMastery
                        );

                        recipe.subItems.push_back(ingredientItem);
                    }
                }
            }

            UpdateCounts(recipe, false);
            recipes.push_back(recipe);
        }
    }

    LogThis("created " + std::to_string(recipes.size()) + " recipes.");

    return recipes;
}

int GetMasteryRank() {
    LogThis("called GetName");
    nlohmann::json player = ReadData(DataType::Player);
    LogThis("Parsed Player Json successfully");
    return getValueByKey<int>(player, "PlayerLevel", 0);
}

std::string toTitleCase(const std::string& str) {
    std::string out = str;
    std::transform(out.begin(), out.end(), out.begin(), ::tolower);
    if (!out.empty()) out[0] = static_cast<char>(::toupper(out[0]));
    return out;
}

std::vector<std::vector<ItemData>> SplitEquipment(std::vector<Recipe> recipes)
{
    LogThis("Called GetEquipment");
    std::vector<ItemData> primeItems;
    std::vector<ItemData> normalItems;;

    for (const auto& recipe : recipes) {
        const auto& output = recipe.mainItem;

        if (output.info.maxLevel == 0) {
            continue; // skip non mastery relevant items
        }

        // Split into prime vs normal; toTitleCase only capitalizes the very first character which is never part of 'prime'
        if (toTitleCase(output.craftedId).find("prime") != std::string::npos) {
            primeItems.push_back(output);
        } else {
            normalItems.push_back(output);
        }
    }

    return { normalItems, primeItems };
}

std::vector<MissionData> GetMissions(const json& playerJson, const json& NodesXp, const json& allNodes) {
    LogThis("called GetMissions");
    std::vector<MissionData> missions;
    MissionData data;

    // Index mission completion by tag
    std::unordered_map<std::string, json> missionDataMap;
    json missionsJson = getValueByKey<nlohmann::json>(playerJson, "Missions");
    for (const auto& mission : missionsJson) {
        std::string tag = getValueByKey<std::string>(mission, "Tag");
        if (!tag.empty()) {
            missionDataMap[tag] = mission;
            if (tag.find("Junction") != std::string::npos) { //Junctions wont be found in public export so handle here
                data.tag = tag;
                data.name = tag; //there are names for the junctions
                size_t pos = tag.find("To", 1);
                if (pos != std::string::npos) {
                    data.region = tag.substr(0, pos);
                }
                else {
                    data.region = tag; //fallback just make it their own region
                }
                data.baseXp = 1000;
                int completes = getValueByKey<int>(mission, "Completes", 0, true);
                int tier = getValueByKey<int>(mission, "Tier", 0, true);
                data.isCompleted = completes > 0;
                data.sp = (tier == 1); // Steel Path completed
                missions.push_back(data);
            }
        }
    }

    // Process all nodes from allNodes["ExportRegions"]
    json exportRegions = getValueByKey<nlohmann::json>(allNodes, "ExportRegions");
    for (const auto& node : exportRegions) {

        data.tag = getValueByKey<std::string>(node, "uniqueName");
        data.name = getValueByKey<std::string>(node, "name");
        data.region = getValueByKey<std::string>(node, "systemName");

        // Completion logic
        auto it = missionDataMap.find(data.tag);
        if (it != missionDataMap.end()) {
            const json& m = it->second;
            int completes = getValueByKey<int>(m, "Completes", 0, true);
            int tier = getValueByKey<int>(m, "Tier", 0, true);
            //TODO: Verify if tier can be 1 and completes 0 or more concretely when it increases the tier to 1
            data.isCompleted = completes > 0;
            data.sp = (tier == 1); // Steel Path completed
        }

        // XP lookup
        if (NodesXp.contains(data.tag)) {
            data.baseXp = NodesXp[data.tag].get<int>();
        } else {
            data.baseXp = 0;
            LogThis("No Xp found for: " + data.tag);
        }

        missions.push_back(data);
    }

    return missions;
}

std::vector<MissionData> GetIncompleteMissions(const std::vector<MissionData>& missions) {
    std::vector<MissionData> result;
    for (const auto& mission : missions) {
        if (!mission.isCompleted && mission.baseXp > 0) {
            result.push_back(mission);
        }
    }
    return result;
}

int GetTotalMissionXp(const std::vector<MissionData>& missions) {
    int total = 0;
    for (const auto& mission : missions) {
        int xp = mission.baseXp;
        if (mission.isCompleted) {
            xp *= mission.sp ? 2 : 1;
            total += xp;
        }
    }
    return total;
}

MissionSummary GetMissionsSummary() {
    const json& playerJson = ReadData(DataType::Player);
    const json& NodesXp = ReadData(DataType::Nodes);
    const json& allNodes = ReadData(DataType::Regions);
    MissionSummary summary;
    summary.missions = GetMissions(playerJson, NodesXp, allNodes);
    summary.totalCount = static_cast<int>(summary.missions.size());
    summary.incompleteCount = static_cast<int>(std::count_if(
        summary.missions.begin(), summary.missions.end(),
        [](const MissionData& m) { return !m.isCompleted && m.baseXp > 0; }
    ));
    return summary;
}

int GetIntrinsicXp(const json& jsonData) {
    int totalXp = 0;
    json intrinsicJson = getValueByKey<nlohmann::json>(jsonData, "PlayerSkills");
    for (auto& intrinsic : intrinsicJson.items()) {
        int intrinsicLevel = intrinsic.value().get<int>();
        totalXp += intrinsicLevel * 1500;
    }
    return totalXp;
}

int extraSpecialXp = 0;

int GetCurrentXP() {
    LogThis("called GetXP");
    nlohmann::json player = ReadData(DataType::Player);
    LogThis("Parsed Player Json successfully");
    int total = 0;
    MasteryInfo info{};
    extraSpecialXp = 0;
    const auto& xpArray = getValueByKey<nlohmann::json>(player, "XPInfo");
    for (const auto& entry : xpArray) {
        std::string id = entry.value("ItemType", "");
        int xp = entry.value("XP", 0);  // Use 0 as default if "XP" missing

        if (!id.empty()) {
            info = GetMasteryLevelForItem(id, xp);
            int toAdd = 0;
            if (info.usedHalfAffinity) toAdd += info.level * 100;
            else toAdd += info.level * 200;
            total += toAdd;
            //LogThis(id + " : " + std::to_string(toAdd));
            if (nameFromId(id, true) == id) {
                extraSpecialXp += toAdd;
                LogThis("special XP not found in export: " + id);
            }
        }
    }
    LogThis("parsed XPInfo for a total of " + std::to_string(total) + " Mastery Xp.");
    std::vector<MissionData> missiondata = GetMissions(player, ReadData(DataType::Nodes), ReadData(DataType::Regions)); //TODO: solve problem: how to get new values on updates; for DataType::Nodes
    int missionXp = GetTotalMissionXp(missiondata);
    total += missionXp;
    LogThis("got mission xp: " + std::to_string(missionXp));
    int intrinsicXp = GetIntrinsicXp(player);
    total += intrinsicXp;
    LogThis("got Intrinsic xp: " + std::to_string(intrinsicXp));

    return total;
}

std::string intrinsicNameFromKey(const std::string& key, const std::string& categoryRaw) {
    std::string name = key.substr(4);
    size_t pos = name.rfind('_');
    if (pos != std::string::npos) {
        name = name.substr(pos + 1);
    }
    return toTitleCase(name);
}

std::vector<IntrinsicCategory> GetIntrinsics() {
    LogThis("Getting ordered Intrinsics");
    const nlohmann::ordered_json intrinsics = getValueByKey<nlohmann::ordered_json, nlohmann::ordered_json>(ReadDataOrdered(DataType::Player), "PlayerSkills");

    std::vector<IntrinsicCategory> categories;
    IntrinsicCategory* currentCategory = nullptr;
    std::string categoryRaw;

    for (auto& [key, value] : intrinsics.items()) {
        if (key.rfind("LPP_", 0) == 0) {
            // Found a new category
            categoryRaw = key.substr(4); // strip "LPP_"

            IntrinsicCategory cat;
            if (categoryNameMap.count(categoryRaw)) {
                cat.name = categoryNameMap[categoryRaw];
            } else {
                cat.name = toTitleCase(categoryRaw);
            }
            currentCategory = &categories.emplace_back(std::move(cat));
            //LogThis("CurrentCategory updated: " + currentCategory->name);
        }
        else if (key.rfind("LPS_", 0) == 0 && currentCategory) {
            Intrinsic s;
            s.name = intrinsicNameFromKey(key, categoryRaw);
            s.level = value.get<int>();
            //LogThis("   Added Intrinsic : " + s.name);
            currentCategory->skills.push_back(std::move(s));
        }
    }
    return categories;
}

Rarity parseRarity(const std::string& rarityStr) {
    if (rarityStr == "COMMON")   return Rarity::Common;
    if (rarityStr == "UNCOMMON") return Rarity::Uncommon;
    if (rarityStr == "RARE")     return Rarity::Rare;
    if (rarityStr == "LEGENDARY") return Rarity::Legendary;
    return Rarity::Unknown;
}

std::vector<Relic> GetRelics() {
    LogThis("called GetRelics");
    nlohmann::json relics = getValueByKey<nlohmann::json>(ReadData(DataType::Relics), "ExportRelicArcane");
    nlohmann::json player = ReadData(DataType::Player);

    std::vector<Relic> result;
    Relic relic;
    for (const auto& r : relics) {

        // Main item = the relic itself
        relic.mainItem.id   = r.value("uniqueName", "");

        // Skip any arcanes
        if (relic.mainItem.id.rfind("/Lotus/Types/Game/Projections/", 0) != 0) {
            continue;
        }

        // Skip any relic upgrade versions
        const std::string bronzeSuffix = "Bronze";
        if (relic.mainItem.id.size() < bronzeSuffix.size() || relic.mainItem.id.compare(relic.mainItem.id.size() - bronzeSuffix.size(), bronzeSuffix.size(), bronzeSuffix) != 0) {
            continue;
        }

        relic.mainItem.name = r.value("name", "");
        relic.mainItem.image = imgFromId(relic.mainItem.id);
        relic.mainItem.possessionCounts[ItemPossessionType::Intact] = CountFromId(relic.mainItem.id);
        relic.mainItem.possessionCounts[ItemPossessionType::Exceptional] = CountFromId(replaceLast(relic.mainItem.id, "Bronze", "Silver"));
        relic.mainItem.possessionCounts[ItemPossessionType::Flawless] = CountFromId(replaceLast(relic.mainItem.id, "Bronze", "Gold"));
        relic.mainItem.possessionCounts[ItemPossessionType::Radiant] = CountFromId(replaceLast(relic.mainItem.id, "Bronze", "Platinum"));

        relic.mainItem.category = InventoryCategories::Relic;
        relic.mainItem.rarity = Rarity::Unknown;


        // Sub-items = rewards inside the relic
        if (r.contains("relicRewards") && r["relicRewards"].is_array()) {
            for (const auto& reward : r["relicRewards"]) {
                RelicData item;

                item.id   = reward.value("rewardName", ""); //this for some reason adds a StoreItems/ to id which is found nowhere else
                size_t pos = item.id .find("StoreItems/");
                if (pos != std::string::npos) {
                    item.id .erase(pos, std::string("StoreItems/").length());
                }

                item.name = nameFromId(resultFromBp(item.id));
                item.image = imgFromId(item.id);
                //Not sure if we will use Category for the relic rewards
                item.category = GetItemCategoryFromId(resultFromBp(item.id));//InventoryCategories::Prime; //cant use this because we dont have crafted here: GetItemCategoryFromId(item.id);
                item.possessionCounts[ItemPossessionType::Blueprint] = CountFromId(item.id);
                item.possessionCounts[ItemPossessionType::Crafted] = CountFromId(resultFromBp(item.id));
                item.rarity = parseRarity(reward.value("rarity", ""));

                relic.subItems.push_back(std::move(item));
            }
        }

        result.push_back(std::move(relic));
    }

    return result;
}

std::unordered_set<std::string> blacklistedArcanes = {
    "/Lotus/Upgrades/CosmeticEnhancers/Defensive/CorrosiveProcResist",
    "/Lotus/Upgrades/CosmeticEnhancers/Defensive/GasProcResist",
    "/Lotus/Upgrades/CosmeticEnhancers/Defensive/ImpactProcResist",
    "/Lotus/Upgrades/CosmeticEnhancers/Defensive/PoisonProcResist",
    "/Lotus/Upgrades/CosmeticEnhancers/Defensive/PunctureProcResist",
    "/Lotus/Upgrades/CosmeticEnhancers/Utility/DamageReductionDuringRevive",
    "/Lotus/Upgrades/CosmeticEnhancers/Utility/SlowerBleedOutOnPredeath"
};

std::vector<Arcane> GetArcanes() {
    LogThis("called GetArcanes");
    nlohmann::json relics = getValueByKey<nlohmann::json>(ReadData(DataType::Relics), "ExportRelicArcane");
    nlohmann::json player = ReadData(DataType::Player);

    std::vector<Arcane> result;
    Arcane arcane;
    std::unordered_map<std::string, Arcane> arcaneMap;
    for (const auto& r : relics) {

        //why de?
        if (!r.contains("levelStats")) {
            continue;
        }
        arcane.id   = r.value("uniqueName", "");

        if (blacklistedArcanes.find(arcane.id) != blacklistedArcanes.end()) {
            continue;
        }

        // Skip any relic
        if (arcane.id.rfind("/Lotus/Upgrades/CosmeticEnhancers/", 0) != 0) {
            continue;
        }

        arcane.name = r.value("name", "");
        arcane.image = imgFromId(arcane.id);
        arcane.possessionCounts = GetLevelledCounts(arcane.id);
        std::vector<std::string> stats;

        for (const auto& level : r["levelStats"]) {
            if (!level["stats"].empty()) {
                stats.push_back(level["stats"][0].get<std::string>());
            }
        }

        arcane.stats = stats;
        arcane.category = InventoryCategories::Arcane;
        arcane.rarity = parseRarity(r.value("rarity", ""));

        auto iter = arcaneMap.find(arcane.name);
        if (iter != arcaneMap.end()) {
            const auto& existing = iter->second;
            bool arcaneHasRarity = (arcane.rarity != Rarity::Unknown);
            bool existingHasRarity = (existing.rarity != Rarity::Unknown);

            if (arcaneHasRarity && !existingHasRarity) {
                //LogThis("Duplicate name found: " + arcane.name + ", replacing with more complete entry (rarity set).");
                arcaneMap[arcane.name] = arcane;
            } else if (!arcaneHasRarity && existingHasRarity) {
                //LogThis("Duplicate name found but existing entry is more complete (rarity set): " + arcane.name);
            } else {
                // Both have unknown rarity (or both have rarity set)
                //LogThis("Duplicate name with same completeness: " + arcane.name);
                // Log rarity + levelStats for both
                /*LogThis("Existing rarity: " + std::to_string(static_cast<int>(existing.rarity)) + ", levelStats:");
                for (const auto& line : arcane.stats) {
                    LogThis("  " + line);
                }

                LogThis("Current rarity: " + std::to_string(static_cast<int>(arcane.rarity)) + ", levelStats:");
                for (const auto& line : arcane.stats) {
                    LogThis("  " + line);
                }*/
                arcaneMap[arcane.name] = arcane;
            }
        } else {
            arcaneMap[arcane.name] = arcane;
            if (arcane.rarity == Rarity::Unknown) {
                //LogThis(arcane.name + " has no rarity, defaulting to common");
                arcane.rarity = Rarity::Common;
            }

            //LogThis(arcane.name);
            result.push_back(std::move(arcane));
        }
    }

    return result;
}

std::vector<Mod> GetMods() {
    LogThis("called GetMods");
    nlohmann::json mods = getValueByKey<nlohmann::json>(ReadData(DataType::Mods), "ExportUpgrades");
    nlohmann::json railjackMods = getValueByKey<nlohmann::json>(ReadData(DataType::Mods), "ExportAvionics");
    nlohmann::json player = ReadData(DataType::Player);

    std::vector<Mod> result;
    Mod mod;
    std::unordered_map<std::string, Arcane> arcaneMap;
    for (const auto& r : mods) {

        mod.id   = r.value("uniqueName", "");
        if (mod.id.rfind("Randomized", 0) != std::string::npos) continue; //rivens
        mod.name = r.value("name", "");
        if (mod.name == "Unfused Artifact") continue; //not sure what this is
        mod.image = imgFromId(mod.id);
        mod.possessionCounts = GetLevelledCounts(mod.id);
        mod.baseDrain = r.value("baseDrain", 0);
        mod.fusionLimit = r.value("fusionLimit", 0);
        mod.category = InventoryCategories::Mod;
        mod.rarity = parseRarity(r.value("rarity", ""));

        result.push_back(std::move(mod));
    }

    return result;
}

void ResetStorage() {

}