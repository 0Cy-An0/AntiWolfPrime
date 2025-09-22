#ifndef DATAREADER_H
#define DATAREADER_H
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>
#include <nlohmann/json_fwd.hpp>

enum class JsonType {
    Int,
    String,
    Bool,
    Array,
    Object,
    Double,
    Null
};

enum class InventoryCategories : uint32_t {
    None            = 0,
    Prime           = 1 << 0,
    Warframe        = 1 << 1,
    Weapon          = 1 << 2,
    Companion       = 1 << 3,
    Necramech       = 1 << 4,
    Archwing        = 1 << 5,
    Archweapon      = 1 << 6,
    Component       = 1 << 7,
    Nemesis         = 1 << 8,
    DojoSpecial     = 1 << 9,
    LandingCraft    = 1 << 10,
    Modular         = 1 << 11,
    Relic           = 1 << 12,
    NoMastery       = 1 << 13,
    Arcane          = 1 << 14,
    Mod             = 1 << 15,
    FounderSpecial  = 1 << 16,
    DucatItem       = 1 << 17
};

struct RankCount {
    int rank = 0;
    int count = 0;
};

enum class ItemPossessionType {
    Blueprint,
    Crafted,
    Intact,
    Exceptional,
    Flawless,
    Radiant
};

enum class Rarity {
    Common,
    Uncommon,
    Rare,
    Legendary,
    Unknown
};

struct MasteryInfo {
    int level;
    int maxLevel;
    bool usedHalfAffinity;
};

struct IData {
    virtual ~IData() = default;

    [[nodiscard]] virtual const std::string& getId() const = 0;
    [[nodiscard]] virtual const std::string& getCraftedId() const = 0;
    [[nodiscard]] virtual const std::string& getName() const = 0;
    [[nodiscard]] virtual const std::string& getImage() const = 0;
    [[nodiscard]] virtual InventoryCategories getCategory() const = 0;
    [[nodiscard]] virtual const bool& getMastered() const = 0;
    [[nodiscard]] virtual const std::unordered_map<ItemPossessionType, int>& getPossessionCounts() const = 0;
    virtual void setPossessionCount(ItemPossessionType type, int count) = 0;
    virtual bool setInfo(MasteryInfo newInfo) = 0;
};

struct IModData {
    virtual ~IModData() = default;

    [[nodiscard]] virtual const std::string& getId() const = 0;
    [[nodiscard]] virtual const std::string& getName() const = 0;
    [[nodiscard]] virtual const std::string& getImage() const = 0;
    [[nodiscard]] virtual InventoryCategories getCategory() const = 0;
    [[nodiscard]] virtual Rarity getRarity() const = 0;
    [[nodiscard]] virtual const std::vector<RankCount>& getPossessionCounts() const = 0;
    [[nodiscard]] virtual int getMaxRank() const = 0;
    virtual void setPossessionCount(std::vector<RankCount> newCounts) = 0;
};

struct IDataContainer {
    virtual ~IDataContainer() = default;

    [[nodiscard]] virtual const IData& getMainData() const = 0;
    [[nodiscard]] virtual IData& modifyMainData() = 0;
    // Warning: getSubData() is not thread-safe and uses a static buffer internally
    [[nodiscard]] virtual const std::vector<const IData*>& getSubData() const = 0;
};

struct ItemData : IData{
    std::string id{};
    std::string craftedId{};
    std::string name{};
    std::string image{};
    MasteryInfo info = {};
    bool mastered = false;
    std::unordered_map<ItemPossessionType, int> possessionCounts;
    InventoryCategories category{};

    // Implement IData
    [[nodiscard]] const std::string& getId() const override { return id; }
    [[nodiscard]] const std::string& getCraftedId() const override { return craftedId; }
    [[nodiscard]] const std::string& getName() const override { return name; }
    [[nodiscard]] const std::string& getImage() const override { return image; }
    [[nodiscard]] InventoryCategories getCategory() const override { return category; }
    [[nodiscard]] const bool& getMastered() const override { return mastered; }
    [[nodiscard]] const std::unordered_map<ItemPossessionType, int>& getPossessionCounts() const override { return possessionCounts; }
    void setPossessionCount(ItemPossessionType type, int count) override {
        possessionCounts[type] = count;
    }

    bool setInfo(const MasteryInfo newInfo) override {
        info = newInfo;
        mastered = (newInfo.level == newInfo.maxLevel);
        return true;
    }
};

static std::vector<const IData*> IDataTmp;

struct Recipe : IDataContainer {
    ItemData mainItem{};
    std::vector<ItemData> subItems;

    // Implement IDataContainer
    [[nodiscard]] const IData& getMainData() const override { return mainItem; }

    [[nodiscard]] IData& modifyMainData() override { return mainItem; }

    [[nodiscard]] const std::vector<const IData*>& getSubData() const override {

        IDataTmp.clear();
        for (const auto& s : subItems) {
            IDataTmp.push_back(&s);
        }
        return IDataTmp;
    }
};

//TODO: dont Relics sub items also need craftedid? or should have
struct RelicData : IData{
    std::string id{};
    std::string name{};
    std::string image{};
    bool mastered = false; //this is always false; just triggers the symbol for the view Widget; Maybe favourite?
    std::unordered_map<ItemPossessionType, int> possessionCounts;
    InventoryCategories category{};
    Rarity rarity{Rarity::Unknown};

