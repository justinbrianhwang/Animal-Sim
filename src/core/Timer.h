#pragma once

namespace animsim {

class Timer {
public:
    Timer();

    void update();

    float getDeltaTime() const { return m_deltaTime; }
    float getElapsed() const { return m_elapsed; }
    float getFPS() const { return m_fps; }
    int getFrameCount() const { return m_frameCount; }

private:
    double m_lastTime = 0.0;
    float m_deltaTime = 0.0f;
    float m_elapsed = 0.0f;
    float m_fps = 0.0f;
    int m_frameCount = 0;

    // FPS counter
    float m_fpsAccumulator = 0.0f;
    int m_fpsFrameCount = 0;
};

} // namespace animsim
