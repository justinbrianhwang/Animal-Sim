#include "simulation/experiments/DrugEfficacyExperiment.h"
#include "simulation/AnimalRegistry.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace animsim {

DrugEfficacyExperiment::DrugEfficacyExperiment(const ExperimentConfig& config)
    : ExperimentBase(config) {}

void DrugEfficacyExperiment::setup() {
    // Dose levels
    if (m_config.doselevels.empty()) {
        for (int g = 0; g < m_config.numGroups; g++) {
            if (g == 0) {
                m_config.doselevels.push_back(0.0f); // vehicle control
            } else {
                float frac = static_cast<float>(g) / (m_config.numGroups - 1);
                m_config.doselevels.push_back(5.0f + 45.0f * frac);
            }
        }
    }

    // Group labels
    m_groupLabels.clear();
    m_groupLabels.push_back("Vehicle");
    for (int g = 1; g < m_config.numGroups; g++) {
        char buf[64];
        if (g < static_cast<int>(m_config.doselevels.size())) {
            std::snprintf(buf, sizeof(buf), "Drug %.0f mg/kg", m_config.doselevels[g]);
        } else {
            std::snprintf(buf, sizeof(buf), "Group %d", g + 1);
        }
        m_groupLabels.push_back(buf);
    }

    if (m_config.durationHours <= 0.0f) m_config.durationHours = 336.0f; // 14 days
    if (m_config.timeStepHours <= 0.0f) m_config.timeStepHours = 1.0f;

    ExperimentBase::setup();

    // Initialize disease state
    m_diseaseStates.resize(m_animals.size());
    for (size_t i = 0; i < m_animals.size(); i++) {
        auto& ds = m_diseaseStates[i];
        if (m_config.diseaseModel == "tumor") {
            ds.tumorVolume = m_rng.normal(100.0f, 20.0f); // initial tumor 100mm^3
            ds.diseaseSeverity = 20.0f;
        } else if (m_config.diseaseModel == "infection") {
            ds.infectionLoad = m_rng.normal(50.0f, 10.0f);
            ds.diseaseSeverity = 30.0f;
        } else { // anti_inflammatory
            ds.inflammationScore = m_rng.normal(7.0f, 1.0f);
            ds.diseaseSeverity = 40.0f;
        }
    }
}