    // Implement IData
    [[nodiscard]] const std::string& getId() const override { return id; }
    [[nodiscard]] const std::string& getCraftedId() const override { return id; } //just has no crafted variant
    [[nodiscard]] const std::string& getName() const override { return name; }
    [[nodiscard]] const std::string& getImage() const override { return image; }
    [[nodiscard]] InventoryCategories getCategory() const override { return category; }
    [[nodiscard]] const bool& getMastered() const override { return mastered; }
    [[nodiscard]] const std::unordered_map<ItemPossessionType, int>& getPossessionCounts() const override { return possessionCounts; }
    void setPossessionCount(ItemPossessionType type, int count) override {
        possessionCounts[type] = count;
    }

    bool setInfo(const MasteryInfo newInfo) override {
        //has no info will do nothing
        return false;
    }
};

struct Relic : IDataContainer {
    RelicData mainItem{};
    std::vector<RelicData> subItems;

    // Implement IDataContainer
    [[nodiscard]] const IData& getMainData() const override { return mainItem; }

    [[nodiscard]] IData& modifyMainData() override { return mainItem; }

    [[nodiscard]] const std::vector<const IData*>& getSubData() const override {
        IDataTmp.clear();
        for (const auto& s : subItems) {
            IDataTmp.push_back(&s);
        }
        return IDataTmp;
    }
};

struct Arcane : IModData {
    std::string id{};
    std::string name{};
    std::string image{};
    std::vector<RankCount> possessionCounts;
    InventoryCategories category{};
    Rarity rarity{Rarity::Unknown};
    std::vector<std::string> stats{};

    // Implement IData
    [[nodiscard]] const std::string& getId() const override { return id; }
    [[nodiscard]] const std::string& getName() const override { return name; }
    [[nodiscard]] const std::string& getImage() const override { return image; }
    [[nodiscard]] InventoryCategories getCategory() const override { return category; }
    [[nodiscard]] Rarity getRarity() const override { return rarity; }
    [[nodiscard]] const std::vector<RankCount>& getPossessionCounts() const override { return possessionCounts; }
    void setPossessionCount(const std::vector<RankCount> newCounts) override {
        possessionCounts = newCounts;
    }
    int getMaxRank() const override { return 5; }
};

struct Mod : IModData {
    std::string id{};
    std::string name{};
    std::string image{};
    std::vector<RankCount> possessionCounts;
    InventoryCategories category{};
    Rarity rarity{Rarity::Unknown};
    int baseDrain = 0;
    int fusionLimit = 0;

    // Implement IData
    [[nodiscard]] const std::string& getId() const override { return id; }
    [[nodiscard]] const std::string& getName() const override { return name; }
    [[nodiscard]] const std::string& getImage() const override { return image; }
    [[nodiscard]] InventoryCategories getCategory() const override { return category; }
    [[nodiscard]] Rarity getRarity() const override { return rarity; }
    [[nodiscard]] const std::vector<RankCount>& getPossessionCounts() const override { return possessionCounts; }
    void setPossessionCount(const std::vector<RankCount> newCounts) override {
        possessionCounts = newCounts;
    }
    int getMaxRank() const override { return fusionLimit; }
};

struct MissionData {
    std::string name;
    std::string tag;
    std::string region;
    int baseXp = 0;
    bool sp = false;
    bool isCompleted = false;
};

struct MissionSummary {
    int totalCount = 0;
    int incompleteCount = 0;
    std::vector<MissionData> missions;
};

struct Intrinsic {
    std::string name;
    int level;
};

struct IntrinsicCategory {
    std::string name;
    std::vector<Intrinsic> skills;
};

static std::map<std::string, std::string> categoryNameMap = {
    {"SPACE", "Railjack"},
    {"DRIFTER", "Duviri"}
};

static std::unordered_map<std::string, std::string> ImgMap;
static std::unordered_map<std::string, std::string> NameMap;
static std::unordered_map<std::string, int> CountMap;
static std::unordered_map<std::string, std::string> BpToResultMap;
static std::unordered_map<std::string, std::string> ResultToBpMap;
static std::unordered_map<std::string, std::map<int,int>> UpgradeMap;
static std::unordered_map<std::string, std::string> RivenFingerprintMap;
static std::unordered_map<std::string, int> XPMap;
extern int extraSpecialXp;

// Bitwise OR
InventoryCategories operator|(InventoryCategories lhs, InventoryCategories rhs);

// Bitwise AND
InventoryCategories operator&(InventoryCategories lhs, InventoryCategories rhs);

// Check if 'value' has 'category' (strict)
bool hasCategoryStrict(InventoryCategories value, InventoryCategories category);

// Include check: returns true if ANY bits in 'category' are set in 'value'
bool hasCategory(InventoryCategories value, InventoryCategories category);

// Exclude check: returns true if NONE of the bits in 'category' are set in 'value'
bool hasNoCategory(InventoryCategories value, InventoryCategories category);

// Return 'value' with 'category' set
InventoryCategories addCategory(InventoryCategories value, InventoryCategories category);

// Return 'value' with 'category' unset
InventoryCategories unsetCategory(InventoryCategories value, InventoryCategories category);

int GetMasteryRank();
int GetCurrentXP();
MissionSummary GetMissionsSummary();
std::vector<IntrinsicCategory> GetIntrinsics();
std::vector<std::vector<ItemData>> SplitEquipment(std::vector<Recipe> recipes);
std::vector<Recipe> GetRecipes();
std::vector<Relic> GetRelics();
std::vector<Arcane> GetArcanes();
std::vector<Mod> GetMods();

//game relevant
void RefreshImgMap();
void RefreshNameMap();
void RefreshResultBpMap();
std::string imgFromId(const std::string& id);

//player relevant
void RefreshCountMap();
void RefreshUpgradeMap();
void RefreshXPMap();

void UpdateCounts(IDataContainer& container, bool isRelic);
void UpdateCounts(IModData& modData);
int GetExtraSpecialXp();

#endif //DATAREADER_H
