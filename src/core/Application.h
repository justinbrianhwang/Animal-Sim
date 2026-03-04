#pragma once

#include "renderer/Renderer.h"
#include "renderer/Camera.h"
#include "renderer/Mesh.h"
#include "simulation/Types.h"
#include "simulation/SimulationEngine.h"
#include "interactive/Controller.h"
#include "scene/Scene.h"
#include <memory>
#include <string>
#include <vector>

namespace animsim {

class Window;
class Timer;

enum class AppState {
    Dashboard,
    ExperimentSetup,
    Simulation,
    Interactive,
    Results,
    DrugEditor
};

class Application {
public:
    Application();
    ~Application();

    void run();

    // State transitions
    void goToDashboard();
    void goToSetup(const std::string& experimentType);
    void goToSimulation();
    void goToInteractive();
    void goToResults();
    void goToDrugEditor();

    AppState getState() const { return m_state; }
    Window& getWindow() { return *m_window; }
    Timer& getTimer() { return *m_timer; }
    Renderer& getRenderer() { return m_renderer; }
    Camera& getCamera() { return m_camera; }
    Scene& getScene() { return m_scene; }

    // Currently selected experiment type
    const std::string& getSelectedExperimentType() const { return m_selectedExperimentType; }

    // Simulation
    SimulationEngine& getSimEngine() { return m_simEngine; }
    ExperimentConfig& getExperimentConfig() { return m_experimentConfig; }
    void startSimulation();
    void startInteractive();

    // Interactive controller
    InteractiveController& getInteractiveCtrl() { return m_interactiveCtrl; }

    // Store result for results screen (from either sim or interactive)
    ExperimentResult& getLastResult() { return m_lastResult; }

    // Selected animal for 3D display
    int getSelectedAnimalIdx() const { return m_selectedAnimalIdx; }
    void setSelectedAnimalIdx(int idx) { m_selectedAnimalIdx = idx; }

    // 3D scene - dynamic render objects rebuilt each frame
    void collectRenderObjects(std::vector<RenderObject>& out) const;

private:
    void handleCameraInput();
    void initImGui();
    void shutdownImGui();
    void beginFrame();
    void endFrame();
    void renderGui();
    void updateScene();

    std::unique_ptr<Window> m_window;
    std::unique_ptr<Timer> m_timer;

    // 3D rendering
    Renderer m_renderer;
    Camera m_camera;
    Scene m_scene;

    // Simulation
    SimulationEngine m_simEngine;
    ExperimentConfig m_experimentConfig;
    ExperimentResult m_lastResult;

    // Interactive mode
    InteractiveController m_interactiveCtrl;

    // Procedure animation trigger
    float m_nextProcAnimTime = 0.0f;
    int m_procAnimAnimalIdx = 0;

    int m_selectedAnimalIdx = 0;
    AppState m_state = AppState::Dashboard;
    std::string m_selectedExperimentType;
};

} // namespace animsim
