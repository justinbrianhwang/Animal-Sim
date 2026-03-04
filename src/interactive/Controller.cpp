#include "interactive/Controller.h"
#include "simulation/AnimalRegistry.h"
#include "simulation/DrugRegistry.h"
#include <cstdio>
#include <algorithm>

namespace animsim {

InteractiveController::InteractiveController() {}

void InteractiveController::setup(const ExperimentConfig& config) {
    m_config = config;
    m_currentTime = 0.0f;
    m_eventLog.clear();
    m_snapshots.clear();
    m_animals.clear();
    m_vitalHistory.clear();

    // Create populations
    int animalId = 1;
    for (int g = 0; g < config.numGroups; g++) {
        for (int a = 0; a < config.animalsPerGroup; a++) {
            m_animals.push_back(std::make_unique<Animal>(
                animalId++, config.species, g, m_rng));
        }
    }

    // Group labels
    m_groupLabels.clear();
    for (int g = 0; g < config.numGroups; g++) {
        char buf[32];
        std::snprintf(buf, sizeof(buf), "Group %d", g + 1);
        m_groupLabels.push_back(buf);
    }

    // Initial snapshot
    TimePointSnapshot snap;
    snap.timeHours = 0.0f;
    for (auto& a : m_animals) {
        snap.animalStates.push_back(a->getState());
    }
    m_snapshots.push_back(std::move(snap));

    // Log
    ProcedureEvent evt;
    evt.timeHours = 0.0f;
    evt.procedure = ProcedureType::Observe;
    evt.animalIndex = -1;
    evt.description = "Experiment initialized";
    m_eventLog.push_back(evt);
}

void InteractiveController::advanceTime(float hours) {
    float targetTime = m_currentTime + hours;
    float dt = m_config.timeStepHours;
    while (m_currentTime < targetTime && m_currentTime < m_config.durationHours) {
        float step = std::min(dt, targetTime - m_currentTime);
        m_currentTime += step;
        updateAnimals(step);
    }

    m_hasProcDelta = false; // Clear delta display when time advances

    // Record vital snapshots for graphing
    for (int i = 0; i < static_cast<int>(m_animals.size()); i++) {
        recordVitalSnapshot(i);
    }

    // Take snapshot
    TimePointSnapshot snap;
    snap.timeHours = m_currentTime;
    for (auto& a : m_animals) {
        snap.animalStates.push_back(a->getState());
    }
    m_snapshots.push_back(std::move(snap));
}

void InteractiveController::setTime(float hours) {
    m_currentTime = std::clamp(hours, 0.0f, m_config.durationHours);
}

float InteractiveController::getProgress() const {
    if (m_config.durationHours <= 0.0f) return 1.0f;
    return std::min(1.0f, m_currentTime / m_config.durationHours);
}

bool InteractiveController::performProcedure(ProcedureType proc, int animalIndex,
                                              const std::string& bodyPart, float doseAmount) {
    if (animalIndex < 0 || animalIndex >= static_cast<int>(m_animals.size()))
        return false;

    auto& animal = *m_animals[animalIndex];
    if (!animal.isAlive() && proc != ProcedureType::Necropsy)
        return false;

    // Store pre-procedure state for delta display
    m_preProcState = animal.getState();
    m_lastProcAnimalIdx = animalIndex;

    ProcedureEvent evt;
    evt.timeHours = m_currentTime;
    evt.procedure = proc;
    evt.animalIndex = animalIndex;
    evt.bodyPart = bodyPart;

    char desc[128];

    const std::string& drugId = m_config.drugId;
    auto& drugInfo = DrugRegistry::instance().getDrug(drugId);

    switch (proc) {
    case ProcedureType::OralGavage:
        animal.administerDose(doseAmount > 0 ? doseAmount : 50.0f, "oral", drugId);
        std::snprintf(desc, sizeof(desc), "Oral gavage: %.1f mg/kg %s to %s",
                      doseAmount > 0 ? doseAmount : 50.0f, drugInfo.name.c_str(), animal.getLabel().c_str());
        break;

    case ProcedureType::IVInjection:
        animal.administerDose(doseAmount > 0 ? doseAmount : 50.0f, "iv", drugId);
        std::snprintf(desc, sizeof(desc), "IV injection: %.1f mg/kg %s to %s",
                      doseAmount > 0 ? doseAmount : 50.0f, drugInfo.name.c_str(), animal.getLabel().c_str());
        break;

    case ProcedureType::IPInjection:
        animal.administerDose(doseAmount > 0 ? doseAmount : 50.0f, "ip", drugId);
        std::snprintf(desc, sizeof(desc), "IP injection: %.1f mg/kg %s to %s",
                      doseAmount > 0 ? doseAmount : 50.0f, drugInfo.name.c_str(), animal.getLabel().c_str());
        break;

    case ProcedureType::SCInjection:
        animal.administerDose(doseAmount > 0 ? doseAmount : 50.0f, "sc", drugId);
        std::snprintf(desc, sizeof(desc), "SC injection: %.1f mg/kg %s to %s",
                      doseAmount > 0 ? doseAmount : 50.0f, drugInfo.name.c_str(), animal.getLabel().c_str());
        break;

    case ProcedureType::DermalApplication:
        animal.administerDose(doseAmount > 0 ? doseAmount : 50.0f, "dermal", drugId);
        std::snprintf(desc, sizeof(desc), "Dermal application: %.1f mg/kg %s to %s",
                      doseAmount > 0 ? doseAmount : 50.0f, drugInfo.name.c_str(), animal.getLabel().c_str());
        break;

    case ProcedureType::BloodSample:
        evt.value = animal.getState().drugConcentration;
        std::snprintf(desc, sizeof(desc), "Blood sample from %s: drug conc = %.2f mg/L",
                      animal.getLabel().c_str(), evt.value);
        break;

    case ProcedureType::Weigh:
        evt.value = animal.getState().vitals.weight;
        std::snprintf(desc, sizeof(desc), "Weighed %s: %.3f kg",
                      animal.getLabel().c_str(), evt.value);
        break;

    case ProcedureType::Temperature:
        evt.value = animal.getState().vitals.temperature;
        std::snprintf(desc, sizeof(desc), "Temperature %s: %.1f C",
                      animal.getLabel().c_str(), evt.value);
        break;

    case ProcedureType::Observe:
        std::snprintf(desc, sizeof(desc), "Observed %s: %s, health %.0f%%",
                      animal.getLabel().c_str(),
                      animalStatusToString(animal.getState().status),
                      animal.getState().overallHealth);
        break;

    case ProcedureType::Euthanize:
        animal.euthanize(m_currentTime);
        std::snprintf(desc, sizeof(desc), "Euthanized %s at t=%.1fh",
                      animal.getLabel().c_str(), m_currentTime);
        break;

    case ProcedureType::Necropsy:
        std::snprintf(desc, sizeof(desc), "Necropsy on %s: organs examined",
                      animal.getLabel().c_str());
        break;

    case ProcedureType::BehavioralTest:
        evt.value = m_rng.normal(50.0f, 15.0f); // behavioral score
        std::snprintf(desc, sizeof(desc), "Behavioral test on %s: score %.1f",
                      animal.getLabel().c_str(), evt.value);
        break;

    case ProcedureType::SkinScoring:
        evt.value = m_rng.uniform(0.0f, 4.0f); // Draize score
        std::snprintf(desc, sizeof(desc), "Skin scoring on %s: Draize score %.1f",
                      animal.getLabel().c_str(), evt.value);
        break;

    case ProcedureType::SalineFlush:
        animal.applySalineFlush();
        std::snprintf(desc, sizeof(desc), "Saline flush on %s: drug conc -30%%",
                      animal.getLabel().c_str());
        break;

    case ProcedureType::ActivatedCharcoal:
        animal.applyActivatedCharcoal();
        std::snprintf(desc, sizeof(desc), "Activated charcoal on %s: drug conc -50%%",
                      animal.getLabel().c_str());
        break;

    case ProcedureType::Antidote:
        animal.applyAntidote(drugId);
        std::snprintf(desc, sizeof(desc), "Antidote for %s on %s: drug conc -90%%",
                      drugInfo.name.c_str(), animal.getLabel().c_str());
        break;
    }

    // Immediate physiology update for dose procedures so vitals change right away
    {
        bool isDoseProc = (proc == ProcedureType::OralGavage || proc == ProcedureType::IVInjection ||
                           proc == ProcedureType::IPInjection || proc == ProcedureType::SCInjection ||
                           proc == ProcedureType::DermalApplication ||
                           proc == ProcedureType::SalineFlush || proc == ProcedureType::ActivatedCharcoal ||
                           proc == ProcedureType::Antidote);
        if (isDoseProc && animal.isAlive()) {
            animal.updatePhysiology(0.05f, m_rng);
            float drugConc = animal.getState().drugConcentration;
            if (drugConc > 10.0f) {
                animal.applyToxicDamage((drugConc / 100.0f) * 0.3f, m_rng);
            }
            animal.checkMortality(m_rng);
            if (animal.getState().status == AnimalStatus::Dead &&
                animal.getState().timeOfDeath < 0.0f) {
                animal.getState().timeOfDeath = m_currentTime;
            }
        }
    }
    m_hasProcDelta = true;

    // Record vital snapshot after procedure
    recordVitalSnapshot(animalIndex);

    evt.description = desc;
    m_eventLog.push_back(evt);
    return true;
}

std::vector<ProcedureType> InteractiveController::getAvailableTools() const {
    std::vector<ProcedureType> tools;

    switch (m_config.type) {
    case ExperimentType::Toxicology:
        tools = {ProcedureType::OralGavage, ProcedureType::IVInjection,
                 ProcedureType::IPInjection, ProcedureType::SCInjection,
                 ProcedureType::BloodSample, ProcedureType::Weigh,
                 ProcedureType::Temperature, ProcedureType::Observe,
                 ProcedureType::Euthanize, ProcedureType::Necropsy,
                 ProcedureType::SalineFlush, ProcedureType::ActivatedCharcoal,
                 ProcedureType::Antidote};
        break;
    case ExperimentType::Pharmacokinetics:
        tools = {ProcedureType::OralGavage, ProcedureType::IVInjection,
                 ProcedureType::IPInjection, ProcedureType::SCInjection,
                 ProcedureType::BloodSample, ProcedureType::Weigh,
                 ProcedureType::Observe,
                 ProcedureType::SalineFlush, ProcedureType::Antidote};
        break;
    case ExperimentType::Behavioral:
        tools = {ProcedureType::OralGavage, ProcedureType::IPInjection,
                 ProcedureType::BehavioralTest, ProcedureType::Weigh,
                 ProcedureType::Observe};
        break;
    case ExperimentType::DrugEfficacy:
        tools = {ProcedureType::OralGavage, ProcedureType::IVInjection,
                 ProcedureType::IPInjection, ProcedureType::SCInjection,
                 ProcedureType::BloodSample, ProcedureType::Weigh,
                 ProcedureType::Temperature, ProcedureType::Observe,
                 ProcedureType::Euthanize,
                 ProcedureType::SalineFlush, ProcedureType::ActivatedCharcoal,
                 ProcedureType::Antidote};
        break;
    case ExperimentType::SkinIrritation:
        tools = {ProcedureType::DermalApplication, ProcedureType::SkinScoring,
                 ProcedureType::Weigh, ProcedureType::Observe};
        break;
    }

    return tools;
}

void InteractiveController::updateAnimals(float dt) {
    for (auto& animal : m_animals) {
        if (!animal->isAlive()) continue;
        animal->updatePhysiology(dt, m_rng);
        // Apply toxic damage based on drug concentration over time
        float drugConc = animal->getState().drugConcentration;
        if (drugConc > 5.0f) {
            float severity = (drugConc / 100.0f) * dt;
            animal->applyToxicDamage(severity, m_rng);
        }
        animal->checkMortality(m_rng);
        if (animal->getState().status == AnimalStatus::Dead &&
            animal->getState().timeOfDeath < 0.0f) {
            animal->getState().timeOfDeath = m_currentTime;
        }
    }
}

ExperimentResult InteractiveController::computeResults() const {
    ExperimentResult result;
    result.type = m_config.type;
    result.completed = true;
    result.snapshots = m_snapshots;

    int totalAnimals = static_cast<int>(m_animals.size());
    int totalDeaths = 0;
    for (auto& a : m_animals) {
        if (!a->isAlive()) totalDeaths++;
    }

    char summary[256];
    std::snprintf(summary, sizeof(summary),
                  "Interactive experiment complete. %d procedures performed. "
                  "Animals: %d total, %d survived, %d died.",
                  static_cast<int>(m_eventLog.size()), totalAnimals,
                  totalAnimals - totalDeaths, totalDeaths);
    result.summary = summary;

    return result;
}

void InteractiveController::recordVitalSnapshot(int animalIdx) {
    if (animalIdx < 0 || animalIdx >= static_cast<int>(m_animals.size())) return;
    auto& state = m_animals[animalIdx]->getState();

    VitalSnapshot vs;
    vs.time = m_currentTime;
    vs.heartRate = state.vitals.heartRate;
    vs.temperature = state.vitals.temperature;
    vs.respRate = state.vitals.respiratoryRate;
    vs.spo2 = state.vitals.oxygenSaturation;
    vs.weight = state.vitals.weight;
    vs.totalDrugConc = state.drugConcentration;

    auto& history = m_vitalHistory[animalIdx];
    history.push_back(vs);
    if (static_cast<int>(history.size()) > MAX_VITAL_HISTORY) {
        history.erase(history.begin());
    }
}

static const std::vector<VitalSnapshot> s_emptyHistory;

const std::vector<VitalSnapshot>& InteractiveController::getVitalHistory(int animalIdx) const {
    auto it = m_vitalHistory.find(animalIdx);
    if (it != m_vitalHistory.end()) return it->second;
    return s_emptyHistory;
}

} // namespace animsim
