#include "simulation/experiments/ToxicologyExperiment.h"
#include "simulation/AnimalRegistry.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace animsim {

ToxicologyExperiment::ToxicologyExperiment(const ExperimentConfig& config)
    : ExperimentBase(config) {}

void ToxicologyExperiment::setup() {
    const auto& sp = AnimalRegistry::getParams(m_config.species);

    // Set up dose levels if not provided
    if (m_config.doselevels.empty()) {
        // Default LD50 dose range based on species sensitivity
        float baseDose = 100.0f / sp.ld50Sensitivity;
        m_config.doselevels.clear();
        for (int g = 0; g < m_config.numGroups; g++) {
            if (g == 0) {
                m_config.doselevels.push_back(0.0f); // control
            } else {
                float frac = static_cast<float>(g) / (m_config.numGroups - 1);
                m_config.doselevels.push_back(baseDose * 0.2f + baseDose * 1.8f * frac);
            }
        }
    }

    // Generate group labels
    m_groupLabels.clear();
    for (int g = 0; g < m_config.numGroups; g++) {
        char buf[64];
        if (g < static_cast<int>(m_config.doselevels.size())) {
            std::snprintf(buf, sizeof(buf), "%.0f mg/kg", m_config.doselevels[g]);
        } else {
            std::snprintf(buf, sizeof(buf), "Group %d", g + 1);
        }
        m_groupLabels.push_back(buf);
    }

    // Set duration
    if (m_config.toxStudyType == "ld50") {
        if (m_config.durationHours <= 0.0f) m_config.durationHours = 72.0f;
    } else if (m_config.toxStudyType == "chronic") {
        if (m_config.durationHours <= 0.0f) m_config.durationHours = 720.0f;
    }

    // Create animal populations
    ExperimentBase::setup();
}

bool ToxicologyExperiment::step(float timeHours, float dt) {
    m_currentTime = timeHours;

    // Administer doses at the start
    if (!m_dosesAdministered && timeHours >= 0.0f) {
        administerDoses(timeHours);
        m_dosesAdministered = true;
        m_doseTime = timeHours;
    }

    // For chronic study, re-dose periodically
    if (m_config.toxStudyType == "chronic") {
        float hoursSinceDose = timeHours - m_doseTime;
        if (hoursSinceDose >= 24.0f) {
            administerDoses(timeHours);
            m_doseTime = timeHours;
        }
    }

    // Update each animal
    for (auto& animal : m_animals) {
        if (!animal->isAlive()) continue;

        animal->updatePhysiology(dt, m_rng);

        // Apply toxic damage based on drug concentration
        float conc = animal->getState().drugConcentration;
        if (conc > 10.0f) {
            float severity = (conc - 10.0f) / 50.0f; // normalized severity
            severity *= dt; // scale by time step
            animal->applyToxicDamage(severity, m_rng);
        }

        animal->checkMortality(m_rng);
        if (animal->getState().status == AnimalStatus::Dead &&
            animal->getState().timeOfDeath < 0.0f) {
            animal->getState().timeOfDeath = timeHours;
        }
    }

    takeSnapshot(timeHours);

    if (timeHours >= m_config.durationHours) {
        m_complete = true;
        return false;
    }
    return true;
}

void ToxicologyExperiment::administerDoses(float timeHours) {
    for (int g = 0; g < m_config.numGroups; g++) {
        float dose = (g < static_cast<int>(m_config.doselevels.size()))
                     ? m_config.doselevels[g] : 0.0f;
        auto groupAnimals = getGroupAnimals(g);
        for (auto* animal : groupAnimals) {
            animal->administerDose(dose, m_config.route);
        }
    }
}

