#pragma once

#include "renderer/Mesh.h"
#include "renderer/Renderer.h"
#include "simulation/Types.h"
#include <glm/glm.hpp>
#include <vector>

namespace animsim {

// Animated procedure tool (syringe, gavage tube, etc.)
class ProcedureAnimation {
public:
    ProcedureAnimation();

    // Start an animation
    void start(ProcedureType type, const glm::vec3& targetPos, float durationSeconds = 2.0f);

    // Update animation (call each frame with delta time)
    void update(float dt);

    // Is animation currently playing?
    bool isPlaying() const { return m_playing; }

    // Get render objects for the animated tool + particles
    void getRenderObjects(std::vector<RenderObject>& out) const;

    // Spawn injection particles
    void spawnParticles(const glm::vec3& pos, const glm::vec3& color, int count = 12);

    // Update particles (call each frame)
    void updateParticles(float dt);

private:
    void buildSyringe();
    void buildGavageTube();
    void buildThermometer();
    void updateToolTransform();

    struct ToolPart {
        Mesh mesh;
        glm::vec3 localOffset{0.0f};
        glm::vec3 color{0.8f};
        float specular = 0.8f;
        mutable glm::mat4 worldTransform{1.0f};
    };

    std::vector<ToolPart> m_toolParts;
    ProcedureType m_type = ProcedureType::Observe;

    // Animation state
    bool m_playing = false;
    float m_time = 0.0f;
    float m_duration = 2.0f;
    glm::vec3 m_startPos{0.0f};
    glm::vec3 m_targetPos{0.0f};
    glm::vec3 m_currentPos{0.0f};
    float m_currentRotation = 0.0f;

    // Pre-built tool meshes
    bool m_syringeBuilt = false;
    bool m_gavageBuilt = false;
    bool m_thermoBuilt = false;

    // Particle system
    struct Particle {
        glm::vec3 pos;
        glm::vec3 vel;
        glm::vec3 color;
        float life;     // seconds remaining
        float maxLife;
    };
    std::vector<Particle> m_particles;
    Mesh m_particleMesh;
    bool m_particleMeshBuilt = false;
};

} // namespace animsim
