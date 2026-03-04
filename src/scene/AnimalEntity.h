#pragma once

#include "renderer/Mesh.h"
#include "renderer/Renderer.h"
#include "simulation/Types.h"
#include <glm/glm.hpp>
#include <vector>

namespace animsim {

// A procedural 3D animal composed of multiple mesh parts
class AnimalEntity {
public:
    AnimalEntity();

    void buildModel(Species species);
    void setPosition(const glm::vec3& pos);
    void setRotation(float yawDegrees);
    void setHealthColor(float healthPercent);

    // Animate: breathing, death, cyanosis
    void animate(float deltaTime, float respRate, float health, float spo2, AnimalStatus status);

    // Get render objects for drawing
    void getRenderObjects(std::vector<RenderObject>& out) const;

    glm::vec3 getPosition() const { return m_position; }

    // Body region positions (world space) for procedure targeting
    glm::vec3 getBodyCenter() const;
    glm::vec3 getHeadPosition() const;
    glm::vec3 getTailPosition() const;
    glm::vec3 getAbdomenPosition() const;
    glm::vec3 getBackPosition() const;

private:
    void buildRat();
    void buildMouse();
    void buildRabbit();
    void buildGuineaPig();
    void buildDog();
    void buildMonkey();
    void addPart(Mesh&& mesh, glm::vec3 offset, glm::vec3 scale,
                 glm::vec3 color, float spec);
    void updateTransforms();

    struct Part {
        Mesh mesh;
        glm::vec3 localOffset{0.0f};
        glm::vec3 localScale{1.0f};
        glm::vec3 baseColor{0.9f, 0.85f, 0.78f};
        glm::vec3 currentColor{0.9f, 0.85f, 0.78f};
        float specular = 0.2f;
        mutable glm::mat4 worldTransform{1.0f};
    };

    std::vector<Part> m_parts;
    glm::vec3 m_position{0.0f};
    float m_yaw = 0.0f;
    float m_scale = 1.0f;
    Species m_species = Species::Rat;

    // Animation state
    float m_breathPhase = 0.0f;
    float m_deathTimer = 0.0f;    // 0 = alive, 1 = fully fallen
    float m_deathRoll = 0.0f;     // current roll angle in degrees
    bool m_isDead = false;
};

} // namespace animsim
