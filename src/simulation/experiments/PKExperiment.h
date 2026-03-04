#pragma once

#include "simulation/ExperimentBase.h"

namespace animsim {

class PKExperiment : public ExperimentBase {
public:
    explicit PKExperiment(const ExperimentConfig& config);

    void setup() override;
    bool step(float timeHours, float dt) override;
    void computeResults() override;

private:
    void administerDose(float timeHours);
    void computePKMetrics();

    bool m_doseAdministered = false;

    // Per-animal concentration tracking for PK curves
    struct PKProfile {
        std::vector<float> times;
        std::vector<float> concentrations;
        float cmax = 0.0f;
        float tmax = 0.0f;
    };
    std::vector<PKProfile> m_profiles;
};

} // namespace animsim
