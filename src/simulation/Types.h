#pragma once

#include <string>
#include <vector>
#include <unordered_map>
#include <cstdint>

namespace animsim {

// ─── Vital Signs ───────────────────────────────────────────────────────────

struct VitalSigns {
    float heartRate = 360.0f;       // bpm
    float temperature = 37.0f;      // Celsius
    float respiratoryRate = 85.0f;  // breaths/min
    float bloodPressureSys = 120.0f;
    float bloodPressureDia = 80.0f;
    float oxygenSaturation = 98.5f; // %
    float weight = 0.3f;            // kg
};

// ─── Blood Chemistry ───────────────────────────────────────────────────────

struct BloodChemistry {
    float wbc = 8.0f;      // 10^3/uL
    float rbc = 8.5f;      // 10^6/uL
    float platelets = 800.0f; // 10^3/uL
    float hemoglobin = 15.0f; // g/dL
    float alt = 45.0f;     // U/L (liver)
    float ast = 120.0f;    // U/L (liver)
    float bun = 20.0f;     // mg/dL (kidney)
    float creatinine = 0.5f; // mg/dL (kidney)
    float glucose = 120.0f;  // mg/dL
    float albumin = 3.5f;    // g/dL
};

// ─── Organ Health ──────────────────────────────────────────────────────────

struct OrganHealth {
    float liver = 100.0f;
    float kidney = 100.0f;
    float heart = 100.0f;
    float lungs = 100.0f;
    float brain = 100.0f;
    float stomach = 100.0f;
    float intestines = 100.0f;
    float skin = 100.0f;
    float spleen = 100.0f;
    float bone_marrow = 100.0f;

    float getOverallHealth() const {
        return (liver + kidney + heart + lungs + brain +
                stomach + intestines + skin + spleen + bone_marrow) / 10.0f;
    }
};

// ─── Animal Status ─────────────────────────────────────────────────────────

enum class AnimalStatus {
    Alive,
    Moribund,
    Dead,
    Euthanized
};

inline const char* animalStatusToString(AnimalStatus s) {
    switch (s) {
    case AnimalStatus::Alive: return "Alive";
    case AnimalStatus::Moribund: return "Moribund";
    case AnimalStatus::Dead: return "Dead";
    case AnimalStatus::Euthanized: return "Euthanized";
    }
    return "Unknown";
}

// ─── Active Drug Instance ──────────────────────────────────────────────────

struct ActiveDrug {
    std::string drugId;
    float concentration = 0.0f;    // mg/L in plasma
    float cumulativeDose = 0.0f;   // mg/kg total administered
    float timeAdministered = 0.0f; // hours when first administered
};

// ─── Detailed Animal State ─────────────────────────────────────────────────

struct AnimalDetailedState {
    VitalSigns vitals;
    BloodChemistry blood;
    OrganHealth organs;
    AnimalStatus status = AnimalStatus::Alive;
    float overallHealth = 100.0f;
    float drugConcentration = 0.0f;   // total across all drugs (computed)
    float cumulativeDose = 0.0f;      // total across all drugs (computed)
    std::vector<ActiveDrug> activeDrugs; // per-drug tracking
    float timeOfDeath = -1.0f;        // hours, -1 means alive
};

// ─── Vital Snapshot (for real-time graphing) ──────────────────────────────

struct VitalSnapshot {
    float time = 0.0f;
    float heartRate = 0.0f;
    float temperature = 0.0f;
    float respRate = 0.0f;
    float spo2 = 0.0f;
    float weight = 0.0f;
    float totalDrugConc = 0.0f;
};

// ─── Snapshot per time point ───────────────────────────────────────────────

struct TimePointSnapshot {
    float timeHours = 0.0f;
    std::vector<AnimalDetailedState> animalStates; // indexed same as animal list
};

// ─── Species Info ──────────────────────────────────────────────────────────

enum class Species {
    Mouse,
    Rat,
    Rabbit,
    GuineaPig,
    Dog,
    Monkey,
    Count
};

inline const char* speciesToString(Species s) {
    switch (s) {
    case Species::Mouse: return "Mouse";
    case Species::Rat: return "Rat";
    case Species::Rabbit: return "Rabbit";
    case Species::GuineaPig: return "Guinea Pig";
    case Species::Dog: return "Dog";
    case Species::Monkey: return "Monkey";
    default: return "Unknown";
    }
}

inline Species speciesFromIndex(int idx) {
    if (idx >= 0 && idx < static_cast<int>(Species::Count))
        return static_cast<Species>(idx);
    return Species::Rat;
}

// ─── Species Physiology Parameters ─────────────────────────────────────────

struct SpeciesParams {
    Species species;
    const char* name;
    float weightMean;        // kg
    float weightStd;
    float heartRateMean;     // bpm
    float heartRateStd;
    float tempMean;          // Celsius
    float tempStd;
    float respRateMean;
    float respRateStd;
    float metabolicRate;     // relative to rat=1.0
    float ld50Sensitivity;   // dose multiplier
    float bloodVolumeMl;     // ml per kg
};

// ─── Experiment Configuration ──────────────────────────────────────────────

enum class ExperimentType {
    Toxicology,
    Pharmacokinetics,
    Behavioral,
    DrugEfficacy,
    SkinIrritation
};

inline const char* experimentTypeToString(ExperimentType t) {
    switch (t) {
    case ExperimentType::Toxicology: return "Toxicology";
    case ExperimentType::Pharmacokinetics: return "Pharmacokinetics";
    case ExperimentType::Behavioral: return "Behavioral";
    case ExperimentType::DrugEfficacy: return "Drug Efficacy";
    case ExperimentType::SkinIrritation: return "Skin Irritation";
    }
    return "Unknown";
}

struct ExperimentConfig {
    ExperimentType type = ExperimentType::Toxicology;
    Species species = Species::Rat;
    int numGroups = 3;
    int animalsPerGroup = 5;
    float durationHours = 72.0f;
    float timeStepHours = 1.0f;

