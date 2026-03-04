#pragma once

#include <random>
#include <cstdint>

namespace animsim {

class RandomEngine {
public:
    RandomEngine();
    explicit RandomEngine(uint64_t seed);

    void seed(uint64_t s);

    float uniform(float min, float max);
    float normal(float mean, float stddev);
    float lognormal(float mean, float stddev);
    int uniformInt(int min, int max);
    bool chance(float probability); // 0.0 to 1.0

    std::mt19937_64& generator() { return m_gen; }

private:
    std::mt19937_64 m_gen;
};

} // namespace animsim
