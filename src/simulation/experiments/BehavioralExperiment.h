#pragma once

#include "simulation/ExperimentBase.h"
#include <unordered_map>

namespace animsim {

struct BehavioralScore {
    float latency = 0.0f;      // time to complete task (seconds)
    float distance = 0.0f;     // distance traveled
    float errorCount = 0.0f;   // number of errors
    float freezingTime = 0.0f; // fear conditioning freezing %
    float socialScore = 0.0f;  // social interaction score
    float learningIndex = 0.0f; // improvement over trials
};

class BehavioralExperiment : public ExperimentBase {
public:
    explicit BehavioralExperiment(const ExperimentConfig& config);

    void setup() override;
    bool step(float timeHours, float dt) override;
    void computeResults() override;

private:
    void runBehavioralTrial(float timeHours);

    // Per-animal behavioral scores over trials
    std::vector<std::vector<BehavioralScore>> m_trialScores;
    float m_nextTrialTime = 0.0f;
    float m_trialInterval = 4.0f; // hours between trials
    int m_trialCount = 0;
};

} // namespace animsim