void ToxicologyExperiment::computeResults() {
    ExperimentBase::computeResults();

    auto& tox = m_result.toxResult;
    tox.totalAnimals = static_cast<int>(m_animals.size());
    tox.totalDeaths = getDeadCount();
    tox.mortalityRate = (tox.totalAnimals > 0)
        ? static_cast<float>(tox.totalDeaths) / tox.totalAnimals * 100.0f
        : 0.0f;

    // Mortality by group
    tox.doseLevels = m_config.doselevels;
    tox.groupLabels = m_groupLabels;
    tox.mortalityByGroup.clear();

    for (int g = 0; g < m_config.numGroups; g++) {
        auto group = getGroupAnimals(g);
        int deaths = 0;
        for (auto* a : group) {
            if (!a->isAlive()) deaths++;
        }
        float rate = group.empty() ? 0.0f : static_cast<float>(deaths) / group.size() * 100.0f;
        tox.mortalityByGroup.push_back(rate);
    }

    // Compute LD50 using linear interpolation of dose-response
    tox.ld50 = computeLD50();
    tox.ld50Lower = tox.ld50 * 0.75f; // rough CI
    tox.ld50Upper = tox.ld50 * 1.30f;

    // Hill coefficient estimation
    if (tox.ld50 > 0.0f && tox.mortalityByGroup.size() >= 3) {
        // Simplified Hill slope from the steepest part of the curve
        float maxSlope = 0.0f;
        for (size_t i = 1; i < tox.mortalityByGroup.size(); i++) {
            if (i < tox.doseLevels.size() && tox.doseLevels[i] > tox.doseLevels[i-1]) {
                float dResponse = tox.mortalityByGroup[i] - tox.mortalityByGroup[i-1];
                float dDose = std::log(tox.doseLevels[i] / std::max(1.0f, tox.doseLevels[i-1]));
                if (dDose > 0.0f) {
                    maxSlope = std::max(maxSlope, dResponse / dDose);
                }
            }
        }
        tox.hillCoefficient = maxSlope / 25.0f; // normalize to typical Hill range
        tox.hillCoefficient = std::max(0.5f, std::min(5.0f, tox.hillCoefficient));
    } else {
        tox.hillCoefficient = 2.0f; // default
    }

    char summary[256];
    std::snprintf(summary, sizeof(summary),
                  "LD50 = %.1f mg/kg [%.1f - %.1f], Mortality: %d/%d (%.1f%%)",
                  tox.ld50, tox.ld50Lower, tox.ld50Upper,
                  tox.totalDeaths, tox.totalAnimals, tox.mortalityRate);
    m_result.summary = summary;
}

float ToxicologyExperiment::computeLD50() const {
    // Linear interpolation to find dose at 50% mortality
    if (m_config.doselevels.size() < 2) return 0.0f;

    std::vector<std::pair<float, float>> doseResponse; // (dose, mortality%)
    for (int g = 0; g < m_config.numGroups; g++) {
        if (g >= static_cast<int>(m_config.doselevels.size())) break;
        auto group = getGroupAnimals(g);
        int dead = 0;
        for (auto* a : group) {
            if (!a->isAlive()) dead++;
        }
        float mort = group.empty() ? 0.0f : static_cast<float>(dead) / group.size() * 100.0f;
        doseResponse.emplace_back(m_config.doselevels[g], mort);
    }

    // Sort by dose
    std::sort(doseResponse.begin(), doseResponse.end());

    // Find where 50% falls
    for (size_t i = 1; i < doseResponse.size(); i++) {
        float d0 = doseResponse[i-1].first, r0 = doseResponse[i-1].second;
        float d1 = doseResponse[i].first, r1 = doseResponse[i].second;
        if (r0 <= 50.0f && r1 >= 50.0f && r1 > r0) {
            float frac = (50.0f - r0) / (r1 - r0);
            return d0 + frac * (d1 - d0);
        }
    }

    // If 50% was never reached, extrapolate from last two points
    if (doseResponse.size() >= 2) {
        auto& last = doseResponse.back();
        if (last.second > 0.0f && last.second < 50.0f) {
            return last.first * 50.0f / last.second;
        }
        return last.first; // best estimate
    }
    return 0.0f;
}

} // namespace animsim
