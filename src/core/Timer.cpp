#include "core/Timer.h"
#include <GLFW/glfw3.h>

namespace animsim {

Timer::Timer() {
    m_lastTime = glfwGetTime();
}

void Timer::update() {
    double currentTime = glfwGetTime();
    m_deltaTime = static_cast<float>(currentTime - m_lastTime);
    m_lastTime = currentTime;
    m_elapsed += m_deltaTime;
    m_frameCount++;

    // Update FPS every 0.5 seconds
    m_fpsAccumulator += m_deltaTime;
    m_fpsFrameCount++;
    if (m_fpsAccumulator >= 0.5f) {
        m_fps = static_cast<float>(m_fpsFrameCount) / m_fpsAccumulator;
        m_fpsAccumulator = 0.0f;
        m_fpsFrameCount = 0;
    }
}

} // namespace animsim
