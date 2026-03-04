#include "simulation/AnimalRegistry.h"

namespace animsim {

// Species physiology data based on real laboratory animal values
const std::array<SpeciesParams, static_cast<size_t>(Species::Count)> AnimalRegistry::s_params = {{
    // Mouse
    {Species::Mouse, "Mouse",
     0.025f, 0.005f,    // weight: 25g ± 5g
     600.0f, 50.0f,     // heart rate: 600 ± 50 bpm
     37.0f, 0.5f,       // temp: 37.0 ± 0.5°C
     163.0f, 20.0f,     // resp rate: 163 ± 20
     1.5f,              // metabolic rate (vs rat)
     0.8f,              // LD50 sensitivity
     72.0f},            // blood volume ml/kg

    // Rat
    {Species::Rat, "Rat",
     0.300f, 0.040f,    // weight: 300g ± 40g
     360.0f, 30.0f,     // heart rate: 360 ± 30 bpm
     37.2f, 0.4f,       // temp: 37.2 ± 0.4°C
     85.0f, 10.0f,      // resp rate: 85 ± 10
     1.0f,              // metabolic rate (reference)
     1.0f,              // LD50 sensitivity (reference)
     64.0f},            // blood volume ml/kg

    // Rabbit
    {Species::Rabbit, "Rabbit",
     3.0f, 0.4f,        // weight: 3kg ± 0.4kg
     220.0f, 20.0f,     // heart rate: 220 ± 20 bpm
     38.5f, 0.5f,       // temp: 38.5 ± 0.5°C
     40.0f, 5.0f,       // resp rate: 40 ± 5
     0.7f,              // metabolic rate
     1.2f,              // LD50 sensitivity
     56.0f},            // blood volume ml/kg

    // Guinea Pig
    {Species::GuineaPig, "Guinea Pig",
     0.800f, 0.100f,    // weight: 800g ± 100g
     280.0f, 25.0f,     // heart rate: 280 ± 25 bpm
     38.0f, 0.4f,       // temp: 38.0 ± 0.4°C
     90.0f, 10.0f,      // resp rate: 90 ± 10
     0.9f,              // metabolic rate
     0.9f,              // LD50 sensitivity
     75.0f},            // blood volume ml/kg

    // Dog (Beagle)
    {Species::Dog, "Dog",
     10.0f, 1.5f,       // weight: 10kg ± 1.5kg
     100.0f, 15.0f,     // heart rate: 100 ± 15 bpm
     38.5f, 0.3f,       // temp: 38.5 ± 0.3°C
     22.0f, 4.0f,       // resp rate: 22 ± 4
     0.5f,              // metabolic rate
     1.5f,              // LD50 sensitivity
     85.0f},            // blood volume ml/kg

    // Monkey (Cynomolgus)
    {Species::Monkey, "Monkey",
     5.0f, 1.0f,        // weight: 5kg ± 1kg
     160.0f, 20.0f,     // heart rate: 160 ± 20 bpm
     38.0f, 0.3f,       // temp: 38.0 ± 0.3°C
     30.0f, 5.0f,       // resp rate: 30 ± 5
     0.6f,              // metabolic rate
     1.8f,              // LD50 sensitivity (more sensitive)
     72.0f},            // blood volume ml/kg
}};

const SpeciesParams& AnimalRegistry::getParams(Species species) {
    return s_params[static_cast<size_t>(species)];
}

const char* AnimalRegistry::getSpeciesName(Species species) {
    return s_params[static_cast<size_t>(species)].name;
}

} // namespace animsim
