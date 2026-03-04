#include "core/Application.h"
#include "core/Window.h"
#include "core/Timer.h"
#include "gui/GuiManager.h"
#include "simulation/experiments/ToxicologyExperiment.h"
#include "simulation/experiments/PKExperiment.h"
#include "simulation/experiments/BehavioralExperiment.h"
#include "simulation/experiments/DrugEfficacyExperiment.h"
#include "simulation/experiments/SkinIrritationExperiment.h"

#include <glad/gl.h>
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>
#include <glm/gtc/matrix_transform.hpp>

namespace animsim {

Application::Application() {
    m_window = std::make_unique<Window>(1600, 900, "Animal Experiment Simulator 3D");
    m_timer = std::make_unique<Timer>();
    initImGui();
    m_renderer.init();

    // Build initial lab scene with 1 preview animal
    m_scene.buildLab();
    m_scene.setupAnimals(Species::Rat, 1);

    // Camera: looking down at bench (Surgeon Simulator-style)
    m_camera.lookAt(glm::vec3(0.0f, 0.82f, 0.0f));
    m_camera.setDistance(1.8f);

    // Warmer lab lighting
    m_renderer.lightDirection = glm::vec3(-0.3f, -1.0f, -0.4f);
    m_renderer.lightColor = glm::vec3(1.0f, 0.97f, 0.92f);
    m_renderer.ambientColor = glm::vec3(0.22f, 0.22f, 0.26f);
}

Application::~Application() {
    shutdownImGui();
}

void Application::run() {
    while (!m_window->shouldClose()) {
        m_timer->update();
        m_window->pollEvents();

        if (m_window->isKeyPressed(GLFW_KEY_ESCAPE)) {
            break;
        }

        updateScene();

        // Render 3D scene to full screen
        {
            int winW = m_window->getWidth();
            int winH = m_window->getHeight();
            std::vector<RenderObject> renderObjs;
            collectRenderObjects(renderObjs);
            m_renderer.renderToScreen(renderObjs, m_camera, winW, winH);
        }

        // ImGui HUD overlay on top
        beginFrame();
        handleCameraInput();
        renderGui();
        endFrame();

        m_window->swapBuffers();
    }
}

void Application::handleCameraInput() {
    ImGuiIO& io = ImGui::GetIO();
    if (io.WantCaptureMouse) return;

    if (ImGui::IsMouseDragging(ImGuiMouseButton_Right)) {
        ImVec2 delta = io.MouseDelta;
        m_camera.rotate(delta.x * 0.3f, -delta.y * 0.3f);
    }
    if (ImGui::IsMouseDragging(ImGuiMouseButton_Middle)) {
        ImVec2 delta = io.MouseDelta;
        m_camera.pan(delta.x, delta.y);
    }
    if (io.MouseWheel != 0.0f) {
        m_camera.zoom(io.MouseWheel);
    }
}

void Application::goToDashboard() {
    m_state = AppState::Dashboard;
    m_selectedExperimentType.clear();
    m_simEngine.cancel();
    m_selectedAnimalIdx = 0;
}

void Application::goToSetup(const std::string& experimentType) {
    m_state = AppState::ExperimentSetup;
    m_selectedExperimentType = experimentType;
}

void Application::goToSimulation() {
    m_state = AppState::Simulation;
}

void Application::goToInteractive() {
    m_state = AppState::Interactive;
}

void Application::goToResults() {
    if (m_simEngine.getState() == SimState::Complete) {
        m_lastResult = m_simEngine.getResult();
    }
    m_state = AppState::Results;
}

void Application::goToDrugEditor() {
    m_state = AppState::DrugEditor;
}

void Application::startSimulation() {
    if (m_selectedExperimentType == "toxicology") {
        m_experimentConfig.type = ExperimentType::Toxicology;
    } else if (m_selectedExperimentType == "pharmacokinetics") {
        m_experimentConfig.type = ExperimentType::Pharmacokinetics;
    } else if (m_selectedExperimentType == "behavioral") {
        m_experimentConfig.type = ExperimentType::Behavioral;
    } else if (m_selectedExperimentType == "drug_efficacy") {
        m_experimentConfig.type = ExperimentType::DrugEfficacy;
    } else if (m_selectedExperimentType == "skin_irritation") {
        m_experimentConfig.type = ExperimentType::SkinIrritation;
    }

    m_experimentConfig.doselevels.clear();

    // Only 1 animal visible on the bench
    m_scene.setupAnimals(m_experimentConfig.species, 1);
    m_selectedAnimalIdx = 0;

    std::unique_ptr<ExperimentBase> exp;
    switch (m_experimentConfig.type) {
    case ExperimentType::Toxicology:
        exp = std::make_unique<ToxicologyExperiment>(m_experimentConfig);
        break;
    case ExperimentType::Pharmacokinetics:
        exp = std::make_unique<PKExperiment>(m_experimentConfig);
        break;
    case ExperimentType::Behavioral:
        exp = std::make_unique<BehavioralExperiment>(m_experimentConfig);
        break;
    case ExperimentType::DrugEfficacy:
        exp = std::make_unique<DrugEfficacyExperiment>(m_experimentConfig);
        break;
    case ExperimentType::SkinIrritation:
        exp = std::make_unique<SkinIrritationExperiment>(m_experimentConfig);
        break;
    }

    m_simEngine.start(std::move(exp));
    m_nextProcAnimTime = 0.5f;
    m_procAnimAnimalIdx = 0;
    m_state = AppState::Simulation;
}

void Application::startInteractive() {
    if (m_selectedExperimentType == "toxicology") {
        m_experimentConfig.type = ExperimentType::Toxicology;
    } else if (m_selectedExperimentType == "pharmacokinetics") {
        m_experimentConfig.type = ExperimentType::Pharmacokinetics;
    } else if (m_selectedExperimentType == "behavioral") {
        m_experimentConfig.type = ExperimentType::Behavioral;
    } else if (m_selectedExperimentType == "drug_efficacy") {
        m_experimentConfig.type = ExperimentType::DrugEfficacy;
    } else if (m_selectedExperimentType == "skin_irritation") {
        m_experimentConfig.type = ExperimentType::SkinIrritation;
    }

    // Only 1 animal visible on the bench
    m_scene.setupAnimals(m_experimentConfig.species, 1);
    m_selectedAnimalIdx = 0;

    m_interactiveCtrl.setup(m_experimentConfig);
    m_state = AppState::Interactive;
}

void Application::initImGui() {
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();

    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

    ImGui_ImplGlfw_InitForOpenGL(m_window->getHandle(), true);
    ImGui_ImplOpenGL3_Init("#version 330 core");

    GuiManager::applyTheme();
}

void Application::shutdownImGui() {
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
}

void Application::beginFrame() {
    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();
}

void Application::endFrame() {
    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void Application::renderGui() {
    switch (m_state) {
    case AppState::Dashboard:
        GuiManager::renderDashboard(*this);
        break;
    case AppState::ExperimentSetup:
        GuiManager::renderSetup(*this);
        break;
    case AppState::Simulation:
        GuiManager::renderSimulation(*this);
        break;
    case AppState::Interactive:
        GuiManager::renderInteractive(*this);
        break;
    case AppState::Results:
        GuiManager::renderResults(*this);
        break;
    case AppState::DrugEditor:
        GuiManager::renderDrugEditor(*this);
        break;
    }

    GuiManager::renderStatusBar(*this);
}

void Application::updateScene() {
    float dt = m_timer->getDeltaTime();
    m_scene.update(dt);

    // During simulation: show selected animal's health + animate
    if (m_state == AppState::Simulation && m_simEngine.getState() == SimState::Running) {
        auto animalInfos = m_simEngine.getAnimalInfos();
        if (m_selectedAnimalIdx < static_cast<int>(animalInfos.size()) &&
            m_scene.getAnimalCount() > 0) {
            auto& info = animalInfos[m_selectedAnimalIdx];
            m_scene.updateAnimalHealth(0, info.health);
            m_scene.animateAnimal(0, dt, info.vitals.respiratoryRate,
                                  info.health, info.vitals.oxygenSaturation, info.status);
        }

        // Auto-trigger procedure animations on the visible animal
        if (!m_scene.hasActiveAnimation()) {
            m_nextProcAnimTime -= dt;
            if (m_nextProcAnimTime <= 0.0f && m_scene.getAnimalCount() > 0) {
                ProcedureType types[] = {
                    ProcedureType::IPInjection,
                    ProcedureType::OralGavage,
                    ProcedureType::IVInjection,
                    ProcedureType::SCInjection,
                    ProcedureType::BloodSample,
                    ProcedureType::Temperature,
                };
                int typeIdx = m_procAnimAnimalIdx % 6;
                m_scene.playProcedure(types[typeIdx], 0);
                m_procAnimAnimalIdx++;
                m_nextProcAnimTime = 4.0f;
            }
        }
    }

    // During interactive mode: show selected animal's health + animate
    if (m_state == AppState::Interactive) {
        auto& ctrl = m_interactiveCtrl;
        if (m_selectedAnimalIdx < ctrl.getAnimalCount() && m_scene.getAnimalCount() > 0) {
            auto& state = ctrl.getAnimal(m_selectedAnimalIdx).getState();
            m_scene.updateAnimalHealth(0, state.overallHealth);
            m_scene.animateAnimal(0, dt, state.vitals.respiratoryRate,
                                  state.overallHealth, state.vitals.oxygenSaturation,
                                  state.status);
        }
    }

    // Dashboard/setup: gentle idle breathing for the preview animal
    if ((m_state == AppState::Dashboard || m_state == AppState::ExperimentSetup ||
         m_state == AppState::DrugEditor) && m_scene.getAnimalCount() > 0) {
        m_scene.animateAnimal(0, dt, 85.0f, 100.0f, 98.5f, AnimalStatus::Alive);
    }
}

void Application::collectRenderObjects(std::vector<RenderObject>& out) const {
    m_scene.getRenderObjects(out);
}

} // namespace animsim
