#include "renderer/Camera.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

namespace animsim {

Camera::Camera() {
    updatePosition();
}

void Camera::rotate(float deltaYaw, float deltaPitch) {
    m_yaw += deltaYaw;
    m_pitch += deltaPitch;
    m_pitch = std::clamp(m_pitch, m_minPitch, m_maxPitch);
    updatePosition();
}

void Camera::zoom(float delta) {
    m_distance -= delta * 0.5f;
    m_distance = std::clamp(m_distance, m_minDistance, m_maxDistance);
    updatePosition();
}

void Camera::pan(float deltaX, float deltaY) {
    glm::vec3 forward = glm::normalize(m_target - m_position);
    glm::vec3 right = glm::normalize(glm::cross(forward, glm::vec3(0, 1, 0)));
    glm::vec3 up = glm::normalize(glm::cross(right, forward));

    float panSpeed = m_distance * 0.002f;
    m_target += right * (-deltaX * panSpeed) + up * (deltaY * panSpeed);
    updatePosition();
}

void Camera::lookAt(const glm::vec3& target) {
    m_target = target;
    updatePosition();
}

void Camera::setDistance(float dist) {
    m_distance = std::clamp(dist, m_minDistance, m_maxDistance);
    updatePosition();
}

glm::mat4 Camera::getViewMatrix() const {
    return glm::lookAt(m_position, m_target, glm::vec3(0.0f, 1.0f, 0.0f));
}

glm::mat4 Camera::getProjectionMatrix(float aspectRatio) const {
    return glm::perspective(glm::radians(fov), aspectRatio, nearPlane, farPlane);
}

glm::vec3 Camera::getPosition() const {
    return m_position;
}

void Camera::updatePosition() {
    float yawRad = glm::radians(m_yaw);
    float pitchRad = glm::radians(m_pitch);

    m_position.x = m_target.x + m_distance * std::cos(pitchRad) * std::cos(yawRad);
    m_position.y = m_target.y + m_distance * std::sin(pitchRad);
    m_position.z = m_target.z + m_distance * std::cos(pitchRad) * std::sin(yawRad);
}

} // namespace animsim
