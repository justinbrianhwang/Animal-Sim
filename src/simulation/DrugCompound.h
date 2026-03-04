#pragma once

#include <string>

namespace animsim {

struct DrugCompound {
    std::string id;           // unique key: "acetaminophen", "cisplatin", "custom_001"
    std::string name;         // display name
    std::string category;     // "analgesic", "antineoplastic", "toxicant", "custom"

    // Pharmacokinetics
    float molecularWeight = 200.0f;    // g/mol
    float halfLifeHours = 2.0f;        // elimination half-life
    float bioavailabilityOral = 0.6f;
    float bioavailabilityIV = 1.0f;
    float bioavailabilityIP = 0.9f;
    float bioavailabilitySC = 0.85f;
    float bioavailabilityDermal = 0.2f;
    float volumeOfDistribution = 0.5f; // L/kg

    // Toxicity profile — organ-specific damage weights (0 = no damage, 1 = primary target)
    float hepatotoxicity = 0.5f;     // liver
    float nephrotoxicity = 0.3f;     // kidney
    float cardiotoxicity = 0.2f;     // heart
    float neurotoxicity = 0.2f;      // brain
    float pulmonaryToxicity = 0.1f;  // lungs
    float giToxicity = 0.3f;         // stomach/intestines
    float hematotoxicity = 0.2f;     // bone marrow/spleen
    float dermalToxicity = 0.1f;     // skin

    // Dose-response
    float therapeuticDose = 10.0f;   // mg/kg - effective dose
    float ld50 = 300.0f;             // mg/kg - lethal dose 50%
    float noael = 50.0f;             // mg/kg - no observable adverse effect level

    bool isBuiltIn = true;

    // Get bioavailability for a given route
    float getBioavailability(const std::string& route) const {
        if (route == "oral") return bioavailabilityOral;
        if (route == "iv") return bioavailabilityIV;
        if (route == "ip") return bioavailabilityIP;
        if (route == "sc") return bioavailabilitySC;
        if (route == "dermal") return bioavailabilityDermal;
        return 1.0f;
    }
};

} // namespace animsim
