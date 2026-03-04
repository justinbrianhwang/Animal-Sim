#include "simulation/experiments/SkinIrritationExperiment.h"
#include "simulation/AnimalRegistry.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace animsim {

SkinIrritationExperiment::SkinIrritationExperiment(const ExperimentConfig& config)
    : ExperimentBase(config) {}

void SkinIrritationExperiment::setup() {
    // Concentration levels
    if (m_config.doselevels.empty()) {
        m_config.doselevels.push_back(0.0f); // vehicle control
        for (int g = 1; g < m_config.numGroups; g++) {
            float frac = static_cast<float>(g) / (m_config.numGroups - 1);
            m_config.doselevels.push_back(1.0f + 99.0f * frac); // 1-100% concentration
        }
    }

    // Group labels
    m_groupLabels.clear();
    m_groupLabels.push_back("Vehicle");
    for (int g = 1; g < m_config.numGroups; g++) {
        char buf[64];
        if (g < static_cast<int>(m_config.doselevels.size())) {
            std::snprintf(buf, sizeof(buf), "%.0f%% Conc", m_config.doselevels[g]);
        } else {
            std::snprintf(buf, sizeof(buf), "Group %d", g + 1);
        }
        m_groupLabels.push_back(buf);
    }

    if (m_config.skinTestType == "draize") {
        if (m_config.durationHours <= 0.0f) m_config.durationHours = 72.0f;
    } else { // LLNA
        if (m_config.durationHours <= 0.0f) m_config.durationHours = 120.0f; // 5 days
    }

    ExperimentBase::setup();

    m_skinData.resize(m_animals.size());
    m_nextScoringTime = 1.0f;
}

bool SkinIrritationExperiment::step(float timeHours, float dt) {
    m_currentTime = timeHours;

    // Apply test substance at t=0
    if (!m_substanceApplied && timeHours >= 0.0f) {
        applyTestSubstance();
        m_substanceApplied = true;
    }

    // Score skin at intervals
    if (timeHours >= m_nextScoringTime) {
        scoreSkin(timeHours);
        m_nextScoringTime = timeHours + m_scoringInterval;
    }

    // Update animals
    for (size_t i = 0; i < m_animals.size(); i++) {
        auto& animal = *m_animals[i];
        if (!animal.isAlive()) continue;

        // Skin irritation affects organ health (skin specifically)
        int gi = animal.getGroupIndex();
        float conc = (gi < static_cast<int>(m_config.doselevels.size()))
                     ? m_config.doselevels[gi] : 0.0f;

        // Skin damage proportional to concentration
        float irritation = conc / 100.0f * 0.02f * dt;
        animal.getState().organs.skin -= irritation * m_rng.uniform(5.0f, 15.0f);
        animal.getState().organs.skin = std::max(0.0f, animal.getState().organs.skin);

        // LLNA: lymph node proliferation
        if (m_config.skinTestType == "llna") {
            float si = 1.0f + conc / 50.0f * m_rng.normal(1.0f, 0.2f);
            m_skinData[i].stimulationIndex = std::max(m_skinData[i].stimulationIndex, si);
            m_skinData[i].lymphNodeWeight = 5.0f * si + m_rng.normal(0, 0.5f);
        }

        animal.updatePhysiology(dt, m_rng);
        animal.checkMortality(m_rng);
    }

    takeSnapshot(timeHours);

    if (timeHours >= m_config.durationHours) {
        m_complete = true;
        return false;
    }
    return true;
}

void SkinIrritationExperiment::applyTestSubstance() {
    for (size_t i = 0; i < m_animals.size(); i++) {
        auto& animal = *m_animals[i];
        int gi = animal.getGroupIndex();
        float conc = (gi < static_cast<int>(m_config.doselevels.size()))
                     ? m_config.doselevels[gi] : 0.0f;

        // Dermal application
        animal.administerDose(conc * 0.1f, "dermal"); // low systemic absorption
    }
}

