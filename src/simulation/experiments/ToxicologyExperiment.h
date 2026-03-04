#pragma once

#include "simulation/ExperimentBase.h"

namespace animsim {

class ToxicologyExperiment : public ExperimentBase {
public:
    explicit ToxicologyExperiment(const ExperimentConfig& config);

    void setup() override;
    bool step(float timeHours, float dt) override;
    void computeResults() override;

private:
    void administerDoses(float timeHours);
    float computeLD50() const;

    bool m_dosesAdministered = false;
    float m_doseTime = 0.0f;
};

} // namespace animsim
