#pragma once

#include "simulation/Types.h"
#include <array>

namespace animsim {

class AnimalRegistry {
public:
    static const SpeciesParams& getParams(Species species);
    static const char* getSpeciesName(Species species);

private:
    static const std::array<SpeciesParams, static_cast<size_t>(Species::Count)> s_params;
};

} // namespace animsim
