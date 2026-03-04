#include "simulation/SimulationEngine.h"
#include <chrono>

namespace animsim {

SimulationEngine::SimulationEngine() {}

SimulationEngine::~SimulationEngine() {
    cancel();
    if (m_thread.joinable()) {
        m_thread.join();
    }
}

void SimulationEngine::start(std::unique_ptr<ExperimentBase> experiment) {
    // Cancel any running experiment
    cancel();
    if (m_thread.joinable()) {
        m_thread.join();
    }

    m_experiment = std::move(experiment);
    m_cancelRequested = false;
    m_pauseRequested = false;
    m_progress = 0.0f;
    m_currentTime = 0.0f;
    m_state = SimState::Running;

    // Setup the experiment
    m_experiment->setup();

    // Start background thread
    m_thread = std::thread(&SimulationEngine::runThread, this);
}

void SimulationEngine::pause() {
    if (m_state == SimState::Running) {
        m_pauseRequested = true;
        m_state = SimState::Paused;
    }
}

void SimulationEngine::resume() {
    if (m_state == SimState::Paused) {
        m_pauseRequested = false;
        m_state = SimState::Running;
    }
}

void SimulationEngine::cancel() {
    m_cancelRequested = true;
    m_pauseRequested = false; // unpause so thread can exit
    if (m_thread.joinable()) {
        m_thread.join();
    }
    m_state = SimState::Idle;
}

float SimulationEngine::getProgress() const {
    return m_progress.load();
}

float SimulationEngine::getCurrentTime() const {
    return m_currentTime.load();
}

bool SimulationEngine::getLatestSnapshot(TimePointSnapshot& out) const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (m_latestSnapshot.animalStates.empty()) return false;
    out = m_latestSnapshot;
    return true;
}

const ExperimentResult& SimulationEngine::getResult() const {
    return m_result;
}

std::vector<SimulationEngine::AnimalInfo> SimulationEngine::getAnimalInfos() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    std::vector<AnimalInfo> infos;
    if (!m_experiment) return infos;

    for (auto& animal : m_experiment->getAnimals()) {
        AnimalInfo info;
        info.id = animal->getId();
        info.label = animal->getLabel();
        info.groupIndex = animal->getGroupIndex();
        info.health = animal->getState().overallHealth;
        info.status = animal->getState().status;
        info.vitals = animal->getState().vitals;
        info.drugConcentration = animal->getState().drugConcentration;
        infos.push_back(info);
    }
    return infos;
}

std::vector<std::string> SimulationEngine::getGroupLabels() const {
    std::lock_guard<std::mutex> lock(m_mutex);
    if (!m_experiment) return {};
    return m_experiment->getGroupLabels();
}

void SimulationEngine::runThread() {
    float timeStep = m_experiment->getConfig().timeStepHours;
    float duration = m_experiment->getConfig().durationHours;
    float time = 0.0f;

    while (!m_cancelRequested && time <= duration) {
        // Handle pause
        while (m_pauseRequested && !m_cancelRequested) {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
        if (m_cancelRequested) break;

        // Run one step
        {
            std::lock_guard<std::mutex> lock(m_mutex);
            bool continueRunning = m_experiment->step(time, timeStep);
            if (!continueRunning) {
                m_experiment->computeResults();
                m_result = m_experiment->getResult();
                m_progress = 1.0f;
                m_currentTime = time;
                m_state = SimState::Complete;
                return;
            }

            // Update latest snapshot
            auto& snaps = m_experiment->getSnapshots();
            if (!snaps.empty()) {
                m_latestSnapshot = snaps.back();
            }
        }

        time += timeStep;
        m_currentTime = time;
        m_progress = std::min(1.0f, time / duration);

        // Simulate at controlled speed (not max CPU)
        float sleepMs = timeStep * 20.0f / m_speedMultiplier; // ~20ms per sim hour at 1x
        sleepMs = std::max(1.0f, std::min(100.0f, sleepMs));
        std::this_thread::sleep_for(std::chrono::milliseconds(static_cast<int>(sleepMs)));
    }

    if (!m_cancelRequested) {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_experiment->computeResults();
        m_result = m_experiment->getResult();
        m_state = SimState::Complete;
    }
}

} // namespace animsim
