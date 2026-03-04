#include "scene/Scene.h"
#include <glm/gtc/matrix_transform.hpp>
#include <cmath>

namespace animsim {

Scene::Scene() {}

void Scene::buildLab() {
    m_labMeshes.clear();
    m_labObjects.clear();

    // Reserve to prevent reallocation invalidating pointers
    m_labMeshes.reserve(40);

    // ── Floor (large lab tile) ──────────────────────────────────
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(8.0f, 0.05f, 6.0f), glm::vec3(0.85f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, -0.025f, 0.0f));
        obj.color = glm::vec3(0.38f, 0.40f, 0.38f);
        obj.specularStrength = 0.4f;
        m_labObjects.push_back(obj);
    }

    // ── Back wall ───────────────────────────────────────────────
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(8.0f, 3.0f, 0.1f), glm::vec3(0.85f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.5f, -2.5f));
        obj.color = glm::vec3(0.62f, 0.64f, 0.68f);
        obj.specularStrength = 0.15f;
        m_labObjects.push_back(obj);
    }

    // ── Lab bench (main workspace) ──────────────────────────────
    // Bench top (darker wood, wider)
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(3.5f, 0.07f, 1.8f), glm::vec3(0.85f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.75f, 0.0f));
        obj.color = glm::vec3(0.45f, 0.36f, 0.28f);
        obj.specularStrength = 0.35f;
        m_labObjects.push_back(obj);
    }

    // Bench legs (chrome)
    m_labMeshes.push_back(Mesh::createCylinder(0.035f, 0.75f, 10, glm::vec3(0.7f)));
    glm::vec3 legPos[] = {
        {-1.55f, 0.375f, 0.70f}, {1.55f, 0.375f, 0.70f},
        {-1.55f, 0.375f, -0.70f}, {1.55f, 0.375f, -0.70f}
    };
    for (auto& lp : legPos) {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), lp);
        obj.color = glm::vec3(0.50f, 0.50f, 0.55f);
        obj.specularStrength = 0.85f;
        m_labObjects.push_back(obj);
    }

    // ── Green rubber lab mat (on bench surface) ─────────────────
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(1.8f, 0.015f, 0.9f), glm::vec3(0.85f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.798f, 0.0f));
        obj.color = glm::vec3(0.18f, 0.35f, 0.22f);
        obj.specularStrength = 0.2f;
        m_labObjects.push_back(obj);
    }

    // ── Overhead fluorescent light ──────────────────────────────
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(1.2f, 0.04f, 0.3f), glm::vec3(0.95f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.5f, 0.0f));
        obj.color = glm::vec3(0.95f, 0.95f, 0.92f);
        obj.specularStrength = 0.1f;
        m_labObjects.push_back(obj);
    }
    // Light housing
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(1.3f, 0.06f, 0.35f), glm::vec3(0.8f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 2.55f, 0.0f));
        obj.color = glm::vec3(0.45f, 0.45f, 0.48f);
        obj.specularStrength = 0.5f;
        m_labObjects.push_back(obj);
    }

    // ── Weighing scale (right side of bench) ────────────────────
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(0.28f, 0.04f, 0.22f), glm::vec3(0.9f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0.82f, -0.3f));
        obj.color = glm::vec3(0.32f, 0.32f, 0.36f);
        obj.specularStrength = 0.7f;
        m_labObjects.push_back(obj);
    }
    // Scale display
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(0.08f, 0.1f, 0.02f), glm::vec3(0.9f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(1.2f, 0.90f, -0.42f));
        obj.color = glm::vec3(0.15f, 0.20f, 0.15f);
        obj.specularStrength = 0.3f;
        m_labObjects.push_back(obj);
    }

    // ── Beakers (right-back area) ───────────────────────────────
    m_labMeshes.push_back(Mesh::createCylinder(0.03f, 0.10f, 12, glm::vec3(0.9f)));
    for (float xOff : {0.0f, 0.10f, 0.20f}) {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f + xOff, 0.85f, 0.55f));
        obj.color = glm::vec3(0.6f, 0.72f, 0.82f);
        obj.specularStrength = 0.95f;
        m_labObjects.push_back(obj);
    }

    // Colored liquids inside beakers
    m_labMeshes.push_back(Mesh::createCylinder(0.025f, 0.04f, 12, glm::vec3(0.9f)));
    glm::vec3 liquidColors[] = {{0.3f, 0.7f, 0.4f}, {0.3f, 0.5f, 0.9f}, {0.9f, 0.6f, 0.3f}};
    for (int i = 0; i < 3; i++) {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f),
            glm::vec3(1.0f + i * 0.10f, 0.83f, 0.55f));
        obj.color = liquidColors[i];
        obj.specularStrength = 0.5f;
        m_labObjects.push_back(obj);
    }

    // ── Test tube rack ──────────────────────────────────────────
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(0.20f, 0.03f, 0.08f), glm::vec3(0.85f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(-1.2f, 0.81f, -0.5f));
        obj.color = glm::vec3(0.55f, 0.55f, 0.60f);
        obj.specularStrength = 0.6f;
        m_labObjects.push_back(obj);
    }
    // Test tubes
    m_labMeshes.push_back(Mesh::createCylinder(0.006f, 0.08f, 6, glm::vec3(0.9f)));
    for (int i = 0; i < 6; i++) {
        float x = -1.30f + i * 0.04f;
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(x, 0.87f, -0.5f));
        obj.color = glm::vec3(0.65f, 0.78f, 0.88f);
        obj.specularStrength = 0.95f;
        m_labObjects.push_back(obj);
    }

    // ── Syringe (lying on bench) ────────────────────────────────
    m_labMeshes.push_back(Mesh::createCylinder(0.013f, 0.18f, 8, glm::vec3(0.9f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(0.6f, 0.84f, 0.5f));
        t = glm::rotate(t, glm::radians(90.0f), glm::vec3(0, 0, 1));
        obj.transform = t;
        obj.color = glm::vec3(0.82f, 0.85f, 0.90f);
        obj.specularStrength = 0.95f;
        m_labObjects.push_back(obj);
    }
    // Needle
    m_labMeshes.push_back(Mesh::createCylinder(0.002f, 0.10f, 4, glm::vec3(0.9f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(0.46f, 0.84f, 0.5f));
        t = glm::rotate(t, glm::radians(90.0f), glm::vec3(0, 0, 1));
        obj.transform = t;
        obj.color = glm::vec3(0.70f, 0.70f, 0.75f);
        obj.specularStrength = 1.0f;
        m_labObjects.push_back(obj);
    }

    // ── Clipboard (left-back) ───────────────────────────────────
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(0.18f, 0.25f, 0.015f), glm::vec3(0.9f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        glm::mat4 t = glm::translate(glm::mat4(1.0f), glm::vec3(-1.3f, 0.93f, -0.15f));
        t = glm::rotate(t, glm::radians(-15.0f), glm::vec3(1, 0, 0));
        obj.transform = t;
        obj.color = glm::vec3(0.55f, 0.40f, 0.25f);
        obj.specularStrength = 0.2f;
        m_labObjects.push_back(obj);
    }

    // ── Petri dishes (front-right) ──────────────────────────────
    m_labMeshes.push_back(Mesh::createCylinder(0.04f, 0.012f, 14, glm::vec3(0.9f)));
    for (float xOff : {0.0f, 0.10f}) {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.7f + xOff, 0.81f, -0.45f));
        obj.color = glm::vec3(0.70f, 0.75f, 0.80f);
        obj.specularStrength = 0.9f;
        m_labObjects.push_back(obj);
    }

    // ── Back cabinet / shelf ────────────────────────────────────
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(2.0f, 1.2f, 0.4f), glm::vec3(0.8f)));
    {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 1.35f, -2.1f));
        obj.color = glm::vec3(0.52f, 0.50f, 0.55f);
        obj.specularStrength = 0.3f;
        m_labObjects.push_back(obj);
    }
    // Cabinet shelves
    m_labMeshes.push_back(Mesh::createCube(glm::vec3(1.9f, 0.02f, 0.35f), glm::vec3(0.8f)));
    for (float y : {1.0f, 1.4f, 1.8f}) {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, y, -2.08f));
        obj.color = glm::vec3(0.48f, 0.46f, 0.50f);
        obj.specularStrength = 0.4f;
        m_labObjects.push_back(obj);
    }
    // Bottles on shelves
    m_labMeshes.push_back(Mesh::createCylinder(0.025f, 0.12f, 8, glm::vec3(0.9f)));
    glm::vec3 bottleColors[] = {
        {0.3f, 0.5f, 0.8f}, {0.7f, 0.3f, 0.3f}, {0.3f, 0.7f, 0.4f},
        {0.8f, 0.6f, 0.2f}, {0.5f, 0.3f, 0.7f}
    };
    for (int i = 0; i < 5; i++) {
        RenderObject obj;
        obj.mesh = &m_labMeshes.back();
        obj.transform = glm::translate(glm::mat4(1.0f),
            glm::vec3(-0.6f + i * 0.3f, 1.50f, -2.0f));
        obj.color = bottleColors[i];
        obj.specularStrength = 0.8f;
        m_labObjects.push_back(obj);
    }
}

