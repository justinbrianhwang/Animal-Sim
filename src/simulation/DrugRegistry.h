#pragma once

#include "simulation/DrugCompound.h"
#include <vector>
#include <string>
#include <unordered_map>

namespace animsim {

class DrugRegistry {
public:
    static DrugRegistry& instance();

    // Access
    const DrugCompound& getDrug(const std::string& id) const;
    const std::vector<DrugCompound>& getAllDrugs() const { return m_drugs; }
    bool hasDrug(const std::string& id) const;

    // Custom drug management
    void addDrug(const DrugCompound& drug);
    void updateDrug(const DrugCompound& drug);
    void removeDrug(const std::string& id);

    // JSON persistence
    bool saveCustomDrugs(const std::string& path) const;
    bool loadCustomDrugs(const std::string& path);
    std::string getDefaultSavePath() const;

private:
    DrugRegistry();
    void registerBuiltInDrugs();
    void rebuildIndex();

    std::vector<DrugCompound> m_drugs;
    std::unordered_map<std::string, int> m_index; // id -> index in m_drugs
};

} // namespace animsim
