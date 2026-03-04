#include "scene/ProcedureAnimation.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>
#include <algorithm>

namespace animsim {

ProcedureAnimation::ProcedureAnimation() {}

void ProcedureAnimation::start(ProcedureType type, const glm::vec3& targetPos, float durationSeconds) {
    m_type = type;
    m_targetPos = targetPos;
    m_duration = durationSeconds;
    m_time = 0.0f;
    m_playing = true;

    // Start position: above and to the right of target
    m_startPos = targetPos + glm::vec3(0.3f, 0.4f, 0.0f);
    m_currentPos = m_startPos;

    // Build the appropriate tool mesh
    m_toolParts.clear();

    switch (type) {
    case ProcedureType::OralGavage:
        buildGavageTube();
        break;
    case ProcedureType::Temperature:
        buildThermometer();
        break;
    case ProcedureType::IVInjection:
    case ProcedureType::IPInjection:
    case ProcedureType::SCInjection:
    case ProcedureType::BloodSample:
        buildSyringe();
        break;
    default:
        // No visual tool for observe/weigh/etc
        m_playing = false;
        break;
    }
}

void ProcedureAnimation::update(float dt) {
    if (!m_playing) return;

    m_time += dt;
    float t = std::min(1.0f, m_time / m_duration);

    // Easing: smoothstep for approach, hold at target, then retract
    float phase;
    if (t < 0.4f) {
        // Approach phase: move toward target
        phase = t / 0.4f;
        float smooth = phase * phase * (3.0f - 2.0f * phase); // smoothstep
        m_currentPos = glm::mix(m_startPos, m_targetPos, smooth);
        m_currentRotation = -45.0f * smooth; // rotate to injection angle
    } else if (t < 0.7f) {
        // Hold at target (injection/procedure happening)
        m_currentPos = m_targetPos;
        m_currentRotation = -45.0f;
        // Slight pulsing
        float pulse = std::sin((t - 0.4f) / 0.3f * 3.14159f * 4.0f) * 0.005f;
        m_currentPos.y += pulse;

        // Spawn particles at injection point (once, near midpoint)
        if (t > 0.45f && t < 0.55f) {
            bool isInjection = (m_type == ProcedureType::IVInjection ||
                                m_type == ProcedureType::IPInjection ||
                                m_type == ProcedureType::SCInjection);
            bool isBlood = (m_type == ProcedureType::BloodSample);
            if (isInjection) {
                spawnParticles(m_targetPos, glm::vec3(0.85f, 0.65f, 0.2f), 3); // amber drug particles
            } else if (isBlood) {
                spawnParticles(m_targetPos, glm::vec3(0.8f, 0.15f, 0.12f), 3); // red blood particles
            } else if (m_type == ProcedureType::OralGavage) {
                spawnParticles(m_targetPos, glm::vec3(0.3f, 0.6f, 0.9f), 2); // blue liquid particles
            }
        }
    } else {
        // Retract
        float retractT = (t - 0.7f) / 0.3f;
        float smooth = retractT * retractT * (3.0f - 2.0f * retractT);
        m_currentPos = glm::mix(m_targetPos, m_startPos, smooth);
        m_currentRotation = -45.0f * (1.0f - smooth);
    }

    updateToolTransform();
    updateParticles(dt);

    if (t >= 1.0f) {
        m_playing = false;
    }
}

void ProcedureAnimation::getRenderObjects(std::vector<RenderObject>& out) const {
    if (!m_playing && m_particles.empty()) return;

    // Tool parts
    if (m_playing) {
        for (auto& part : m_toolParts) {
            RenderObject ro;
            ro.mesh = const_cast<Mesh*>(&part.mesh);
            ro.transform = part.worldTransform;
            ro.color = part.color;
            ro.specularStrength = part.specular;
            out.push_back(ro);
        }
    }

    // Particles
    for (auto& p : m_particles) {
        RenderObject ro;
        ro.mesh = const_cast<Mesh*>(&m_particleMesh);
        ro.transform = glm::translate(glm::mat4(1.0f), p.pos);
        float alpha = p.life / p.maxLife;
        ro.color = p.color * alpha;
        ro.specularStrength = 0.3f;
        out.push_back(ro);
    }
}

void ProcedureAnimation::buildSyringe() {
    // Barrel
    ToolPart barrel;
    barrel.mesh = Mesh::createCylinder(0.012f, 0.15f, 8, glm::vec3(1.0f));
    barrel.localOffset = {0.0f, 0.0f, 0.0f};
    barrel.color = glm::vec3(0.85f, 0.88f, 0.92f); // glass-like
    barrel.specular = 0.95f;
    m_toolParts.push_back(std::move(barrel));

    // Plunger
    ToolPart plunger;
    plunger.mesh = Mesh::createCylinder(0.008f, 0.06f, 6, glm::vec3(1.0f));
    plunger.localOffset = {0.0f, 0.1f, 0.0f};
    plunger.color = glm::vec3(0.4f, 0.4f, 0.45f);
    plunger.specular = 0.6f;
    m_toolParts.push_back(std::move(plunger));

    // Needle
    ToolPart needle;
    needle.mesh = Mesh::createCylinder(0.002f, 0.08f, 4, glm::vec3(1.0f));
    needle.localOffset = {0.0f, -0.115f, 0.0f};
    needle.color = glm::vec3(0.75f, 0.75f, 0.80f); // steel
    needle.specular = 1.0f;
    m_toolParts.push_back(std::move(needle));

    // Liquid inside (colored)
    ToolPart liquid;
    liquid.mesh = Mesh::createCylinder(0.009f, 0.08f, 6, glm::vec3(1.0f));
    liquid.localOffset = {0.0f, -0.02f, 0.0f};
    liquid.color = glm::vec3(0.3f, 0.6f, 0.9f); // blue liquid
    liquid.specular = 0.5f;
    m_toolParts.push_back(std::move(liquid));
}

void ProcedureAnimation::buildGavageTube() {
    // Syringe body
    ToolPart body;
    body.mesh = Mesh::createCylinder(0.015f, 0.18f, 8, glm::vec3(1.0f));
    body.localOffset = {0.0f, 0.0f, 0.0f};
    body.color = glm::vec3(0.85f, 0.88f, 0.92f);
    body.specular = 0.9f;
    m_toolParts.push_back(std::move(body));

    // Feeding tube (long, thin, flexible look)
    ToolPart tube;
    tube.mesh = Mesh::createCylinder(0.004f, 0.2f, 4, glm::vec3(1.0f));
    tube.localOffset = {0.0f, -0.19f, 0.0f};
    tube.color = glm::vec3(0.8f, 0.6f, 0.4f); // orange tube
    tube.specular = 0.3f;
    m_toolParts.push_back(std::move(tube));

    // Ball tip
    ToolPart tip;
    tip.mesh = Mesh::createSphere(0.006f, 6, glm::vec3(1.0f));
    tip.localOffset = {0.0f, -0.3f, 0.0f};
    tip.color = glm::vec3(0.75f, 0.75f, 0.80f);
    tip.specular = 0.8f;
    m_toolParts.push_back(std::move(tip));
}

void ProcedureAnimation::buildThermometer() {
    // Body
    ToolPart body;
    body.mesh = Mesh::createCylinder(0.008f, 0.15f, 6, glm::vec3(1.0f));
    body.localOffset = {0.0f, 0.0f, 0.0f};
    body.color = glm::vec3(0.9f, 0.9f, 0.92f);
    body.specular = 0.8f;
    m_toolParts.push_back(std::move(body));

    // Tip (metallic)
    ToolPart tip;
    tip.mesh = Mesh::createCylinder(0.005f, 0.02f, 4, glm::vec3(1.0f));
    tip.localOffset = {0.0f, -0.085f, 0.0f};
    tip.color = glm::vec3(0.7f, 0.7f, 0.75f);
    tip.specular = 0.95f;
    m_toolParts.push_back(std::move(tip));
}

void ProcedureAnimation::spawnParticles(const glm::vec3& pos, const glm::vec3& color, int count) {
    // Build particle mesh once
    if (!m_particleMeshBuilt) {
        m_particleMesh = Mesh::createSphere(0.004f, 4, glm::vec3(1.0f));
        m_particleMeshBuilt = true;
    }

    for (int i = 0; i < count; i++) {
        Particle p;
        p.pos = pos;
        // Random outward velocity
        float angle = static_cast<float>(i) / count * 6.28f + (rand() % 100) / 100.0f;
        float speed = 0.05f + (rand() % 100) / 1000.0f;
        p.vel = glm::vec3(std::cos(angle) * speed, 0.04f + (rand() % 50) / 1000.0f, std::sin(angle) * speed);
        p.color = color;
        p.maxLife = 0.6f + (rand() % 40) / 100.0f;
        p.life = p.maxLife;
        m_particles.push_back(p);
    }
}

void ProcedureAnimation::updateParticles(float dt) {
    for (auto& p : m_particles) {
        p.pos += p.vel * dt;
        p.vel.y -= 0.15f * dt; // gravity
        p.life -= dt;
    }
    // Remove dead particles
    m_particles.erase(
        std::remove_if(m_particles.begin(), m_particles.end(),
                       [](const Particle& p) { return p.life <= 0.0f; }),
        m_particles.end());
}

void ProcedureAnimation::updateToolTransform() {
    glm::mat4 base = glm::translate(glm::mat4(1.0f), m_currentPos);
    base = glm::rotate(base, glm::radians(m_currentRotation), glm::vec3(0, 0, 1));

    for (auto& part : m_toolParts) {
        glm::mat4 local = glm::translate(glm::mat4(1.0f), part.localOffset);
        part.worldTransform = base * local;
    }
}

} // namespace animsim
