#pragma once

#include "simulation/Types.h"
#include "simulation/Animal.h"
#include "simulation/RandomEngine.h"
#include <vector>
#include <memory>
#include <string>

namespace animsim {

class ExperimentBase {
public:
    ExperimentBase(const ExperimentConfig& config);
    virtual ~ExperimentBase() = default;

    // Setup the experiment (create animals, set parameters)
    virtual void setup();

    // Run one time step (returns false when experiment is done)
    virtual bool step(float timeHours, float dt);

    // Compute final results
    virtual void computeResults();

    // Access
    const ExperimentConfig& getConfig() const { return m_config; }
    const std::vector<std::unique_ptr<Animal>>& getAnimals() const { return m_animals; }
    const std::vector<TimePointSnapshot>& getSnapshots() const { return m_snapshots; }
    const std::vector<std::string>& getGroupLabels() const { return m_groupLabels; }
    const ExperimentResult& getResult() const { return m_result; }

    float getCurrentTime() const { return m_currentTime; }
    float getProgress() const;
    bool isComplete() const { return m_complete; }

    // Take a snapshot of current state
    void takeSnapshot(float timeHours);

    // Get animals in a specific group
    std::vector<Animal*> getGroupAnimals(int groupIndex);
    std::vector<const Animal*> getGroupAnimals(int groupIndex) const;
    int getAliveCount() const;
    int getDeadCount() const;

protected:
    void createPopulations();

    ExperimentConfig m_config;
    std::vector<std::unique_ptr<Animal>> m_animals;
    std::vector<TimePointSnapshot> m_snapshots;
    std::vector<std::string> m_groupLabels;
    ExperimentResult m_result;
    RandomEngine m_rng;
    float m_currentTime = 0.0f;
    bool m_complete = false;
};

} // namespace animsim