bool DrugEfficacyExperiment::step(float timeHours, float dt) {
    m_currentTime = timeHours;

    // Daily dosing
    if (timeHours - m_lastDoseTime >= m_dosingInterval || !m_dosesAdministered) {
        administerDoses(timeHours);
        m_dosesAdministered = true;
        m_lastDoseTime = timeHours;
    }

    // Update disease progression and drug effects
    for (size_t i = 0; i < m_animals.size(); i++) {
        auto& animal = *m_animals[i];
        if (!animal.isAlive()) continue;

        auto& ds = m_diseaseStates[i];
        float drugConc = animal.getState().drugConcentration;
        float drugEffect = std::min(drugConc / 30.0f, 2.0f); // normalized drug effect

        if (m_config.diseaseModel == "tumor") {
            // Tumor growth: exponential growth reduced by drug
            float growthRate = 0.02f * (1.0f - 0.6f * drugEffect);
            ds.tumorVolume *= (1.0f + growthRate * dt);
            ds.tumorVolume = std::max(0.0f, ds.tumorVolume + m_rng.normal(0, 2.0f));

            // Drug-induced tumor shrinkage at high effect
            if (drugEffect > 1.0f) {
                ds.tumorVolume *= (1.0f - 0.01f * (drugEffect - 1.0f) * dt);
            }

            ds.diseaseSeverity = std::min(100.0f, ds.tumorVolume / 10.0f);

            // Tumor affects organ health
            if (ds.tumorVolume > 500.0f) {
                float severity = (ds.tumorVolume - 500.0f) / 2000.0f * dt;
                animal.applyToxicDamage(severity * 0.3f, m_rng);
            }

        } else if (m_config.diseaseModel == "infection") {
            // Infection dynamics
            float growthRate = 0.05f * (1.0f - 0.8f * drugEffect);
            ds.infectionLoad *= (1.0f + growthRate * dt);
            ds.infectionLoad = std::max(0.0f, ds.infectionLoad + m_rng.normal(0, 1.0f));

            // Immune response (natural clearance)
            ds.infectionLoad *= (1.0f - 0.005f * dt);

            ds.diseaseSeverity = std::min(100.0f, ds.infectionLoad);

            if (ds.infectionLoad > 80.0f) {
                float severity = (ds.infectionLoad - 80.0f) / 100.0f * dt;
                animal.applyToxicDamage(severity * 0.2f, m_rng);
            }

        } else { // anti_inflammatory
            // Inflammation response
            float naturalRecovery = 0.01f * dt;
            float drugReduction = 0.05f * drugEffect * dt;
            ds.inflammationScore -= naturalRecovery + drugReduction;
            ds.inflammationScore = std::clamp(ds.inflammationScore + m_rng.normal(0, 0.1f), 0.0f, 10.0f);

            ds.diseaseSeverity = ds.inflammationScore * 10.0f;
        }

        // Disease affects vitals
        animal.getState().overallHealth = 100.0f - ds.diseaseSeverity * 0.5f;

        animal.updatePhysiology(dt, m_rng);

        // Drug toxicity at high doses
        if (drugConc > 20.0f) {
            float toxSeverity = (drugConc - 20.0f) / 80.0f * dt;
            animal.applyToxicDamage(toxSeverity, m_rng);
        }

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

void DrugEfficacyExperiment::administerDoses(float timeHours) {
    for (int g = 0; g < m_config.numGroups; g++) {
        float dose = (g < static_cast<int>(m_config.doselevels.size()))
                     ? m_config.doselevels[g] : 0.0f;
        auto groupAnimals = getGroupAnimals(g);
        for (auto* animal : groupAnimals) {
            if (animal->isAlive()) {
                animal->administerDose(dose, m_config.route);
            }
        }
    }
}

void DrugEfficacyExperiment::computeResults() {
    ExperimentBase::computeResults();

    // Compute efficacy metrics per group
    char summary[512];
    std::string summaryStr;
    char buf[128];

    for (int g = 0; g < m_config.numGroups; g++) {
        auto group = getGroupAnimals(g);
        float avgSeverity = 0.0f;
        int count = 0;

        for (auto* a : group) {
            int idx = 0;
            for (size_t i = 0; i < m_animals.size(); i++) {
                if (m_animals[i].get() == a) { idx = static_cast<int>(i); break; }
            }
            avgSeverity += m_diseaseStates[idx].diseaseSeverity;
            count++;
        }

        if (count > 0) avgSeverity /= count;

        std::snprintf(buf, sizeof(buf), "%s: severity=%.1f",
                      m_groupLabels[g].c_str(), avgSeverity);
        if (!summaryStr.empty()) summaryStr += " | ";
        summaryStr += buf;
    }

    // Compute overall efficacy (reduction vs control)
    float controlSeverity = 0.0f, treatedSeverity = 0.0f;
    int controlN = 0, treatedN = 0;
    for (size_t i = 0; i < m_animals.size(); i++) {
        if (m_animals[i]->getGroupIndex() == 0) {
            controlSeverity += m_diseaseStates[i].diseaseSeverity;
            controlN++;
        } else {
            treatedSeverity += m_diseaseStates[i].diseaseSeverity;
            treatedN++;
        }
    }
    if (controlN > 0) controlSeverity /= controlN;
    if (treatedN > 0) treatedSeverity /= treatedN;

    float efficacy = (controlSeverity > 0) ?
        (controlSeverity - treatedSeverity) / controlSeverity * 100.0f : 0.0f;

    std::snprintf(summary, sizeof(summary),
                  "Model: %s | Efficacy: %.1f%% reduction vs control | %s",
                  m_config.diseaseModel.c_str(), efficacy, summaryStr.c_str());
    m_result.summary = summary;
}

} // namespace animsim
