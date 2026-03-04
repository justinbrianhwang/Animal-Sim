#include "simulation/ExperimentBase.h"
#include "simulation/AnimalRegistry.h"

namespace animsim {

ExperimentBase::ExperimentBase(const ExperimentConfig& config)
    : m_config(config)
{
    m_result.type = config.type;
}

void ExperimentBase::setup() {
    createPopulations();
    takeSnapshot(0.0f);
}

bool ExperimentBase::step(float timeHours, float dt) {
    m_currentTime = timeHours;

    for (auto& animal : m_animals) {
        animal->updatePhysiology(dt, m_rng);
        animal->checkMortality(m_rng);
        if (animal->getState().status == AnimalStatus::Dead &&
            animal->getState().timeOfDeath < 0.0f) {
            // Set death time if not already set
            animal->getState().timeOfDeath = timeHours;
        }
    }

    takeSnapshot(timeHours);

    if (timeHours >= m_config.durationHours) {
        m_complete = true;
        return false;
    }
    return true;
}

void ExperimentBase::computeResults() {
    m_result.completed = true;
    m_result.snapshots = m_snapshots;
}

void ExperimentBase::createPopulations() {
    m_animals.clear();
    int animalId = 1;

    for (int g = 0; g < m_config.numGroups; g++) {
        for (int a = 0; a < m_config.animalsPerGroup; a++) {
            m_animals.push_back(std::make_unique<Animal>(
                animalId++, m_config.species, g, m_rng));
        }
    }
}

void ExperimentBase::takeSnapshot(float timeHours) {
    TimePointSnapshot snap;
    snap.timeHours = timeHours;
    snap.animalStates.reserve(m_animals.size());

    for (auto& animal : m_animals) {
        snap.animalStates.push_back(animal->getState());
    }

    m_snapshots.push_back(std::move(snap));
}

float ExperimentBase::getProgress() const {
    if (m_config.durationHours <= 0.0f) return 1.0f;
    return std::min(1.0f, m_currentTime / m_config.durationHours);
}

std::vector<Animal*> ExperimentBase::getGroupAnimals(int groupIndex) {
    std::vector<Animal*> result;
    for (auto& a : m_animals) {
        if (a->getGroupIndex() == groupIndex) {
            result.push_back(a.get());
        }
    }
    return result;
}

std::vector<const Animal*> ExperimentBase::getGroupAnimals(int groupIndex) const {
    std::vector<const Animal*> result;
    for (auto& a : m_animals) {
        if (a->getGroupIndex() == groupIndex) {
            result.push_back(a.get());
        }
    }
    return result;
}

int ExperimentBase::getAliveCount() const {
    int count = 0;
    for (auto& a : m_animals) {
        if (a->isAlive()) count++;
    }
    return count;
}

int ExperimentBase::getDeadCount() const {
    int count = 0;
    for (auto& a : m_animals) {
        if (!a->isAlive()) count++;
    }
    return count;
}

} // namespace animsim
