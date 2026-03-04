#pragma once

#include "simulation/ExperimentBase.h"

namespace animsim {

class SkinIrritationExperiment : public ExperimentBase {
public:
    explicit SkinIrritationExperiment(const ExperimentConfig& config);

    void setup() override;
    bool step(float timeHours, float dt) override;
    void computeResults() override;

private:
    void applyTestSubstance();
    void scoreSkin(float timeHours);

    // Per-animal skin scoring
    struct SkinScore {
        float erythema = 0.0f;    // 0-4 scale
        float edema = 0.0f;       // 0-4 scale
        float eschar = 0.0f;      // 0-4 scale
        float total() const { return erythema + edema + eschar; }
    };

    struct AnimalSkinData {
        std::vector<std::pair<float, SkinScore>> scores; // time, score pairs
        float lymphNodeWeight = 0.0f;  // for LLNA
        float stimulationIndex = 0.0f; // LLNA SI
    };

    std::vector<AnimalSkinData> m_skinData;
    bool m_substanceApplied = false;
    float m_nextScoringTime = 0.0f;
    float m_scoringInterval = 24.0f; // score every 24 hours
};

} // namespace animsim