    // Drug selection
    std::string drugId = "generic_toxicant";

    // Toxicology-specific
    std::string toxStudyType = "ld50";  // ld50, dose_response, chronic
    std::vector<float> doselevels;       // mg/kg per group
    std::string route = "oral";          // oral, iv, ip, sc, dermal

    // PK-specific
    int compartments = 1;
    float absorptionRate = 1.0f;
    float eliminationRate = 0.3f;
    float volumeOfDistribution = 0.5f;

    // Behavioral-specific
    std::string paradigm = "morris_water_maze";

    // Drug efficacy-specific
    std::string diseaseModel = "tumor";

    // Skin irritation-specific
    std::string skinTestType = "draize";
};

// ─── Experiment Results ────────────────────────────────────────────────────

struct ToxicologyResult {
    float ld50 = 0.0f;
    float ld50Lower = 0.0f;
    float ld50Upper = 0.0f;
    float hillCoefficient = 0.0f;
    int totalAnimals = 0;
    int totalDeaths = 0;
    float mortalityRate = 0.0f;
    std::vector<float> doseLevels;
    std::vector<float> mortalityByGroup;
    std::vector<std::string> groupLabels;
};

struct PKResult {
    float cmax = 0.0f;
    float tmax = 0.0f;
    float auc = 0.0f;
    float halfLife = 0.0f;
    float clearance = 0.0f;
    std::vector<float> timePoints;
    std::vector<float> concentrations;
};

struct ExperimentResult {
    ExperimentType type;
    bool completed = false;
    std::string summary;
    std::vector<TimePointSnapshot> snapshots;

    // Type-specific results
    ToxicologyResult toxResult;
    PKResult pkResult;
};

// ─── Procedure Types ───────────────────────────────────────────────────────

enum class ProcedureType {
    OralGavage,
    IVInjection,
    IPInjection,
    SCInjection,
    DermalApplication,
    BloodSample,
    Weigh,
    Temperature,
    Observe,
    Euthanize,
    Necropsy,
    BehavioralTest,
    SkinScoring,
    // Treatment procedures
    SalineFlush,
    ActivatedCharcoal,
    Antidote
};

inline const char* procedureTypeToString(ProcedureType p) {
    switch (p) {
    case ProcedureType::OralGavage: return "Oral Gavage";
    case ProcedureType::IVInjection: return "IV Injection";
    case ProcedureType::IPInjection: return "IP Injection";
    case ProcedureType::SCInjection: return "SC Injection";
    case ProcedureType::DermalApplication: return "Dermal Application";
    case ProcedureType::BloodSample: return "Blood Sample";
    case ProcedureType::Weigh: return "Weigh";
    case ProcedureType::Temperature: return "Temperature";
    case ProcedureType::Observe: return "Observe";
    case ProcedureType::Euthanize: return "Euthanize";
    case ProcedureType::Necropsy: return "Necropsy";
    case ProcedureType::BehavioralTest: return "Behavioral Test";
    case ProcedureType::SkinScoring: return "Skin Scoring";
    case ProcedureType::SalineFlush: return "Saline Flush";
    case ProcedureType::ActivatedCharcoal: return "Activated Charcoal";
    case ProcedureType::Antidote: return "Antidote";
    }
    return "Unknown";
}

} // namespace animsim
