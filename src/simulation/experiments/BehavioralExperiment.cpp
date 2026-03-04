#include "simulation/experiments/BehavioralExperiment.h"
#include "simulation/AnimalRegistry.h"
#include <algorithm>
#include <cmath>
#include <cstdio>

namespace animsim {

BehavioralExperiment::BehavioralExperiment(const ExperimentConfig& config)
    : ExperimentBase(config) {}

void BehavioralExperiment::setup() {
    // Dose levels for drug groups
    if (m_config.doselevels.empty()) {
        for (int g = 0; g < m_config.numGroups; g++) {
            if (g == 0) {
                m_config.doselevels.push_back(0.0f); // control/vehicle
            } else {
                float frac = static_cast<float>(g) / (m_config.numGroups - 1);
                m_config.doselevels.push_back(5.0f + 20.0f * frac);
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

    if (m_config.durationHours <= 0.0f) m_config.durationHours = 48.0f;

    ExperimentBase::setup();

    // Initialize trial scores
    m_trialScores.resize(m_animals.size());
    m_nextTrialTime = 1.0f; // first trial at 1 hour (after drug admin)

    // Administer drug doses at t=0
    for (int g = 0; g < m_config.numGroups; g++) {
        float dose = (g < static_cast<int>(m_config.doselevels.size()))
                     ? m_config.doselevels[g] : 0.0f;
        auto groupAnimals = getGroupAnimals(g);
        for (auto* animal : groupAnimals) {
            animal->administerDose(dose, m_config.route);
        }
    }
}

bool BehavioralExperiment::step(float timeHours, float dt) {
    m_currentTime = timeHours;

    // Run behavioral trials at intervals
    if (timeHours >= m_nextTrialTime) {
        runBehavioralTrial(timeHours);
        m_nextTrialTime = timeHours + m_trialInterval;
        m_trialCount++;
    }

    // Update animal physiology
    for (auto& animal : m_animals) {
        if (!animal->isAlive()) continue;
        animal->updatePhysiology(dt, m_rng);
        animal->checkMortality(m_rng);
    }

    takeSnapshot(timeHours);

    if (timeHours >= m_config.durationHours) {
        m_complete = true;
        return false;
    }
    return true;
}

void BehavioralExperiment::runBehavioralTrial(float timeHours) {
    for (size_t i = 0; i < m_animals.size(); i++) {
        if (!m_animals[i]->isAlive()) continue;

        BehavioralScore score;
        float drugEffect = m_animals[i]->getState().drugConcentration / 20.0f;
        drugEffect = std::min(drugEffect, 2.0f);
        int trial = m_trialCount;

        if (m_config.paradigm == "morris_water_maze") {
            // Learning curve: latency decreases with trials, drug may impair or enhance
            float baseLat = 60.0f * std::exp(-0.15f * trial); // natural learning
            float drugMod = 1.0f + 0.5f * drugEffect; // drug impairment
            score.latency = std::max(5.0f, baseLat * drugMod + m_rng.normal(0, 5.0f));
            score.distance = score.latency * m_rng.uniform(0.8f, 1.5f); // swim distance
            score.errorCount = std::max(0.0f, (float)(int)(score.latency / 15.0f + m_rng.normal(0, 1)));

        } else if (m_config.paradigm == "t_maze") {
            // Alternation rate
            float baseRate = 0.5f + 0.3f * (1.0f - std::exp(-0.2f * trial));
            float drugMod = -0.15f * drugEffect;
            score.learningIndex = std::clamp(baseRate + drugMod + m_rng.normal(0, 0.1f), 0.0f, 1.0f);
            score.latency = 30.0f + 20.0f * drugEffect + m_rng.normal(0, 5.0f);

        } else if (m_config.paradigm == "fear_conditioning") {
            // Freezing response
            float baseFreezing = 20.0f + 40.0f * (1.0f - std::exp(-0.3f * trial));
            float drugMod = -15.0f * drugEffect; // anxiolytic effect
            score.freezingTime = std::clamp(baseFreezing + drugMod + m_rng.normal(0, 8.0f), 0.0f, 100.0f);
            score.latency = 10.0f + m_rng.normal(0, 3.0f);

        } else if (m_config.paradigm == "social_interaction") {
            // Social interaction time
            float baseSocial = 50.0f;
            float drugMod = -10.0f * drugEffect;
            score.socialScore = std::max(0.0f, baseSocial + drugMod + m_rng.normal(0, 10.0f));
            score.latency = 5.0f + m_rng.normal(0, 2.0f);

        } else {
            // Default: open field
            score.distance = 200.0f - 30.0f * drugEffect + m_rng.normal(0, 20.0f);
            score.latency = 300.0f; // fixed observation time
            score.freezingTime = std::max(0.0f, 10.0f + 20.0f * drugEffect + m_rng.normal(0, 5.0f));
        }

        // Learning index across trials
        if (m_trialScores[i].size() >= 2) {
            auto& prev = m_trialScores[i].back();
            if (prev.latency > 0.0f) {
                score.learningIndex = (prev.latency - score.latency) / prev.latency;
            }
        }

        m_trialScores[i].push_back(score);
    }
}

void BehavioralExperiment::computeResults() {
    ExperimentBase::computeResults();

    // Compute average behavioral metrics per group
    std::string summaryStr;
    char buf[256];

    for (int g = 0; g < m_config.numGroups; g++) {
        auto group = getGroupAnimals(g);
        float avgLatency = 0.0f, avgDistance = 0.0f, avgFreezing = 0.0f;
        int count = 0;

        for (auto* a : group) {
            int idx = 0;
            for (size_t i = 0; i < m_animals.size(); i++) {
                if (m_animals[i].get() == a) { idx = static_cast<int>(i); break; }
            }
            if (!m_trialScores[idx].empty()) {
                auto& last = m_trialScores[idx].back();
                avgLatency += last.latency;
                avgDistance += last.distance;
                avgFreezing += last.freezingTime;
                count++;
            }
        }

        if (count > 0) {
            avgLatency /= count;
            avgDistance /= count;
            avgFreezing /= count;
        }

        std::snprintf(buf, sizeof(buf), "%s: Lat=%.1fs, Dist=%.0f, Freeze=%.0f%% | ",
                      m_groupLabels[g].c_str(), avgLatency, avgDistance, avgFreezing);
        summaryStr += buf;
    }

    std::snprintf(buf, sizeof(buf), "Paradigm: %s, %d trials completed",
                  m_config.paradigm.c_str(), m_trialCount);
    m_result.summary = std::string(buf) + " | " + summaryStr;
}

} // namespace animsim
