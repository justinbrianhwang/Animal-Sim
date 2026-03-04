#include "simulation/experiments/PKExperiment.h"
#include "simulation/AnimalRegistry.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <numeric>

namespace animsim {

PKExperiment::PKExperiment(const ExperimentConfig& config)
    : ExperimentBase(config) {}

void PKExperiment::setup() {
    // Set up dose levels - different doses per group
    if (m_config.doselevels.empty()) {
        for (int g = 0; g < m_config.numGroups; g++) {
            if (g == 0) {
                m_config.doselevels.push_back(0.0f); // control
            } else {
                float frac = static_cast<float>(g) / (m_config.numGroups - 1);
                m_config.doselevels.push_back(10.0f + 40.0f * frac); // 10-50 mg/kg range
            }
        }
    }

    // Group labels
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

    // Default PK duration
    if (m_config.durationHours <= 0.0f) m_config.durationHours = 48.0f;
    if (m_config.timeStepHours <= 0.0f) m_config.timeStepHours = 0.5f;

    ExperimentBase::setup();

    // Initialize PK profiles
    m_profiles.resize(m_animals.size());
}

bool PKExperiment::step(float timeHours, float dt) {
    m_currentTime = timeHours;

    // Administer dose at t=0
    if (!m_doseAdministered && timeHours >= 0.0f) {
        administerDose(timeHours);
        m_doseAdministered = true;
    }

    // Pharmacokinetic model update
    for (size_t i = 0; i < m_animals.size(); i++) {
        auto& animal = *m_animals[i];
        if (!animal.isAlive()) continue;

        const auto& sp = AnimalRegistry::getParams(m_config.species);

        // Compartmental PK model
        float ke = m_config.eliminationRate;   // elimination rate constant

        float& conc = animal.getState().drugConcentration;

        if (m_config.compartments == 1) {
            // One-compartment model
            conc *= std::exp(-ke * dt * sp.metabolicRate);
        } else {
            // Two-compartment: add distribution phase
            float kd = 0.5f;
            conc *= std::exp(-(ke + kd * 0.1f) * dt * sp.metabolicRate);
        }

        // Record PK profile
        m_profiles[i].times.push_back(timeHours);
        m_profiles[i].concentrations.push_back(conc);
        if (conc > m_profiles[i].cmax) {
            m_profiles[i].cmax = conc;
            m_profiles[i].tmax = timeHours;
        }

        animal.updatePhysiology(dt, m_rng);
        animal.checkMortality(m_rng);
        if (animal.getState().status == AnimalStatus::Dead &&
            animal.getState().timeOfDeath < 0.0f) {
            animal.getState().timeOfDeath = timeHours;
        }
    }

    takeSnapshot(timeHours);

    if (timeHours >= m_config.durationHours) {
        m_complete = true;
        return false;
    }
    return true;
}

void PKExperiment::administerDose(float timeHours) {
    for (int g = 0; g < m_config.numGroups; g++) {
        float dose = (g < static_cast<int>(m_config.doselevels.size()))
                     ? m_config.doselevels[g] : 0.0f;
        auto groupAnimals = getGroupAnimals(g);
        for (auto* animal : groupAnimals) {
            animal->administerDose(dose, m_config.route);
        }
    }
}

void PKExperiment::computeResults() {
    ExperimentBase::computeResults();

    auto& pk = m_result.pkResult;

    // Compute average PK parameters across treated groups
    float totalCmax = 0.0f, totalTmax = 0.0f;
    int treatedCount = 0;

    for (size_t i = 0; i < m_animals.size(); i++) {
        if (m_animals[i]->getGroupIndex() == 0) continue; // skip control
        totalCmax += m_profiles[i].cmax;
        totalTmax += m_profiles[i].tmax;
        treatedCount++;
    }

    if (treatedCount > 0) {
        pk.cmax = totalCmax / treatedCount;
        pk.tmax = totalTmax / treatedCount;
    }

    // Compute half-life from elimination rate
    pk.halfLife = 0.693f / std::max(0.01f, m_config.eliminationRate);

    // Compute AUC using trapezoidal rule from average profile
    // Build average concentration-time profile
    if (!m_profiles.empty() && !m_profiles[0].times.empty()) {
        pk.timePoints = m_profiles[0].times;
        pk.concentrations.resize(pk.timePoints.size(), 0.0f);

        for (size_t i = 0; i < m_animals.size(); i++) {
            if (m_animals[i]->getGroupIndex() == 0) continue;
            for (size_t t = 0; t < m_profiles[i].concentrations.size() &&
                               t < pk.concentrations.size(); t++) {
                pk.concentrations[t] += m_profiles[i].concentrations[t];
            }
        }
        if (treatedCount > 0) {
            for (auto& c : pk.concentrations) c /= treatedCount;
        }

        // Trapezoidal AUC
        pk.auc = 0.0f;
        for (size_t i = 1; i < pk.timePoints.size(); i++) {
            float dt = pk.timePoints[i] - pk.timePoints[i-1];
            pk.auc += 0.5f * (pk.concentrations[i-1] + pk.concentrations[i]) * dt;
        }

        // Clearance = Dose / AUC
        float avgDose = 0.0f;
        for (int g = 1; g < m_config.numGroups; g++) {
            if (g < static_cast<int>(m_config.doselevels.size()))
                avgDose += m_config.doselevels[g];
        }
        if (m_config.numGroups > 1) avgDose /= (m_config.numGroups - 1);
        pk.clearance = (pk.auc > 0.0f) ? avgDose / pk.auc : 0.0f;
    }

    char summary[256];
    std::snprintf(summary, sizeof(summary),
                  "Cmax = %.2f mg/L, Tmax = %.1f h, t1/2 = %.1f h, AUC = %.1f mg*h/L",
                  pk.cmax, pk.tmax, pk.halfLife, pk.auc);
    m_result.summary = summary;
}

} // namespace animsim
