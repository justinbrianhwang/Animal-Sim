#include "simulation/DrugRegistry.h"
#include <nlohmann/json.hpp>
#include <fstream>
#include <cstdlib>

namespace animsim {

using json = nlohmann::json;

DrugRegistry& DrugRegistry::instance() {
    static DrugRegistry reg;
    return reg;
}

DrugRegistry::DrugRegistry() {
    registerBuiltInDrugs();

    // Try loading custom drugs from default path
    std::string path = getDefaultSavePath();
    loadCustomDrugs(path);
}

void DrugRegistry::registerBuiltInDrugs() {
    // 1. Generic Toxicant — balanced damage across organs (legacy behavior)
    {
        DrugCompound d;
        d.id = "generic_toxicant";
        d.name = "Generic Toxicant";
        d.category = "toxicant";
        d.molecularWeight = 200.0f;
        d.halfLifeHours = 2.0f;
        d.bioavailabilityOral = 0.6f;
        d.bioavailabilityIV = 1.0f;
        d.bioavailabilityIP = 0.9f;
        d.bioavailabilitySC = 0.85f;
        d.bioavailabilityDermal = 0.2f;
        d.volumeOfDistribution = 0.5f;
        d.hepatotoxicity = 0.5f;
        d.nephrotoxicity = 0.4f;
        d.cardiotoxicity = 0.3f;
        d.neurotoxicity = 0.2f;
        d.pulmonaryToxicity = 0.2f;
        d.giToxicity = 0.3f;
        d.hematotoxicity = 0.3f;
        d.dermalToxicity = 0.1f;
        d.therapeuticDose = 10.0f;
        d.ld50 = 300.0f;
        d.noael = 50.0f;
        d.isBuiltIn = true;
        m_drugs.push_back(d);
    }

    // 2. Acetaminophen — primarily hepatotoxic
    {
        DrugCompound d;
        d.id = "acetaminophen";
        d.name = "Acetaminophen";
        d.category = "analgesic";
        d.molecularWeight = 151.2f;
        d.halfLifeHours = 2.5f;
        d.bioavailabilityOral = 0.85f;
        d.bioavailabilityIV = 1.0f;
        d.bioavailabilityIP = 0.95f;
        d.bioavailabilitySC = 0.8f;
        d.bioavailabilityDermal = 0.05f;
        d.volumeOfDistribution = 0.9f;
        d.hepatotoxicity = 1.0f;   // primary target organ
        d.nephrotoxicity = 0.4f;
        d.cardiotoxicity = 0.1f;
        d.neurotoxicity = 0.05f;
        d.pulmonaryToxicity = 0.05f;
        d.giToxicity = 0.2f;
        d.hematotoxicity = 0.1f;
        d.dermalToxicity = 0.0f;
        d.therapeuticDose = 15.0f;
        d.ld50 = 338.0f;
        d.noael = 100.0f;
        d.isBuiltIn = true;
        m_drugs.push_back(d);
    }

    // 3. Cisplatin — nephrotoxic + hematotoxic (anticancer)
    {
        DrugCompound d;
        d.id = "cisplatin";
        d.name = "Cisplatin";
        d.category = "antineoplastic";
        d.molecularWeight = 300.1f;
        d.halfLifeHours = 0.5f; // very short initial half-life
        d.bioavailabilityOral = 0.1f;  // poor oral bioavailability
        d.bioavailabilityIV = 1.0f;
        d.bioavailabilityIP = 0.85f;
        d.bioavailabilitySC = 0.3f;
        d.bioavailabilityDermal = 0.01f;
        d.volumeOfDistribution = 0.3f;
        d.hepatotoxicity = 0.3f;
        d.nephrotoxicity = 1.0f;    // primary target
        d.cardiotoxicity = 0.2f;
        d.neurotoxicity = 0.4f;     // peripheral neuropathy
        d.pulmonaryToxicity = 0.1f;
        d.giToxicity = 0.6f;        // severe nausea/vomiting
        d.hematotoxicity = 0.8f;    // myelosuppression
        d.dermalToxicity = 0.1f;
        d.therapeuticDose = 3.0f;
        d.ld50 = 12.0f;
        d.noael = 1.0f;
        d.isBuiltIn = true;
        m_drugs.push_back(d);
    }

    // 4. Morphine — neurotoxic + respiratory depression
    {
        DrugCompound d;
        d.id = "morphine";
        d.name = "Morphine";
        d.category = "opioid_analgesic";
        d.molecularWeight = 285.3f;
        d.halfLifeHours = 3.0f;
        d.bioavailabilityOral = 0.3f;
        d.bioavailabilityIV = 1.0f;
        d.bioavailabilityIP = 0.8f;
        d.bioavailabilitySC = 0.9f;
        d.bioavailabilityDermal = 0.05f;
        d.volumeOfDistribution = 3.5f;
        d.hepatotoxicity = 0.2f;
        d.nephrotoxicity = 0.1f;
        d.cardiotoxicity = 0.3f;     // bradycardia
        d.neurotoxicity = 0.9f;      // primary: CNS depression
        d.pulmonaryToxicity = 0.8f;  // respiratory depression
        d.giToxicity = 0.4f;         // constipation, nausea
        d.hematotoxicity = 0.05f;
        d.dermalToxicity = 0.0f;
        d.therapeuticDose = 5.0f;
        d.ld50 = 500.0f;
        d.noael = 20.0f;
        d.isBuiltIn = true;
        m_drugs.push_back(d);
    }

    // 5. Saline — no toxicity (vehicle/control)
    {
        DrugCompound d;
        d.id = "saline";
        d.name = "Saline (0.9% NaCl)";
        d.category = "vehicle";
        d.molecularWeight = 58.4f;
        d.halfLifeHours = 0.5f;
        d.bioavailabilityOral = 1.0f;
        d.bioavailabilityIV = 1.0f;
        d.bioavailabilityIP = 1.0f;
        d.bioavailabilitySC = 1.0f;
        d.bioavailabilityDermal = 1.0f;
        d.volumeOfDistribution = 0.2f;
        d.hepatotoxicity = 0.0f;
        d.nephrotoxicity = 0.0f;
        d.cardiotoxicity = 0.0f;
        d.neurotoxicity = 0.0f;
        d.pulmonaryToxicity = 0.0f;
        d.giToxicity = 0.0f;
        d.hematotoxicity = 0.0f;
        d.dermalToxicity = 0.0f;
        d.therapeuticDose = 0.0f;
        d.ld50 = 99999.0f;
        d.noael = 99999.0f;
        d.isBuiltIn = true;
        m_drugs.push_back(d);
    }

    rebuildIndex();
}

void DrugRegistry::rebuildIndex() {
    m_index.clear();
    for (int i = 0; i < static_cast<int>(m_drugs.size()); i++) {
        m_index[m_drugs[i].id] = i;
    }
}

const DrugCompound& DrugRegistry::getDrug(const std::string& id) const {
    auto it = m_index.find(id);
    if (it != m_index.end()) {
        return m_drugs[it->second];
    }
    // Fallback to generic toxicant
    return m_drugs[0];
}

bool DrugRegistry::hasDrug(const std::string& id) const {
    return m_index.find(id) != m_index.end();
}

void DrugRegistry::addDrug(const DrugCompound& drug) {
    // Don't allow overwriting built-in drugs
    auto it = m_index.find(drug.id);
    if (it != m_index.end() && m_drugs[it->second].isBuiltIn) {
        return;
    }

    DrugCompound d = drug;
    d.isBuiltIn = false;

    if (it != m_index.end()) {
        m_drugs[it->second] = d;
    } else {
        m_drugs.push_back(d);
        rebuildIndex();
    }
}

void DrugRegistry::updateDrug(const DrugCompound& drug) {
    auto it = m_index.find(drug.id);
    if (it != m_index.end() && !m_drugs[it->second].isBuiltIn) {
        m_drugs[it->second] = drug;
        m_drugs[it->second].isBuiltIn = false;
    }
}

void DrugRegistry::removeDrug(const std::string& id) {
    auto it = m_index.find(id);
    if (it != m_index.end() && !m_drugs[it->second].isBuiltIn) {
        m_drugs.erase(m_drugs.begin() + it->second);
        rebuildIndex();
    }
}

static json drugToJson(const DrugCompound& d) {
    return {
        {"id", d.id},
        {"name", d.name},
        {"category", d.category},
        {"molecularWeight", d.molecularWeight},
        {"halfLifeHours", d.halfLifeHours},
        {"bioavailabilityOral", d.bioavailabilityOral},
        {"bioavailabilityIV", d.bioavailabilityIV},
        {"bioavailabilityIP", d.bioavailabilityIP},
        {"bioavailabilitySC", d.bioavailabilitySC},
        {"bioavailabilityDermal", d.bioavailabilityDermal},
        {"volumeOfDistribution", d.volumeOfDistribution},
        {"hepatotoxicity", d.hepatotoxicity},
        {"nephrotoxicity", d.nephrotoxicity},
        {"cardiotoxicity", d.cardiotoxicity},
        {"neurotoxicity", d.neurotoxicity},
        {"pulmonaryToxicity", d.pulmonaryToxicity},
        {"giToxicity", d.giToxicity},
        {"hematotoxicity", d.hematotoxicity},
        {"dermalToxicity", d.dermalToxicity},
        {"therapeuticDose", d.therapeuticDose},
        {"ld50", d.ld50},
        {"noael", d.noael}
    };
}

static DrugCompound drugFromJson(const json& j) {
    DrugCompound d;
    d.id = j.value("id", "custom");
    d.name = j.value("name", "Custom Drug");
    d.category = j.value("category", "custom");
    d.molecularWeight = j.value("molecularWeight", 200.0f);
    d.halfLifeHours = j.value("halfLifeHours", 2.0f);
    d.bioavailabilityOral = j.value("bioavailabilityOral", 0.6f);
    d.bioavailabilityIV = j.value("bioavailabilityIV", 1.0f);
    d.bioavailabilityIP = j.value("bioavailabilityIP", 0.9f);
    d.bioavailabilitySC = j.value("bioavailabilitySC", 0.85f);
    d.bioavailabilityDermal = j.value("bioavailabilityDermal", 0.2f);
    d.volumeOfDistribution = j.value("volumeOfDistribution", 0.5f);
    d.hepatotoxicity = j.value("hepatotoxicity", 0.5f);
    d.nephrotoxicity = j.value("nephrotoxicity", 0.3f);
    d.cardiotoxicity = j.value("cardiotoxicity", 0.2f);
    d.neurotoxicity = j.value("neurotoxicity", 0.2f);
    d.pulmonaryToxicity = j.value("pulmonaryToxicity", 0.1f);
    d.giToxicity = j.value("giToxicity", 0.3f);
    d.hematotoxicity = j.value("hematotoxicity", 0.2f);
    d.dermalToxicity = j.value("dermalToxicity", 0.1f);
    d.therapeuticDose = j.value("therapeuticDose", 10.0f);
    d.ld50 = j.value("ld50", 300.0f);
    d.noael = j.value("noael", 50.0f);
    d.isBuiltIn = false;
    return d;
}

bool DrugRegistry::saveCustomDrugs(const std::string& path) const {
    json arr = json::array();
    for (auto& d : m_drugs) {
        if (!d.isBuiltIn) {
            arr.push_back(drugToJson(d));
        }
    }

    std::ofstream file(path);
    if (!file.is_open()) return false;
    file << arr.dump(2);
    return true;
}

bool DrugRegistry::loadCustomDrugs(const std::string& path) {
    std::ifstream file(path);
    if (!file.is_open()) return false;

    try {
        json arr = json::parse(file);
        for (auto& item : arr) {
            DrugCompound d = drugFromJson(item);
            // Only add if not conflicting with built-in
            if (!hasDrug(d.id) || !getDrug(d.id).isBuiltIn) {
                addDrug(d);
            }
        }
        return true;
    } catch (...) {
        return false;
    }
}

std::string DrugRegistry::getDefaultSavePath() const {
    const char* home = std::getenv("HOME");
    if (!home) home = "/tmp";
    return std::string(home) + "/.animsim/custom_drugs.json";
}

} // namespace animsim
