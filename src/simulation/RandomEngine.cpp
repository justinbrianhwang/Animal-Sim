#include "simulation/RandomEngine.h"

namespace animsim {

RandomEngine::RandomEngine() {
    std::random_device rd;
    m_gen.seed(rd());
}

RandomEngine::RandomEngine(uint64_t seed) : m_gen(seed) {}

void RandomEngine::seed(uint64_t s) {
    m_gen.seed(s);
}

float RandomEngine::uniform(float min, float max) {
    std::uniform_real_distribution<float> dist(min, max);
    return dist(m_gen);
}

float RandomEngine::normal(float mean, float stddev) {
    std::normal_distribution<float> dist(mean, stddev);
    return dist(m_gen);
}

float RandomEngine::lognormal(float mean, float stddev) {
    std::lognormal_distribution<float> dist(mean, stddev);
    return dist(m_gen);
}

int RandomEngine::uniformInt(int min, int max) {
    std::uniform_int_distribution<int> dist(min, max);
    return dist(m_gen);
}

bool RandomEngine::chance(float probability) {
    return uniform(0.0f, 1.0f) < probability;
}

} // namespace animsim
