#pragma once

#include "scene/AnimalEntity.h"
#include "scene/ProcedureAnimation.h"
#include "renderer/Mesh.h"
#include "renderer/Renderer.h"
#include "simulation/Types.h"
#include <vector>
#include <memory>

namespace animsim {

class Scene {
public:
    Scene();

    // Build the lab environment
    void buildLab();

    // Set up animals for an experiment
    void setupAnimals(Species species, int count);
    void changeSpecies(Species species);

    // Update health visualization
    void updateAnimalHealth(int index, float healthPercent);

    // Animate animal (breathing, death, cyanosis) — call each frame
    void animateAnimal(int index, float dt, float respRate, float health,
                       float spo2, AnimalStatus status);

    // Play a procedure animation on an animal
    void playProcedure(ProcedureType type, int animalIndex);

    // Update animations (call each frame)
    void update(float dt);

    // Collect all render objects for the frame
    void getRenderObjects(std::vector<RenderObject>& out) const;

    // Access
    int getAnimalCount() const { return static_cast<int>(m_animals.size()); }
    bool hasActiveAnimation() const { return m_procedureAnim.isPlaying(); }

private:
    // Lab environment meshes
    std::vector<Mesh> m_labMeshes;
    std::vector<RenderObject> m_labObjects;

    // Animals
    std::vector<std::unique_ptr<AnimalEntity>> m_animals;

    // Procedure animation
    ProcedureAnimation m_procedureAnim;
};

} // namespace animsim
