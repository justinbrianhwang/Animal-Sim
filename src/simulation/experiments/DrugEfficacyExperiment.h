#pragma once

#include "simulation/ExperimentBase.h"

namespace animsim {

class DrugEfficacyExperiment : public ExperimentBase {
public:
    explicit DrugEfficacyExperiment(const ExperimentConfig& config);

    void setup() override;
    bool step(float timeHours, float dt) override;
    void computeResults() override;

private:
    void administerDoses(float timeHours);

    // Disease model state per animal
    struct DiseaseState {
        float diseaseSeverity = 0.0f;   // 0-100 scale
        float tumorVolume = 0.0f;       // mm^3 for tumor model
        float infectionLoad = 0.0f;     // arbitrary units for infection
        float inflammationScore = 0.0f; // 0-10 for anti-inflammatory
    };

    std::vector<DiseaseState> m_diseaseStates;
    bool m_dosesAdministered = false;
    float m_lastDoseTime = 0.0f;
    float m_dosingInterval = 24.0f; // daily dosing
};

} // namespace animsim
