#pragma once

#include "simulation/Types.h"
#include "simulation/ExperimentBase.h"
#include <memory>
#include <thread>
#include <mutex>
#include <atomic>
#include <functional>

namespace animsim {

enum class SimState {
    Idle,
    Running,
    Paused,
    Complete,
    Error
};

class SimulationEngine {
public:
    SimulationEngine();
    ~SimulationEngine();

    // Start a new experiment (takes ownership)
    void start(std::unique_ptr<ExperimentBase> experiment);

    // Controls
    void pause();
    void resume();
    void cancel();

    // State queries (thread-safe)
    SimState getState() const { return m_state.load(); }
    float getProgress() const;
    float getCurrentTime() const;

    // Get a copy of the latest snapshot (thread-safe)
    bool getLatestSnapshot(TimePointSnapshot& out) const;

    // Get the experiment result (only valid after Complete)
    const ExperimentResult& getResult() const;

    // Get all animals for display (lock held briefly)
    struct AnimalInfo {
        int id;
        std::string label;
        int groupIndex;
        float health;
        AnimalStatus status;
        VitalSigns vitals;
        float drugConcentration;
    };
    std::vector<AnimalInfo> getAnimalInfos() const;
    std::vector<std::string> getGroupLabels() const;

    // Speed control
    void setSpeedMultiplier(float mult) { m_speedMultiplier = mult; }
    float getSpeedMultiplier() const { return m_speedMultiplier; }

private:
    void runThread();

    std::unique_ptr<ExperimentBase> m_experiment;
    std::thread m_thread;
    mutable std::mutex m_mutex;

    std::atomic<SimState> m_state{SimState::Idle};
    std::atomic<bool> m_cancelRequested{false};
    std::atomic<bool> m_pauseRequested{false};
    std::atomic<float> m_progress{0.0f};
    std::atomic<float> m_currentTime{0.0f};
    float m_speedMultiplier = 1.0f;

    TimePointSnapshot m_latestSnapshot;
    ExperimentResult m_result;
};

} // namespace animsim