void Scene::setupAnimals(Species species, int count) {
    m_animals.clear();

    // Single animal centered on the bench (Surgeon Simulator style)
    float benchY = 0.82f;

    for (int i = 0; i < count && i < 1; i++) {
        auto animal = std::make_unique<AnimalEntity>();
        animal->buildModel(species);
        animal->setPosition(glm::vec3(0.0f, benchY, 0.0f));
        animal->setRotation(15.0f); // slight angle for visual interest
        m_animals.push_back(std::move(animal));
    }
}

void Scene::changeSpecies(Species species) {
    if (m_animals.empty()) return;
    float benchY = 0.82f;
    m_animals[0]->buildModel(species);
    m_animals[0]->setPosition(glm::vec3(0.0f, benchY, 0.0f));
    m_animals[0]->setRotation(15.0f);
}

void Scene::updateAnimalHealth(int index, float healthPercent) {
    if (index >= 0 && index < static_cast<int>(m_animals.size())) {
        m_animals[index]->setHealthColor(healthPercent);
    }
}

void Scene::playProcedure(ProcedureType type, int animalIndex) {
    if (animalIndex < 0 || animalIndex >= static_cast<int>(m_animals.size())) return;

    glm::vec3 targetPos;
    switch (type) {
    case ProcedureType::OralGavage:
        targetPos = m_animals[animalIndex]->getHeadPosition();
        break;
    case ProcedureType::IPInjection:
        targetPos = m_animals[animalIndex]->getAbdomenPosition();
        break;
    case ProcedureType::SCInjection:
        targetPos = m_animals[animalIndex]->getBackPosition();
        break;
    case ProcedureType::IVInjection:
        targetPos = m_animals[animalIndex]->getTailPosition();
        break;
    case ProcedureType::Temperature:
        targetPos = m_animals[animalIndex]->getTailPosition();
        break;
    default:
        targetPos = m_animals[animalIndex]->getBodyCenter();
        break;
    }

    m_procedureAnim.start(type, targetPos, 2.5f);
}

void Scene::animateAnimal(int index, float dt, float respRate, float health,
                           float spo2, AnimalStatus status) {
    if (index >= 0 && index < static_cast<int>(m_animals.size())) {
        m_animals[index]->animate(dt, respRate, health, spo2, status);
    }
}

void Scene::update(float dt) {
    m_procedureAnim.update(dt);
    m_procedureAnim.updateParticles(dt);
}

void Scene::getRenderObjects(std::vector<RenderObject>& out) const {
    // Lab environment
    out.insert(out.end(), m_labObjects.begin(), m_labObjects.end());

    // Animals
    for (auto& animal : m_animals) {
        animal->getRenderObjects(out);
    }

    // Procedure animation
    m_procedureAnim.getRenderObjects(out);
}

} // namespace animsim
