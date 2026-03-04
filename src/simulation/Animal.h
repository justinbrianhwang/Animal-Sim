#pragma once

#include "simulation/Types.h"
#include "simulation/RandomEngine.h"
#include <string>

namespace animsim {

class Animal {
public:
    Animal(int id, Species species, int groupIndex, RandomEngine& rng);

    // State access
    int getId() const { return m_id; }
    const std::string& getLabel() const { return m_label; }
    Species getSpecies() const { return m_species; }
    int getGroupIndex() const { return m_groupIndex; }
    const AnimalDetailedState& getState() const { return m_state; }
    AnimalDetailedState& getState() { return m_state; }
    bool isAlive() const { return m_state.status == AnimalStatus::Alive || m_state.status == AnimalStatus::Moribund; }

    // Dose administration (drug-aware)
    void administerDose(float doseMgKg, const std::string& route,
                        const std::string& drugId = "generic_toxicant");

    // Treatment procedures
    void applySalineFlush();
    void applyActivatedCharcoal();
    void applyAntidote(const std::string& targetDrugId);

    // Update physiology for one time step
    void updatePhysiology(float dt, RandomEngine& rng);

    // Apply organ damage from toxicity (drug-specific)
    void applyToxicDamage(float severity, RandomEngine& rng);
    void applyDrugSpecificDamage(const std::string& drugId, float severity, RandomEngine& rng);

    // Check death conditions
    void checkMortality(RandomEngine& rng);

    // Kill
    void kill(float timeHours);
    void euthanize(float timeHours);

private:
    void initializePhysiology(RandomEngine& rng);
    void recomputeTotals();
    float computeHealthFromOrgans() const;

    int m_id;
    std::string m_label;
    Species m_species;
    int m_groupIndex;
    AnimalDetailedState m_state;

    // Individual variation factors
    float m_sensitivityFactor = 1.0f; // individual drug sensitivity
    float m_metabolismFactor = 1.0f;  // individual metabolism rate
};

} // namespace animsim