void SkinIrritationExperiment::scoreSkin(float timeHours) {
    for (size_t i = 0; i < m_animals.size(); i++) {
        auto& animal = *m_animals[i];
        if (!animal.isAlive()) continue;

        int gi = animal.getGroupIndex();
        float conc = (gi < static_cast<int>(m_config.doselevels.size()))
                     ? m_config.doselevels[gi] : 0.0f;

        SkinScore score;

        // Draize scoring based on concentration and time
        float intensity = conc / 100.0f;
        float timeFactor = std::min(1.0f, timeHours / 48.0f); // peaks at 48h

        // Natural healing after peak
        if (timeHours > 48.0f) {
            timeFactor *= std::exp(-0.02f * (timeHours - 48.0f));
        }

        score.erythema = std::clamp(
            4.0f * intensity * timeFactor + m_rng.normal(0, 0.3f), 0.0f, 4.0f);
        score.edema = std::clamp(
            3.0f * intensity * timeFactor + m_rng.normal(0, 0.2f), 0.0f, 4.0f);
        score.eschar = std::clamp(
            2.0f * intensity * timeFactor * timeFactor + m_rng.normal(0, 0.1f), 0.0f, 4.0f);

        m_skinData[i].scores.emplace_back(timeHours, score);
    }
}

void SkinIrritationExperiment::computeResults() {
    ExperimentBase::computeResults();

    char summary[512];
    std::string summaryStr;
    char buf[128];

    if (m_config.skinTestType == "draize") {
        // Primary Irritation Index per group
        for (int g = 0; g < m_config.numGroups; g++) {
            auto group = getGroupAnimals(g);
            float totalPII = 0.0f;
            int count = 0;

            for (auto* a : group) {
                int idx = 0;
                for (size_t i = 0; i < m_animals.size(); i++) {
                    if (m_animals[i].get() == a) { idx = static_cast<int>(i); break; }
                }

                // Average all scores for this animal
                float avgScore = 0.0f;
                int scoreCount = 0;
                for (auto& [t, s] : m_skinData[idx].scores) {
                    avgScore += s.total();
                    scoreCount++;
                }
                if (scoreCount > 0) {
                    totalPII += avgScore / scoreCount;
                    count++;
                }
            }

            float pii = (count > 0) ? totalPII / count : 0.0f;
            const char* classification = "Non-irritant";
            if (pii >= 5.0f) classification = "Severe irritant";
            else if (pii >= 2.0f) classification = "Moderate irritant";
            else if (pii >= 0.5f) classification = "Mild irritant";

            std::snprintf(buf, sizeof(buf), "%s: PII=%.2f (%s)",
                          m_groupLabels[g].c_str(), pii, classification);
            if (!summaryStr.empty()) summaryStr += " | ";
            summaryStr += buf;
        }

        std::snprintf(summary, sizeof(summary), "Draize Test | %s", summaryStr.c_str());

    } else { // LLNA
        for (int g = 0; g < m_config.numGroups; g++) {
            auto group = getGroupAnimals(g);
            float avgSI = 0.0f;
            int count = 0;

            for (auto* a : group) {
                int idx = 0;
                for (size_t i = 0; i < m_animals.size(); i++) {
                    if (m_animals[i].get() == a) { idx = static_cast<int>(i); break; }
                }
                avgSI += m_skinData[idx].stimulationIndex;
                count++;
            }

            if (count > 0) avgSI /= count;
            const char* classification = (avgSI >= 3.0f) ? "Sensitizer" : "Non-sensitizer";

            std::snprintf(buf, sizeof(buf), "%s: SI=%.2f (%s)",
                          m_groupLabels[g].c_str(), avgSI, classification);
            if (!summaryStr.empty()) summaryStr += " | ";
            summaryStr += buf;
        }

        std::snprintf(summary, sizeof(summary), "LLNA | %s", summaryStr.c_str());
    }

    m_result.summary = summary;
}

} // namespace animsim
