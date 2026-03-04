#pragma once

#include "simulation/Types.h"
#include "simulation/Animal.h"
#include "simulation/ExperimentBase.h"
#include "simulation/RandomEngine.h"
#include <vector>
#include <memory>
#include <string>

namespace animsim {

struct ProcedureEvent {
    float timeHours;
    ProcedureType procedure;
    int animalIndex;
    std::string bodyPart;
    std::string description;
    float value = 0.0f;  // e.g., dose amount, temperature reading
};

class InteractiveController {
public:
    InteractiveController();

    // Setup for an experiment type
    void setup(const ExperimentConfig& config);

    // Time control
    void advanceTime(float hours);
    void setTime(float hours);
    float getCurrentTime() const { return m_currentTime; }
    float getDuration() const { return m_config.durationHours; }
    float getProgress() const;
    bool isComplete() const { return m_currentTime >= m_config.durationHours; }

    // Perform a procedure on an animal
    bool performProcedure(ProcedureType proc, int animalIndex,
                          const std::string& bodyPart, float doseAmount = 0.0f);

    // Get available tools for current experiment type
    std::vector<ProcedureType> getAvailableTools() const;

    // Access
    const ExperimentConfig& getConfig() const { return m_config; }
    const std::vector<std::unique_ptr<Animal>>& getAnimals() const { return m_animals; }
    int getAnimalCount() const { return static_cast<int>(m_animals.size()); }
    const Animal& getAnimal(int idx) const { return *m_animals[idx]; }

    const std::vector<ProcedureEvent>& getEventLog() const { return m_eventLog; }
    const std::vector<std::string>& getGroupLabels() const { return m_groupLabels; }

    // Compute results when done
    ExperimentResult computeResults() const;

    // Delta tracking for real-time vital display after procedures
    const AnimalDetailedState& getPreProcState() const { return m_preProcState; }
    int getLastProcAnimalIdx() const { return m_lastProcAnimalIdx; }
    bool hasProcDelta() const { return m_hasProcDelta; }
    void clearProcDelta() { m_hasProcDelta = false; }

    // Vital history for real-time graphs (per selected animal)
    const std::vector<VitalSnapshot>& getVitalHistory(int animalIdx) const;

private:
    void updateAnimals(float dt);
    void recordVitalSnapshot(int animalIdx);

    ExperimentConfig m_config;
    std::vector<std::unique_ptr<Animal>> m_animals;
    std::vector<std::string> m_groupLabels;
    std::vector<ProcedureEvent> m_eventLog;
    std::vector<TimePointSnapshot> m_snapshots;
    RandomEngine m_rng;
    float m_currentTime = 0.0f;

    // Pre-procedure state for delta display
    AnimalDetailedState m_preProcState;
    int m_lastProcAnimalIdx = -1;
    bool m_hasProcDelta = false;

    // Vital history per animal (index -> history)
    std::unordered_map<int, std::vector<VitalSnapshot>> m_vitalHistory;
    static constexpr int MAX_VITAL_HISTORY = 500;
};

} // namespace animsim
