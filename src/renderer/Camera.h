#pragma once

#include <glm/glm.hpp>

namespace animsim {

class Camera {
public:
    Camera();

    // Orbit controls
    void rotate(float deltaYaw, float deltaPitch);
    void zoom(float delta);
    void pan(float deltaX, float deltaY);

    // Focus on a point
    void lookAt(const glm::vec3& target);
    void setDistance(float dist);

    // Matrices
    glm::mat4 getViewMatrix() const;
    glm::mat4 getProjectionMatrix(float aspectRatio) const;
    glm::vec3 getPosition() const;
    glm::vec3 getTarget() const { return m_target; }

    // Settings
    float fov = 55.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;

private:
    void updatePosition();

    glm::vec3 m_target{0.0f, 0.5f, 0.0f};
    float m_distance = 2.5f;
    float m_yaw = 0.0f;      // degrees - straight at bench
    float m_pitch = 50.0f;   // degrees - looking down
    float m_minPitch = -85.0f;
    float m_maxPitch = 85.0f;
    float m_minDistance = 1.0f;
    float m_maxDistance = 50.0f;
    glm::vec3 m_position;
};

} // namespace animsim
