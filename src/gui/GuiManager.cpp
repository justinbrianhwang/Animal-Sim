#include "gui/GuiManager.h"
#include "core/Application.h"
#include "core/Timer.h"
#include "renderer/Renderer.h"
#include "renderer/Camera.h"
#include "renderer/Framebuffer.h"
#include "simulation/SimulationEngine.h"
#include "simulation/Types.h"
#include "interactive/Controller.h"
#include "simulation/DrugCompound.h"
#include "simulation/DrugRegistry.h"

#include <glad/gl.h>
#include <imgui.h>
#include <implot.h>
#include <cmath>
#include <cstdio>
#include <algorithm>

namespace animsim {

// ─── HUD Helpers ────────────────────────────────────────────────────────────

static ImGuiWindowFlags hudFlags() {
    return ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
           ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoDocking |
           ImGuiWindowFlags_NoFocusOnAppearing;
}

static void pushHudStyle(float alpha = 0.82f) {
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 10.0f);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 1.5f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.06f, 0.06f, 0.10f, alpha));
    ImGui::PushStyleColor(ImGuiCol_TitleBg, ImVec4(0.08f, 0.10f, 0.16f, alpha));
    ImGui::PushStyleColor(ImGuiCol_TitleBgActive, ImVec4(0.10f, 0.14f, 0.22f, alpha));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.25f, 0.35f, 0.55f, 0.5f));
}

static void popHudStyle() {
    ImGui::PopStyleColor(4);
    ImGui::PopStyleVar(2);
}

// Health bar helper
static void drawHealthBar(float healthPercent) {
    float h = healthPercent / 100.0f;
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram,
        h > 0.8f ? ImVec4(0.2f, 0.8f, 0.3f, 1.0f) :
        h > 0.5f ? ImVec4(0.8f, 0.7f, 0.2f, 1.0f) :
                   ImVec4(0.8f, 0.2f, 0.2f, 1.0f));
    char label[32];
    std::snprintf(label, sizeof(label), "%.0f%%", healthPercent);
    ImGui::ProgressBar(h, ImVec2(-1, 16), label);
    ImGui::PopStyleColor();
}

static ImVec4 healthColor(float h) {
    return h > 80 ? ImVec4(0.2f, 0.9f, 0.4f, 1.0f) :
           h > 50 ? ImVec4(0.9f, 0.8f, 0.2f, 1.0f) :
                    ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
}

// Vital sign row with delta indicator (use inside BeginTable with 3 columns)
static void vitalRow(const char* label, float current, float previous, bool showDelta,
                     const char* fmt, bool higherIsBad = true, float threshold = 0.1f) {
    ImGui::TableNextRow();
    ImGui::TableSetColumnIndex(0);
    ImGui::TextColored(ImVec4(0.60f, 0.63f, 0.72f, 1.0f), "%s", label);
    ImGui::TableSetColumnIndex(1);
    ImGui::Text(fmt, current);
    ImGui::TableSetColumnIndex(2);
    if (showDelta) {
        float delta = current - previous;
        if (std::abs(delta) > threshold) {
            bool worsening = (higherIsBad && delta > 0) || (!higherIsBad && delta < 0);
            ImVec4 color = worsening ? ImVec4(1.0f, 0.35f, 0.35f, 1.0f) :
                                       ImVec4(0.35f, 1.0f, 0.55f, 1.0f);
            char buf[32];
            std::snprintf(buf, sizeof(buf), "%+.1f", delta);
            ImGui::TextColored(color, "%s", buf);
        }
    }
}

// Organ health bar with delta
static void organHealthRow(const char* name, float health, float prevHealth, bool showDelta) {
    ImGui::Text("%-10s", name);
    ImGui::SameLine(78);
    float h = health / 100.0f;
    ImVec4 barColor = h > 0.8f ? ImVec4(0.2f, 0.8f, 0.3f, 1.0f) :
                      h > 0.5f ? ImVec4(0.8f, 0.7f, 0.2f, 1.0f) :
                                 ImVec4(0.8f, 0.2f, 0.2f, 1.0f);
    ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
    char label[16];
    std::snprintf(label, sizeof(label), "%.0f%%", health);
    ImGui::ProgressBar(h, ImVec2(110, 14), label);
    ImGui::PopStyleColor();

    if (showDelta) {
        float delta = health - prevHealth;
        if (std::abs(delta) > 0.1f) {
            ImGui::SameLine();
            ImVec4 color = delta < 0 ? ImVec4(1.0f, 0.35f, 0.35f, 1.0f) :
                                       ImVec4(0.35f, 1.0f, 0.55f, 1.0f);
            char buf[16];
            std::snprintf(buf, sizeof(buf), "%+.0f", delta);
            ImGui::TextColored(color, "%s", buf);
        }
    }
}

// ─── Theme ──────────────────────────────────────────────────────────────────

void GuiManager::applyTheme() {
    ImGuiStyle& style = ImGui::GetStyle();

    // Rounding
    style.WindowRounding = 10.0f;
    style.FrameRounding = 5.0f;
    style.PopupRounding = 5.0f;
    style.ScrollbarRounding = 5.0f;
    style.GrabRounding = 4.0f;
    style.TabRounding = 5.0f;
    style.ChildRounding = 6.0f;

    // Spacing
    style.WindowPadding = ImVec2(10, 8);
    style.FramePadding = ImVec2(8, 4);
    style.ItemSpacing = ImVec2(8, 5);
    style.ItemInnerSpacing = ImVec2(6, 4);
    style.IndentSpacing = 18.0f;

    // Sizes
    style.ScrollbarSize = 12.0f;
    style.GrabMinSize = 10.0f;
    style.WindowBorderSize = 1.5f;

    // Semi-transparent dark lab theme
    ImVec4* c = style.Colors;
    c[ImGuiCol_WindowBg]            = ImVec4(0.06f, 0.06f, 0.10f, 0.82f);
    c[ImGuiCol_ChildBg]             = ImVec4(0.08f, 0.08f, 0.12f, 0.50f);
    c[ImGuiCol_PopupBg]             = ImVec4(0.08f, 0.08f, 0.12f, 0.94f);
    c[ImGuiCol_Border]              = ImVec4(0.25f, 0.30f, 0.45f, 0.50f);
    c[ImGuiCol_FrameBg]             = ImVec4(0.12f, 0.12f, 0.18f, 0.85f);
    c[ImGuiCol_FrameBgHovered]      = ImVec4(0.18f, 0.18f, 0.26f, 0.90f);
    c[ImGuiCol_FrameBgActive]       = ImVec4(0.24f, 0.24f, 0.34f, 0.95f);
    c[ImGuiCol_TitleBg]             = ImVec4(0.08f, 0.10f, 0.16f, 0.85f);
    c[ImGuiCol_TitleBgActive]       = ImVec4(0.10f, 0.14f, 0.22f, 0.90f);
    c[ImGuiCol_MenuBarBg]           = ImVec4(0.10f, 0.10f, 0.14f, 0.85f);
    c[ImGuiCol_ScrollbarBg]         = ImVec4(0.06f, 0.06f, 0.10f, 0.40f);
    c[ImGuiCol_ScrollbarGrab]       = ImVec4(0.30f, 0.30f, 0.42f, 0.80f);
    c[ImGuiCol_ScrollbarGrabHovered]= ImVec4(0.36f, 0.36f, 0.50f, 0.90f);
    c[ImGuiCol_ScrollbarGrabActive] = ImVec4(0.42f, 0.42f, 0.58f, 1.00f);
    c[ImGuiCol_CheckMark]           = ImVec4(0.40f, 0.75f, 0.95f, 1.00f);
    c[ImGuiCol_SliderGrab]          = ImVec4(0.40f, 0.55f, 0.80f, 0.90f);
    c[ImGuiCol_SliderGrabActive]    = ImVec4(0.50f, 0.65f, 0.90f, 1.00f);
    c[ImGuiCol_Button]              = ImVec4(0.18f, 0.28f, 0.48f, 0.85f);
    c[ImGuiCol_ButtonHovered]       = ImVec4(0.25f, 0.38f, 0.60f, 0.90f);
    c[ImGuiCol_ButtonActive]        = ImVec4(0.32f, 0.46f, 0.70f, 1.00f);
    c[ImGuiCol_Header]              = ImVec4(0.18f, 0.22f, 0.35f, 0.85f);
    c[ImGuiCol_HeaderHovered]       = ImVec4(0.25f, 0.32f, 0.48f, 0.90f);
    c[ImGuiCol_HeaderActive]        = ImVec4(0.32f, 0.40f, 0.58f, 1.00f);
    c[ImGuiCol_Separator]           = ImVec4(0.25f, 0.28f, 0.38f, 0.50f);
    c[ImGuiCol_Tab]                 = ImVec4(0.12f, 0.14f, 0.22f, 0.85f);
    c[ImGuiCol_TabHovered]          = ImVec4(0.28f, 0.36f, 0.54f, 0.90f);
    c[ImGuiCol_TabActive]           = ImVec4(0.22f, 0.30f, 0.48f, 1.00f);
    c[ImGuiCol_TextSelectedBg]      = ImVec4(0.30f, 0.45f, 0.70f, 0.35f);
    c[ImGuiCol_Text]                = ImVec4(0.90f, 0.92f, 0.96f, 1.00f);
    c[ImGuiCol_TextDisabled]        = ImVec4(0.45f, 0.48f, 0.55f, 1.00f);
    c[ImGuiCol_PlotLines]           = ImVec4(0.40f, 0.75f, 0.95f, 1.00f);
    c[ImGuiCol_PlotHistogram]       = ImVec4(0.40f, 0.75f, 0.55f, 1.00f);
    c[ImGuiCol_TableHeaderBg]       = ImVec4(0.12f, 0.14f, 0.22f, 0.85f);
    c[ImGuiCol_TableBorderStrong]   = ImVec4(0.20f, 0.22f, 0.32f, 0.80f);
    c[ImGuiCol_TableBorderLight]    = ImVec4(0.16f, 0.18f, 0.26f, 0.60f);
    c[ImGuiCol_TableRowBg]          = ImVec4(0.00f, 0.00f, 0.00f, 0.00f);
    c[ImGuiCol_TableRowBgAlt]       = ImVec4(1.00f, 1.00f, 1.00f, 0.02f);

    // ImPlot theme
    ImPlot::CreateContext();
    ImPlot::StyleColorsDark();
}

// ─── Dockspace (no-op: we use HUD overlay now) ─────────────────────────────

void GuiManager::beginDockspace() {}
void GuiManager::endDockspace() {}

// ─── Viewport (no-op: 3D renders directly to screen) ───────────────────────

void GuiManager::renderViewport(Application&, const char*) {}

// ─── Dashboard Card ─────────────────────────────────────────────────────────

bool GuiManager::experimentCard(const char* title, const char* description,
                                 const char* icon, float width) {
    bool clicked = false;

    ImGui::PushStyleVar(ImGuiStyleVar_ChildRounding, 8.0f);
    ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.10f, 0.10f, 0.16f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.24f, 0.30f, 0.45f, 0.60f));

    ImGui::BeginChild(title, ImVec2(width, 160), ImGuiChildFlags_Borders);

    ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 8);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);

    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.75f, 0.95f, 1.0f));
    ImGui::TextUnformatted(icon);
    ImGui::PopStyleColor();

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.95f, 0.95f, 0.98f, 1.0f));
    ImGui::TextUnformatted(title);
    ImGui::PopStyleColor();

    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.55f, 0.58f, 0.68f, 1.0f));
    ImGui::TextWrapped("%s", description);
    ImGui::PopStyleColor();

    ImGui::SetCursorPosY(ImGui::GetWindowHeight() - 36);
    ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 8);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.18f, 0.32f, 0.52f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.25f, 0.42f, 0.65f, 0.95f));
    if (ImGui::Button("Start", ImVec2(width - 24, 26))) {
        clicked = true;
    }
    ImGui::PopStyleColor(2);

    ImGui::EndChild();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar();

    return clicked;
}

// ─── Dashboard ──────────────────────────────────────────────────────────────

void GuiManager::renderDashboard(Application& app) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float panelW = std::min(740.0f, vp->WorkSize.x - 40.0f);
    float panelH = std::min(540.0f, vp->WorkSize.y - 80.0f);

    ImGui::SetNextWindowPos(ImVec2(
        vp->WorkPos.x + (vp->WorkSize.x - panelW) * 0.5f,
        vp->WorkPos.y + (vp->WorkSize.y - panelH) * 0.38f
    ), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));

    pushHudStyle(0.88f);
    ImGui::Begin("##Dashboard", nullptr, hudFlags() | ImGuiWindowFlags_NoTitleBar);

    // Centered title
    {
        const char* title = "Animal Experiment Simulator";
        float tw = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((panelW - tw) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.4f, 0.75f, 0.95f, 1.0f));
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
    }
    {
        const char* sub = "Select an experiment to begin";
        float sw = ImGui::CalcTextSize(sub).x;
        ImGui::SetCursorPosX((panelW - sw) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.50f, 0.52f, 0.60f, 1.0f));
        ImGui::TextUnformatted(sub);
        ImGui::PopStyleColor();
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float cardWidth = 210.0f;
    float avail = ImGui::GetContentRegionAvail().x;
    int cols = std::max(1, static_cast<int>(avail / (cardWidth + 10.0f)));

    struct ExperimentInfo {
        const char* type;
        const char* title;
        const char* description;
        const char* icon;
    };

    ExperimentInfo experiments[] = {
        {"toxicology", "Toxicology",
         "LD50, dose-response analysis, chronic toxicity with organ tracking.",
         "[TOX]"},
        {"pharmacokinetics", "Pharmacokinetics",
         "1/2-compartment PK, concentration-time profiles, ADME parameters.",
         "[PK]"},
        {"behavioral", "Behavioral",
         "Morris Water Maze, T-Maze, fear conditioning, open field tests.",
         "[BEH]"},
        {"drug_efficacy", "Drug Efficacy",
         "Tumor growth, infection models, anti-inflammatory efficacy.",
         "[EFF]"},
        {"skin_irritation", "Skin Irritation",
         "Draize scoring, LLNA sensitization testing.",
         "[SKIN]"},
    };

    int col = 0;
    for (auto& exp : experiments) {
        if (col > 0) ImGui::SameLine();
        if (experimentCard(exp.title, exp.description, exp.icon, cardWidth)) {
            app.goToSetup(exp.type);
        }
        col++;
        if (col >= cols) col = 0;
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Drug Lab button
    {
        float btnW = 220.0f;
        ImGui::SetCursorPosX((panelW - btnW) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.18f, 0.48f, 0.90f));
        ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.45f, 0.25f, 0.60f, 0.95f));
        ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.55f, 0.32f, 0.70f, 1.0f));
        if (ImGui::Button("[DRUG] Drug Compound Lab", ImVec2(btnW, 36))) {
            app.goToDrugEditor();
        }
        ImGui::PopStyleColor(3);
        float tw = ImGui::CalcTextSize("Define custom drugs for experiments").x;
        ImGui::SetCursorPosX((panelW - tw) * 0.5f);
        ImGui::TextColored(ImVec4(0.50f, 0.52f, 0.60f, 1.0f),
                           "Define custom drugs for experiments");
    }

    ImGui::End();
    popHudStyle();
}

// ─── Setup ──────────────────────────────────────────────────────────────────

void GuiManager::renderSetup(Application& app) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float panelW = std::min(520.0f, vp->WorkSize.x - 40.0f);
    float panelH = std::min(580.0f, vp->WorkSize.y - 80.0f);

    ImGui::SetNextWindowPos(ImVec2(
        vp->WorkPos.x + (vp->WorkSize.x - panelW) * 0.5f,
        vp->WorkPos.y + (vp->WorkSize.y - panelH) * 0.38f
    ), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));

    pushHudStyle(0.88f);
    ImGui::Begin("Experiment Setup", nullptr, hudFlags());

    auto& config = app.getExperimentConfig();
    const auto& expType = app.getSelectedExperimentType();

    ImGui::TextColored(ImVec4(0.4f, 0.75f, 0.95f, 1.0f), "Type: %s", expType.c_str());
    ImGui::Separator();
    ImGui::Spacing();

    // Species selection
    static int speciesIdx = 1;
    const char* speciesNames[] = {"Mouse", "Rat", "Rabbit", "Guinea Pig", "Dog", "Monkey"};
    if (ImGui::Combo("Species", &speciesIdx, speciesNames, 6)) {
        config.species = speciesFromIndex(speciesIdx);
    }

    ImGui::SliderInt("Groups", &config.numGroups, 2, 10);
    ImGui::SliderInt("Animals/Group", &config.animalsPerGroup, 3, 20);
    ImGui::SliderFloat("Duration (h)", &config.durationHours, 1.0f, 720.0f, "%.0f");
    ImGui::SliderFloat("Time Step (h)", &config.timeStepHours, 0.1f, 24.0f, "%.1f");

    // Drug selection
    {
        auto& drugs = DrugRegistry::instance().getAllDrugs();
        static int drugIdx = 0;
        // Sync drugIdx with config.drugId
        for (int i = 0; i < static_cast<int>(drugs.size()); i++) {
            if (drugs[i].id == config.drugId) { drugIdx = i; break; }
        }
        std::vector<const char*> drugNames;
        for (auto& d : drugs) drugNames.push_back(d.name.c_str());
        if (ImGui::Combo("Drug Compound", &drugIdx, drugNames.data(),
                         static_cast<int>(drugNames.size()))) {
            config.drugId = drugs[drugIdx].id;
        }
        // Hint about the selected drug
        auto& selDrug = DrugRegistry::instance().getDrug(config.drugId);
        ImGui::TextColored(ImVec4(0.5f, 0.52f, 0.6f, 0.8f),
                           "  LD50: %.0f mg/kg | t1/2: %.1fh | Primary: %s",
                           selDrug.ld50, selDrug.halfLifeHours, selDrug.category.c_str());
    }

    ImGui::Separator();
    ImGui::Spacing();

    // Type-specific parameters
    if (expType == "toxicology") {
        const char* toxTypes[] = {"LD50 Study", "Dose-Response", "Chronic Toxicity"};
        static int toxIdx = 0;
        if (ImGui::Combo("Study Type", &toxIdx, toxTypes, 3)) {
            const char* toxKeys[] = {"ld50", "dose_response", "chronic"};
            config.toxStudyType = toxKeys[toxIdx];
        }
        const char* routes[] = {"Oral", "IV", "IP", "SC", "Dermal"};
        static int routeIdx = 0;
        if (ImGui::Combo("Route", &routeIdx, routes, 5)) {
            const char* routeKeys[] = {"oral", "iv", "ip", "sc", "dermal"};
            config.route = routeKeys[routeIdx];
        }
    } else if (expType == "pharmacokinetics") {
        ImGui::SliderInt("Compartments", &config.compartments, 1, 2);
        ImGui::SliderFloat("ka", &config.absorptionRate, 0.1f, 5.0f, "%.2f");
        ImGui::SliderFloat("ke", &config.eliminationRate, 0.01f, 2.0f, "%.3f");
        ImGui::SliderFloat("Vd (L/kg)", &config.volumeOfDistribution, 0.1f, 5.0f, "%.2f");
        const char* routes[] = {"Oral", "IV", "IP", "SC"};
        static int routeIdx = 0;
        if (ImGui::Combo("Route", &routeIdx, routes, 4)) {
            const char* routeKeys[] = {"oral", "iv", "ip", "sc"};
            config.route = routeKeys[routeIdx];
        }
    } else if (expType == "behavioral") {
        const char* paradigms[] = {"Morris Water Maze", "T-Maze", "Fear Conditioning",
                                    "Social Interaction", "Open Field"};
        static int paradigmIdx = 0;
        if (ImGui::Combo("Paradigm", &paradigmIdx, paradigms, 5)) {
            const char* paradigmKeys[] = {"morris_water_maze", "t_maze", "fear_conditioning",
                                           "social_interaction", "open_field"};
            config.paradigm = paradigmKeys[paradigmIdx];
        }
        const char* routes[] = {"Oral", "IP"};
        static int routeIdx = 0;
        if (ImGui::Combo("Drug Route", &routeIdx, routes, 2)) {
            const char* routeKeys[] = {"oral", "ip"};
            config.route = routeKeys[routeIdx];
        }
    } else if (expType == "drug_efficacy") {
        const char* models[] = {"Tumor Growth", "Infection", "Anti-Inflammatory"};
        static int modelIdx = 0;
        if (ImGui::Combo("Disease Model", &modelIdx, models, 3)) {
            const char* modelKeys[] = {"tumor", "infection", "anti_inflammatory"};
            config.diseaseModel = modelKeys[modelIdx];
        }
        const char* routes[] = {"Oral", "IV", "IP", "SC"};
        static int routeIdx = 0;
        if (ImGui::Combo("Route", &routeIdx, routes, 4)) {
            const char* routeKeys[] = {"oral", "iv", "ip", "sc"};
            config.route = routeKeys[routeIdx];
        }
    } else if (expType == "skin_irritation") {
        const char* tests[] = {"Draize Test", "LLNA"};
        static int testIdx = 0;
        if (ImGui::Combo("Test Type", &testIdx, tests, 2)) {
            const char* testKeys[] = {"draize", "llna"};
            config.skinTestType = testKeys[testIdx];
        }
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Buttons centered
    float btnW = 150.0f;
    float totalW = btnW * 3 + 16;
    ImGui::SetCursorPosX((panelW - totalW) * 0.5f);

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.25f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.20f, 0.55f, 0.32f, 0.95f));
    if (ImGui::Button("Run Simulation", ImVec2(btnW, 32))) {
        app.startSimulation();
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.30f, 0.50f, 0.90f));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.40f, 0.62f, 0.95f));
    if (ImGui::Button("Interactive", ImVec2(btnW, 32))) {
        app.startInteractive();
    }
    ImGui::PopStyleColor(2);

    ImGui::SameLine();
    if (ImGui::Button("Back", ImVec2(btnW, 32))) {
        app.goToDashboard();
    }

    ImGui::End();
    popHudStyle();
}

// ─── Simulation ─────────────────────────────────────────────────────────────

void GuiManager::renderSimulation(Application& app) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float margin = 10.0f;
    float statusH = 28.0f;
    float topY = vp->WorkPos.y + 8.0f;
    float usableH = vp->WorkSize.y - statusH - 16.0f;

    auto& engine = app.getSimEngine();
    SimState simState = engine.getState();
    float progress = engine.getProgress();
    float simTime = engine.getCurrentTime();
    auto animals = engine.getAnimalInfos();
    auto groupLabels = engine.getGroupLabels();
    static int selectedAnimalIdx = 0;

    // ── Left Panel: Controls + Animal List ──────────────────────
    float leftW = 270.0f;
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + margin, topY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftW, usableH));

    pushHudStyle(0.82f);
    ImGui::Begin("Controls", nullptr, hudFlags());

    // Progress bar
    char progLabel[64];
    std::snprintf(progLabel, sizeof(progLabel), "%.1f / %.0fh (%.0f%%)",
                  simTime, app.getExperimentConfig().durationHours, progress * 100.0f);
    ImGui::ProgressBar(progress, ImVec2(-1, 0), progLabel);
    ImGui::Spacing();

    // Speed + control buttons
    if (simState == SimState::Running) {
        if (ImGui::Button("Pause", ImVec2(-1, 24))) engine.pause();
    } else if (simState == SimState::Paused) {
        if (ImGui::Button("Resume", ImVec2(-1, 24))) engine.resume();
    }

    static float speed = 1.0f;
    ImGui::SetNextItemWidth(-1);
    if (ImGui::SliderFloat("##speed", &speed, 0.1f, 10.0f, "Speed: %.1fx")) {
        engine.setSpeedMultiplier(speed);
    }

    if (ImGui::Button("Cancel", ImVec2(-1, 22))) {
        engine.cancel();
        app.goToDashboard();
    }

    if (simState == SimState::Complete) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.5f, 1.0f), "Simulation Complete!");
        if (ImGui::Button("View Results", ImVec2(-1, 28))) {
            app.goToResults();
        }
    } else if (simState == SimState::Error) {
        ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Error!");
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.8f, 0.8f), "Animals");

    // Animal table
    if (!animals.empty() && ImGui::BeginTable("animals", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY, ImVec2(0, 0))) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 65);
        ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, 45);
        ImGui::TableSetupColumn("Status");
        ImGui::TableHeadersRow();

        for (int i = 0; i < static_cast<int>(animals.size()); i++) {
            auto& a = animals[i];
            ImGui::TableNextRow();
            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(a.label.c_str(), i == selectedAnimalIdx,
                    ImGuiSelectableFlags_SpanAllColumns))
                selectedAnimalIdx = i;
            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(healthColor(a.health), "%.0f", a.health);
            ImGui::TableSetColumnIndex(2);
            const char* st = animalStatusToString(a.status);
            if (a.status == AnimalStatus::Dead || a.status == AnimalStatus::Euthanized)
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "%s", st);
            else if (a.status == AnimalStatus::Moribund)
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "%s", st);
            else
                ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "%s", st);
        }
        ImGui::EndTable();
    }

    // Sync selection to app for 3D health display
    app.setSelectedAnimalIdx(selectedAnimalIdx);

    ImGui::End();
    popHudStyle();

    // ── Right Panel: Vitals Monitor ─────────────────────────────
    float rightW = 250.0f;
    ImGui::SetNextWindowPos(ImVec2(
        vp->WorkPos.x + vp->WorkSize.x - rightW - margin, topY
    ), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightW, 340.0f));

    pushHudStyle(0.82f);
    ImGui::Begin("Vitals", nullptr, hudFlags());

    if (selectedAnimalIdx >= 0 && selectedAnimalIdx < static_cast<int>(animals.size())) {
        auto& sel = animals[selectedAnimalIdx];
        ImGui::TextColored(ImVec4(0.4f, 0.75f, 0.95f, 1.0f), "%s", sel.label.c_str());

        if (sel.groupIndex < static_cast<int>(groupLabels.size()))
            ImGui::TextColored(ImVec4(0.5f, 0.52f, 0.6f, 1.0f), "Group: %s",
                               groupLabels[sel.groupIndex].c_str());

        ImGui::Separator();
        drawHealthBar(sel.health);
        ImGui::Spacing();

        ImGui::Text("Heart Rate: %.0f bpm", sel.vitals.heartRate);
        ImGui::Text("Temperature: %.1f C", sel.vitals.temperature);
        ImGui::Text("Resp Rate: %.0f /min", sel.vitals.respiratoryRate);
        ImGui::Text("SpO2: %.1f%%", sel.vitals.oxygenSaturation);
        ImGui::Text("BP: %.0f/%.0f mmHg", sel.vitals.bloodPressureSys,
                     sel.vitals.bloodPressureDia);
        ImGui::Text("Weight: %.3f kg", sel.vitals.weight);
        ImGui::Separator();
        ImGui::Text("Drug Conc: %.2f mg/L", sel.drugConcentration);
    } else {
        ImGui::TextDisabled("No animal selected");
    }

    ImGui::End();
    popHudStyle();

    // ── Bottom Right: Live Chart ────────────────────────────────
    if (!groupLabels.empty()) {
        float chartW = rightW;
        float chartH = 180.0f;
        ImGui::SetNextWindowPos(ImVec2(
            vp->WorkPos.x + vp->WorkSize.x - chartW - margin,
            topY + 348
        ), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(chartW, chartH));

        pushHudStyle(0.78f);
        ImGui::Begin("Health##chart", nullptr, hudFlags());

        if (ImPlot::BeginPlot("##groupHealth", ImVec2(-1, -1),
                ImPlotFlags_NoTitle | ImPlotFlags_NoLegend)) {
            ImPlot::SetupAxes(nullptr, nullptr, ImPlotAxisFlags_AutoFit, ImPlotAxisFlags_AutoFit);
            int numGroups = static_cast<int>(groupLabels.size());
            for (int g = 0; g < numGroups; g++) {
                float sum = 0.0f;
                int count = 0;
                for (auto& a : animals) {
                    if (a.groupIndex == g) { sum += a.health; count++; }
                }
                float avg = count > 0 ? sum / count : 0.0f;
                float gf = static_cast<float>(g);
                ImPlot::PlotBars(groupLabels[g].c_str(), &gf, &avg, 1, 0.6);
            }
            ImPlot::EndPlot();
        }

        ImGui::End();
        popHudStyle();
    }
}

// ─── Interactive ────────────────────────────────────────────────────────────

void GuiManager::renderInteractive(Application& app) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float margin = 10.0f;
    float statusH = 28.0f;
    float topY = vp->WorkPos.y + 8.0f;
    float usableH = vp->WorkSize.y - statusH - 16.0f;

    auto& ctrl = app.getInteractiveCtrl();
    auto& config = app.getExperimentConfig();
    static int selectedAnimalIdx = 0;
    static int selectedToolIdx = -1;
    static float doseAmount = 50.0f;
    static std::string lastProcDesc;
    static double lastProcTime = -10.0;

    // ── Left Top: Procedure Tools ───────────────────────────────
    float leftW = 240.0f;
    float toolH = usableH * 0.55f;
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + margin, topY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftW, toolH));

    pushHudStyle(0.85f);
    ImGui::Begin("Procedures", nullptr, hudFlags());

    ImGui::TextColored(ImVec4(0.4f, 0.75f, 0.95f, 1.0f), "%s",
                       experimentTypeToString(config.type));

    // Drug selector
    {
        auto& drugs = DrugRegistry::instance().getAllDrugs();
        static int interDrugIdx = 0;
        for (int i = 0; i < static_cast<int>(drugs.size()); i++) {
            if (drugs[i].id == config.drugId) { interDrugIdx = i; break; }
        }
        std::vector<const char*> drugNames;
        for (auto& d : drugs) drugNames.push_back(d.name.c_str());
        ImGui::SetNextItemWidth(-1);
        if (ImGui::Combo("##Drug", &interDrugIdx, drugNames.data(),
                         static_cast<int>(drugNames.size()))) {
            config.drugId = drugs[interDrugIdx].id;
        }
    }

    ImGui::Separator();

    ImGui::TextColored(ImVec4(0.55f, 0.58f, 0.65f, 0.9f), "Select Tool:");
    auto tools = ctrl.getAvailableTools();
    for (int i = 0; i < static_cast<int>(tools.size()); i++) {
        bool isSel = (selectedToolIdx == i);
        if (isSel) {
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.22f, 0.50f, 0.80f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.28f, 0.55f, 0.85f, 1.0f));
        }
        if (ImGui::Button(procedureTypeToString(tools[i]), ImVec2(-1, 24)))
            selectedToolIdx = (selectedToolIdx == i) ? -1 : i;
        if (isSel) ImGui::PopStyleColor(2);
    }

    if (selectedToolIdx >= 0 && selectedToolIdx < static_cast<int>(tools.size())) {
        ImGui::Spacing();
        ImGui::Separator();
        auto selTool = tools[selectedToolIdx];

        ImGui::TextColored(ImVec4(0.5f, 0.7f, 0.9f, 0.9f), "Selected: %s",
                           procedureTypeToString(selTool));

        bool needsDose = (selTool == ProcedureType::OralGavage ||
                         selTool == ProcedureType::IVInjection ||
                         selTool == ProcedureType::IPInjection ||
                         selTool == ProcedureType::SCInjection ||
                         selTool == ProcedureType::DermalApplication);
        if (needsDose) {
            ImGui::SetNextItemWidth(-1);
            ImGui::SliderFloat("##dose", &doseAmount, 1.0f, 500.0f, "Dose: %.1f mg/kg");

            // Show bioavailability from selected drug compound
            {
                auto& selDrug = DrugRegistry::instance().getDrug(config.drugId);
                const char* routeName = "";
                float bioavail = 1.0f;
                if (selTool == ProcedureType::IVInjection) { routeName = "IV"; bioavail = selDrug.bioavailabilityIV; }
                else if (selTool == ProcedureType::IPInjection) { routeName = "IP"; bioavail = selDrug.bioavailabilityIP; }
                else if (selTool == ProcedureType::SCInjection) { routeName = "SC"; bioavail = selDrug.bioavailabilitySC; }
                else if (selTool == ProcedureType::OralGavage) { routeName = "Oral"; bioavail = selDrug.bioavailabilityOral; }
                else if (selTool == ProcedureType::DermalApplication) { routeName = "Dermal"; bioavail = selDrug.bioavailabilityDermal; }
                char bioInfo[64];
                std::snprintf(bioInfo, sizeof(bioInfo), "%s (%.0f%% bioavailability)", routeName, bioavail * 100.0f);
                ImGui::TextColored(ImVec4(0.45f, 0.48f, 0.55f, 0.9f), "%s", bioInfo);
            }
        }

        ImGui::Spacing();

        if (selectedAnimalIdx >= 0 && selectedAnimalIdx < ctrl.getAnimalCount()) {
            auto& targetAnimal = ctrl.getAnimal(selectedAnimalIdx);
            bool canApply = targetAnimal.isAlive() || selTool == ProcedureType::Necropsy;

            if (!canApply) {
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "Animal is dead");
            }

            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.50f, 0.28f, 0.95f));
            ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(0.22f, 0.60f, 0.35f, 1.0f));
            ImGui::PushStyleColor(ImGuiCol_ButtonActive, ImVec4(0.28f, 0.70f, 0.42f, 1.0f));
            if (ImGui::Button("APPLY PROCEDURE", ImVec2(-1, 34)) && canApply) {
                ctrl.performProcedure(selTool, selectedAnimalIdx, "body", doseAmount);
                app.getScene().playProcedure(selTool, 0);

                char notif[128];
                if (needsDose)
                    std::snprintf(notif, sizeof(notif), "%s: %.1f mg/kg -> %s",
                                  procedureTypeToString(selTool), doseAmount,
                                  targetAnimal.getLabel().c_str());
                else
                    std::snprintf(notif, sizeof(notif), "%s -> %s",
                                  procedureTypeToString(selTool),
                                  targetAnimal.getLabel().c_str());
                lastProcDesc = notif;
                lastProcTime = ImGui::GetTime();
            }
            ImGui::PopStyleColor(3);
        } else {
            ImGui::TextDisabled("No animal selected");
        }

        // Last action feedback (fades out)
        if (!lastProcDesc.empty() && (ImGui::GetTime() - lastProcTime) < 5.0) {
            ImGui::Spacing();
            float alpha = std::max(0.0f, 1.0f - static_cast<float>((ImGui::GetTime() - lastProcTime) / 5.0));
            ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.5f, alpha), ">> %s", lastProcDesc.c_str());
        }
    }

    ImGui::End();
    popHudStyle();

    // ── Left Bottom: Time + Event Log ───────────────────────────
    float timeH = usableH - toolH - 8.0f;
    float timeY = topY + toolH + 8.0f;
    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x + margin, timeY), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(leftW, timeH));

    pushHudStyle(0.80f);
    ImGui::Begin("Time & Log", nullptr, hudFlags());

    float currentTime = ctrl.getCurrentTime();
    float duration = ctrl.getDuration();
    char timeLabel[64];
    std::snprintf(timeLabel, sizeof(timeLabel), "t = %.1f / %.0f h", currentTime, duration);
    ImGui::ProgressBar(ctrl.getProgress(), ImVec2(-1, 0), timeLabel);

    if (ImGui::Button("+1h", ImVec2(50, 22))) ctrl.advanceTime(1.0f);
    ImGui::SameLine();
    if (ImGui::Button("+4h", ImVec2(50, 22))) ctrl.advanceTime(4.0f);
    ImGui::SameLine();
    if (ImGui::Button("+24h", ImVec2(50, 22))) ctrl.advanceTime(24.0f);

    if (ctrl.isComplete())
        ImGui::TextColored(ImVec4(0.3f, 0.95f, 0.5f, 1.0f), "Experiment Complete!");

    ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.15f, 0.45f, 0.25f, 0.90f));
    if (ImGui::Button("Finish & Results", ImVec2(-1, 25))) {
        app.getLastResult() = ctrl.computeResults();
        app.goToResults();
    }
    ImGui::PopStyleColor();

    if (ImGui::Button("Cancel", ImVec2(-1, 22))) {
        app.goToDashboard();
    }

    ImGui::Separator();
    ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.8f, 0.8f), "Event Log");

    auto& events = ctrl.getEventLog();
    ImGui::BeginChild("##events", ImVec2(0, 0), ImGuiChildFlags_None);
    for (int i = static_cast<int>(events.size()) - 1; i >= 0; i--) {
        auto& evt = events[i];
        ImVec4 evtColor;
        if (evt.procedure == ProcedureType::Euthanize || evt.procedure == ProcedureType::Necropsy)
            evtColor = ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
        else if (evt.procedure == ProcedureType::Observe)
            evtColor = ImVec4(0.55f, 0.55f, 0.65f, 1.0f);
        else
            evtColor = ImVec4(0.3f, 0.7f, 0.95f, 1.0f);
        ImGui::TextColored(evtColor, "[%.1fh] %s", evt.timeHours, evt.description.c_str());
    }
    ImGui::EndChild();

    ImGui::End();
    popHudStyle();

    // ── Right Panel: Vitals Monitor ─────────────────────────────
    float rightW = 310.0f;
    ImGui::SetNextWindowPos(ImVec2(
        vp->WorkPos.x + vp->WorkSize.x - rightW - margin, topY
    ), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(rightW, usableH));

    pushHudStyle(0.85f);
    ImGui::Begin("Monitor", nullptr, hudFlags());

    bool showDelta = ctrl.hasProcDelta() && (selectedAnimalIdx == ctrl.getLastProcAnimalIdx());
    AnimalDetailedState prevState;
    if (showDelta) prevState = ctrl.getPreProcState();

    if (selectedAnimalIdx >= 0 && selectedAnimalIdx < ctrl.getAnimalCount()) {
        auto& animal = ctrl.getAnimal(selectedAnimalIdx);
        auto& state = animal.getState();

        // Header: name + status
        ImGui::TextColored(ImVec4(0.4f, 0.85f, 1.0f, 1.0f), "%s", animal.getLabel().c_str());
        ImGui::SameLine();
        ImVec4 stColor = state.status == AnimalStatus::Alive ? ImVec4(0.3f, 0.9f, 0.5f, 1.0f) :
                         state.status == AnimalStatus::Moribund ? ImVec4(0.9f, 0.7f, 0.2f, 1.0f) :
                         ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
        ImGui::TextColored(stColor, "[%s]", animalStatusToString(state.status));

        // Overall health bar
        drawHealthBar(state.overallHealth);
        if (showDelta) {
            float hDelta = state.overallHealth - prevState.overallHealth;
            if (std::abs(hDelta) > 0.1f) {
                ImGui::SameLine();
                ImVec4 c = hDelta < 0 ? ImVec4(1.0f, 0.35f, 0.35f, 1.0f) : ImVec4(0.35f, 1.0f, 0.55f, 1.0f);
                char buf[16];
                std::snprintf(buf, sizeof(buf), "%+.1f%%", hDelta);
                ImGui::TextColored(c, "%s", buf);
            }
        }

        ImGui::Spacing();

        // ── VITAL SIGNS ────────────────────────────────
        ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.85f, 0.9f), "VITAL SIGNS");
        ImGui::Separator();

        if (ImGui::BeginTable("vitals", 3, ImGuiTableFlags_None)) {
            ImGui::TableSetupColumn("Param", ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Delta", ImGuiTableColumnFlags_WidthStretch);

            vitalRow("Heart Rate", state.vitals.heartRate,
                     showDelta ? prevState.vitals.heartRate : state.vitals.heartRate,
                     showDelta, "%.0f bpm", true, 1.0f);
            vitalRow("Temp", state.vitals.temperature,
                     showDelta ? prevState.vitals.temperature : state.vitals.temperature,
                     showDelta, "%.1f C", true, 0.05f);
            vitalRow("Resp Rate", state.vitals.respiratoryRate,
                     showDelta ? prevState.vitals.respiratoryRate : state.vitals.respiratoryRate,
                     showDelta, "%.0f /min", true, 1.0f);
            vitalRow("SpO2", state.vitals.oxygenSaturation,
                     showDelta ? prevState.vitals.oxygenSaturation : state.vitals.oxygenSaturation,
                     showDelta, "%.1f %%", false, 0.1f);
            vitalRow("BP Sys", state.vitals.bloodPressureSys,
                     showDelta ? prevState.vitals.bloodPressureSys : state.vitals.bloodPressureSys,
                     showDelta, "%.0f mmHg", true, 1.0f);
            vitalRow("BP Dia", state.vitals.bloodPressureDia,
                     showDelta ? prevState.vitals.bloodPressureDia : state.vitals.bloodPressureDia,
                     showDelta, "%.0f mmHg", true, 1.0f);
            vitalRow("Weight", state.vitals.weight,
                     showDelta ? prevState.vitals.weight : state.vitals.weight,
                     showDelta, "%.3f kg", false, 0.001f);

            ImGui::EndTable();
        }

        // ── Vital Sparklines ──────────────────────────────
        {
            auto& history = ctrl.getVitalHistory(selectedAnimalIdx);
            if (history.size() >= 2) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.85f, 0.7f), "Trends");

                int count = static_cast<int>(history.size());
                // Prepare data arrays
                std::vector<float> times(count), hr(count), temp(count), spo2(count);
                for (int i = 0; i < count; i++) {
                    times[i] = history[i].time;
                    hr[i] = history[i].heartRate;
                    temp[i] = history[i].temperature;
                    spo2[i] = history[i].spo2;
                }

                ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(2, 2));

                auto sparkline = [&](const char* id, const char* label, float* data, int n,
                                     float yMin, float yMax, ImVec4 color) {
                    if (ImPlot::BeginPlot(id, ImVec2(-1, 35),
                        ImPlotFlags_CanvasOnly | ImPlotFlags_NoInputs)) {
                        ImPlot::SetupAxes(nullptr, nullptr,
                            ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                        ImPlot::SetupAxisLimits(ImAxis_X1, times.front(), times.back(), ImPlotCond_Always);
                        ImPlot::SetupAxisLimits(ImAxis_Y1, yMin, yMax, ImPlotCond_Always);
                        ImPlotSpec spec;
                        spec.LineColor = color;
                        spec.LineWeight = 1.5f;
                        ImPlot::PlotLine(label, times.data(), data, n, spec);
                        ImPlot::EndPlot();
                    }
                };

                sparkline("##hrSpark", "HR", hr.data(), count,
                    *std::min_element(hr.begin(), hr.end()) - 10,
                    *std::max_element(hr.begin(), hr.end()) + 10,
                    ImVec4(1.0f, 0.4f, 0.4f, 1.0f));

                sparkline("##tempSpark", "Temp", temp.data(), count,
                    *std::min_element(temp.begin(), temp.end()) - 0.5f,
                    *std::max_element(temp.begin(), temp.end()) + 0.5f,
                    ImVec4(1.0f, 0.8f, 0.3f, 1.0f));

                sparkline("##spo2Spark", "SpO2", spo2.data(), count,
                    std::max(80.0f, *std::min_element(spo2.begin(), spo2.end()) - 2),
                    100.0f,
                    ImVec4(0.3f, 0.8f, 1.0f, 1.0f));

                ImPlot::PopStyleVar();
            }
        }

        ImGui::Spacing();

        // ── Drug Info (per-drug breakdown) ──────────────────────
        {
            float infoH = 48.0f + std::max(0, static_cast<int>(state.activeDrugs.size())) * 18.0f;
            ImGui::PushStyleColor(ImGuiCol_ChildBg, ImVec4(0.12f, 0.10f, 0.18f, 0.70f));
            ImGui::BeginChild("##druginfo", ImVec2(-1, infoH), ImGuiChildFlags_Borders);
            ImGui::Text("Total Drug Conc:");
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.95f, 1.0f), "%.2f mg/L", state.drugConcentration);
            if (showDelta) {
                float dDelta = state.drugConcentration - prevState.drugConcentration;
                if (std::abs(dDelta) > 0.01f) {
                    ImGui::SameLine();
                    char buf[16];
                    std::snprintf(buf, sizeof(buf), "%+.2f", dDelta);
                    ImGui::TextColored(ImVec4(1.0f, 0.8f, 0.2f, 1.0f), "%s", buf);
                }
            }
            // Per-drug breakdown
            ImVec4 drugColors[] = {
                {0.4f, 0.8f, 1.0f, 1.0f}, {1.0f, 0.6f, 0.3f, 1.0f},
                {0.5f, 1.0f, 0.5f, 1.0f}, {1.0f, 0.5f, 0.8f, 1.0f},
                {0.8f, 0.8f, 0.3f, 1.0f}
            };
            for (int di = 0; di < static_cast<int>(state.activeDrugs.size()); di++) {
                auto& ad = state.activeDrugs[di];
                auto& drug = DrugRegistry::instance().getDrug(ad.drugId);
                ImVec4 col = drugColors[di % 5];
                ImGui::TextColored(col, "  %s: %.2f mg/L (cum: %.1f mg/kg)",
                                   drug.name.c_str(), ad.concentration, ad.cumulativeDose);
            }
            if (state.activeDrugs.empty()) {
                ImGui::TextColored(ImVec4(0.4f, 0.42f, 0.5f, 0.7f), "  No active drugs");
            }
            ImGui::EndChild();
            ImGui::PopStyleColor();
        }

        // ── Drug Concentration-Time Curve ────────────────
        {
            auto& history = ctrl.getVitalHistory(selectedAnimalIdx);
            if (history.size() >= 2) {
                ImGui::Spacing();
                ImGui::TextColored(ImVec4(0.4f, 0.95f, 0.95f, 0.8f), "Drug Conc. Curve");
                int count = static_cast<int>(history.size());
                std::vector<float> times(count), concs(count);
                for (int i = 0; i < count; i++) {
                    times[i] = history[i].time;
                    concs[i] = history[i].totalDrugConc;
                }

                ImPlot::PushStyleVar(ImPlotStyleVar_PlotPadding, ImVec2(4, 4));
                if (ImPlot::BeginPlot("##DrugCurve", ImVec2(-1, 80),
                    ImPlotFlags_CanvasOnly | ImPlotFlags_NoInputs)) {
                    ImPlot::SetupAxes("Time (h)", "mg/L",
                        ImPlotAxisFlags_NoDecorations, ImPlotAxisFlags_NoDecorations);
                    ImPlot::SetupAxisLimits(ImAxis_X1, times.front(), times.back(), ImPlotCond_Always);
                    float maxConc = *std::max_element(concs.begin(), concs.end());
                    ImPlot::SetupAxisLimits(ImAxis_Y1, 0, std::max(1.0f, maxConc * 1.2f), ImPlotCond_Always);

                    // NOAEL band from selected drug
                    auto& selDrug = DrugRegistry::instance().getDrug(config.drugId);
                    if (selDrug.noael < maxConc * 2.0f) {
                        ImPlotSpec shadedSpec;
                        shadedSpec.FillColor = ImVec4(0.2f, 0.8f, 0.3f, 0.08f);
                        float noaelY[2] = {selDrug.noael, selDrug.noael};
                        float xBounds[2] = {times.front(), times.back()};
                        ImPlot::PlotShaded("##noael", xBounds, noaelY, 2, 0.0f, shadedSpec);
                        (void)shadedSpec;
                    }

                    ImPlotSpec lineSpec;
                    lineSpec.LineColor = ImVec4(0.4f, 0.95f, 0.95f, 1.0f);
                    lineSpec.LineWeight = 2.0f;
                    ImPlot::PlotLine("Total", times.data(), concs.data(), count, lineSpec);

                    // Event markers (procedures)
                    ImPlotSpec markerSpec;
                    markerSpec.LineColor = ImVec4(1.0f, 1.0f, 0.3f, 0.5f);
                    auto& events = ctrl.getEventLog();
                    for (auto& evt : events) {
                        if (evt.animalIndex == selectedAnimalIdx) {
                            float evtX[2] = {evt.timeHours, evt.timeHours};
                            float evtY[2] = {0.0f, maxConc * 1.2f};
                            ImPlot::PlotLine("##evt", evtX, evtY, 2, markerSpec);
                        }
                    }

                    ImPlot::EndPlot();
                }
                ImPlot::PopStyleVar();
            }
        }

        ImGui::Spacing();

        // ── ORGAN HEALTH ───────────────────────────────
        ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.85f, 0.9f), "ORGAN HEALTH");
        ImGui::Separator();

        organHealthRow("Heart",   state.organs.heart,      showDelta ? prevState.organs.heart      : state.organs.heart,      showDelta);
        organHealthRow("Brain",   state.organs.brain,      showDelta ? prevState.organs.brain      : state.organs.brain,      showDelta);
        organHealthRow("Lungs",   state.organs.lungs,      showDelta ? prevState.organs.lungs      : state.organs.lungs,      showDelta);
        organHealthRow("Liver",   state.organs.liver,      showDelta ? prevState.organs.liver      : state.organs.liver,      showDelta);
        organHealthRow("Kidney",  state.organs.kidney,     showDelta ? prevState.organs.kidney     : state.organs.kidney,     showDelta);
        organHealthRow("Stomach", state.organs.stomach,    showDelta ? prevState.organs.stomach    : state.organs.stomach,    showDelta);
        organHealthRow("Intest.", state.organs.intestines,  showDelta ? prevState.organs.intestines  : state.organs.intestines, showDelta);
        organHealthRow("Spleen",  state.organs.spleen,     showDelta ? prevState.organs.spleen     : state.organs.spleen,     showDelta);
        organHealthRow("Marrow",  state.organs.bone_marrow, showDelta ? prevState.organs.bone_marrow : state.organs.bone_marrow, showDelta);
        organHealthRow("Skin",    state.organs.skin,       showDelta ? prevState.organs.skin       : state.organs.skin,       showDelta);

        ImGui::Spacing();

        // ── BLOOD CHEMISTRY ────────────────────────────
        ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.85f, 0.9f), "BLOOD CHEMISTRY");
        ImGui::Separator();

        if (ImGui::BeginTable("blood", 3, ImGuiTableFlags_None)) {
            ImGui::TableSetupColumn("Test", ImGuiTableColumnFlags_WidthFixed, 90);
            ImGui::TableSetupColumn("Value", ImGuiTableColumnFlags_WidthFixed, 100);
            ImGui::TableSetupColumn("Delta", ImGuiTableColumnFlags_WidthStretch);

            vitalRow("ALT", state.blood.alt,
                     showDelta ? prevState.blood.alt : state.blood.alt,
                     showDelta, "%.0f U/L", true, 1.0f);
            vitalRow("AST", state.blood.ast,
                     showDelta ? prevState.blood.ast : state.blood.ast,
                     showDelta, "%.0f U/L", true, 1.0f);
            vitalRow("BUN", state.blood.bun,
                     showDelta ? prevState.blood.bun : state.blood.bun,
                     showDelta, "%.1f mg/dL", true, 0.5f);
            vitalRow("Creatinine", state.blood.creatinine,
                     showDelta ? prevState.blood.creatinine : state.blood.creatinine,
                     showDelta, "%.2f mg/dL", true, 0.01f);
            vitalRow("WBC", state.blood.wbc,
                     showDelta ? prevState.blood.wbc : state.blood.wbc,
                     showDelta, "%.1f K/uL", true, 0.5f);
            vitalRow("Glucose", state.blood.glucose,
                     showDelta ? prevState.blood.glucose : state.blood.glucose,
                     showDelta, "%.0f mg/dL", true, 1.0f);

            ImGui::EndTable();
        }
    } else {
        ImGui::TextDisabled("No animal selected");
    }

    ImGui::Spacing();
    ImGui::Separator();

    // ── Animal List ─────────────────────────────────
    ImGui::TextColored(ImVec4(0.4f, 0.6f, 0.85f, 0.9f), "SUBJECTS");

    if (ctrl.getAnimalCount() > 0 && ImGui::BeginTable("iAnimals", 3,
            ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg |
            ImGuiTableFlags_ScrollY, ImVec2(0, 0))) {
        ImGui::TableSetupColumn("ID", ImGuiTableColumnFlags_WidthFixed, 70);
        ImGui::TableSetupColumn("HP", ImGuiTableColumnFlags_WidthFixed, 40);
        ImGui::TableSetupColumn("Status");
        ImGui::TableHeadersRow();

        for (int i = 0; i < ctrl.getAnimalCount(); i++) {
            auto& a = ctrl.getAnimal(i);
            auto& st = a.getState();
            ImGui::TableNextRow();

            ImGui::TableSetColumnIndex(0);
            if (ImGui::Selectable(a.getLabel().c_str(), i == selectedAnimalIdx,
                    ImGuiSelectableFlags_SpanAllColumns))
                selectedAnimalIdx = i;

            ImGui::TableSetColumnIndex(1);
            ImGui::TextColored(healthColor(st.overallHealth), "%.0f", st.overallHealth);

            ImGui::TableSetColumnIndex(2);
            const char* stStr = animalStatusToString(st.status);
            if (st.status == AnimalStatus::Dead || st.status == AnimalStatus::Euthanized)
                ImGui::TextColored(ImVec4(0.9f, 0.3f, 0.3f, 1.0f), "%s", stStr);
            else if (st.status == AnimalStatus::Moribund)
                ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.2f, 1.0f), "%s", stStr);
            else
                ImGui::TextColored(ImVec4(0.3f, 0.9f, 0.5f, 1.0f), "%s", stStr);
        }
        ImGui::EndTable();
    }

    app.setSelectedAnimalIdx(selectedAnimalIdx);

    ImGui::End();
    popHudStyle();

    // ── Floating Notification Toast ─────────────────────────────
    if (!lastProcDesc.empty() && (ImGui::GetTime() - lastProcTime) < 4.0) {
        float elapsed = static_cast<float>(ImGui::GetTime() - lastProcTime);
        float alpha = elapsed < 3.0f ? 0.92f : 0.92f * (1.0f - (elapsed - 3.0f));

        ImGui::SetNextWindowPos(ImVec2(
            vp->WorkPos.x + vp->WorkSize.x * 0.5f - 200,
            vp->WorkPos.y + 40
        ), ImGuiCond_Always);
        ImGui::SetNextWindowSize(ImVec2(400, 0));

        ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 8.0f);
        ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(12, 8));
        ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.08f, 0.15f, 0.10f, alpha));
        ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.3f, 0.8f, 0.4f, alpha * 0.6f));

        ImGui::Begin("##notification", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
            ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoFocusOnAppearing |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_AlwaysAutoResize);

        ImGui::TextColored(ImVec4(0.3f, 1.0f, 0.5f, alpha), ">> %s", lastProcDesc.c_str());

        // Show vital changes summary in notification
        if (ctrl.hasProcDelta() && ctrl.getLastProcAnimalIdx() == selectedAnimalIdx &&
            selectedAnimalIdx < ctrl.getAnimalCount()) {
            auto& curState = ctrl.getAnimal(selectedAnimalIdx).getState();
            auto& prevSt = ctrl.getPreProcState();
            float healthDelta = curState.overallHealth - prevSt.overallHealth;
            float drugDelta = curState.drugConcentration - prevSt.drugConcentration;

            char detail[128];
            std::snprintf(detail, sizeof(detail), "Health: %.0f%% (%+.1f) | Drug: %.1f mg/L (%+.1f)",
                          curState.overallHealth, healthDelta,
                          curState.drugConcentration, drugDelta);
            ImGui::TextColored(ImVec4(0.7f, 0.8f, 0.9f, alpha * 0.9f), "%s", detail);
        }

        ImGui::End();
        ImGui::PopStyleColor(2);
        ImGui::PopStyleVar(2);
    }
}

// ─── Results ────────────────────────────────────────────────────────────────

void GuiManager::renderResults(Application& app) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float panelW = std::min(750.0f, vp->WorkSize.x - 40.0f);
    float panelH = std::min(640.0f, vp->WorkSize.y - 80.0f);

    ImGui::SetNextWindowPos(ImVec2(
        vp->WorkPos.x + (vp->WorkSize.x - panelW) * 0.5f,
        vp->WorkPos.y + (vp->WorkSize.y - panelH) * 0.35f
    ), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));

    pushHudStyle(0.90f);
    ImGui::Begin("Results", nullptr, hudFlags());

    auto& result = app.getLastResult();

    ImGui::TextColored(ImVec4(0.4f, 0.75f, 0.95f, 1.0f), "Experiment Results");
    ImGui::Text("Type: %s", experimentTypeToString(result.type));
    if (!result.summary.empty()) {
        ImGui::Spacing();
        ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.6f, 1.0f), "%s", result.summary.c_str());
    }
    ImGui::Separator();
    ImGui::Spacing();

    if (result.type == ExperimentType::Toxicology) {
        auto& tox = result.toxResult;
        if (!tox.doseLevels.empty() && !tox.mortalityByGroup.empty()) {
            if (ImPlot::BeginPlot("Dose-Response Curve", ImVec2(-1, 250))) {
                int n = static_cast<int>(std::min(tox.doseLevels.size(), tox.mortalityByGroup.size()));
                ImPlot::PlotLine("Mortality %", tox.doseLevels.data(), tox.mortalityByGroup.data(), n);
                ImPlot::PlotScatter("Data", tox.doseLevels.data(), tox.mortalityByGroup.data(), n);
                if (tox.ld50 > 0.0f) {
                    float ld50x[] = {tox.ld50, tox.ld50};
                    float ld50y[] = {0.0f, 100.0f};
                    ImPlot::PlotLine("LD50", ld50x, ld50y, 2);
                }
                ImPlot::EndPlot();
            }
            ImGui::Spacing();

            if (ImGui::BeginTable("toxSum", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Parameter"); ImGui::TableSetupColumn("Value");
                ImGui::TableHeadersRow();
                auto row = [](const char* p, const char* v) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(p);
                    ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(v);
                };
                char buf[128];
                std::snprintf(buf, sizeof(buf), "%.1f mg/kg", tox.ld50); row("LD50", buf);
                std::snprintf(buf, sizeof(buf), "[%.1f - %.1f]", tox.ld50Lower, tox.ld50Upper); row("95% CI", buf);
                std::snprintf(buf, sizeof(buf), "%.2f", tox.hillCoefficient); row("Hill Coefficient", buf);
                std::snprintf(buf, sizeof(buf), "%d", tox.totalAnimals); row("Animals Used", buf);
                std::snprintf(buf, sizeof(buf), "%d / %d (%.1f%%)",
                              tox.totalDeaths, tox.totalAnimals, tox.mortalityRate); row("Mortality", buf);
                ImGui::EndTable();
            }
        }
    } else if (result.type == ExperimentType::Pharmacokinetics) {
        auto& pk = result.pkResult;
        if (!pk.timePoints.empty() && !pk.concentrations.empty()) {
            if (ImPlot::BeginPlot("Concentration-Time", ImVec2(-1, 250))) {
                int n = static_cast<int>(std::min(pk.timePoints.size(), pk.concentrations.size()));
                ImPlot::PlotLine("Avg Conc (mg/L)", pk.timePoints.data(), pk.concentrations.data(), n);
                ImPlot::EndPlot();
            }
        }
        ImGui::Spacing();
        if (ImGui::BeginTable("pkSum", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_RowBg)) {
            ImGui::TableSetupColumn("Parameter"); ImGui::TableSetupColumn("Value");
            ImGui::TableHeadersRow();
            auto row = [](const char* p, const char* v) {
                ImGui::TableNextRow();
                ImGui::TableSetColumnIndex(0); ImGui::TextUnformatted(p);
                ImGui::TableSetColumnIndex(1); ImGui::TextUnformatted(v);
            };
            char buf[64];
            std::snprintf(buf, sizeof(buf), "%.2f mg/L", pk.cmax); row("Cmax", buf);
            std::snprintf(buf, sizeof(buf), "%.1f hours", pk.tmax); row("Tmax", buf);
            std::snprintf(buf, sizeof(buf), "%.1f mg*h/L", pk.auc); row("AUC", buf);
            std::snprintf(buf, sizeof(buf), "%.1f hours", pk.halfLife); row("Half-life", buf);
            std::snprintf(buf, sizeof(buf), "%.3f L/h/kg", pk.clearance); row("Clearance", buf);
            ImGui::EndTable();
        }
    } else {
        ImGui::TextWrapped("%s", result.summary.c_str());
    }

    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    float btnRow = ImGui::CalcTextSize("Export CSV").x + ImGui::CalcTextSize("Export JSON").x +
                   ImGui::CalcTextSize("New Experiment").x + 60;
    ImGui::SetCursorPosX((panelW - btnRow) * 0.5f);

    if (ImGui::Button("Export CSV")) {}
    ImGui::SameLine();
    if (ImGui::Button("Export JSON")) {}
    ImGui::SameLine();
    if (ImGui::Button("New Experiment")) {
        app.goToDashboard();
    }

    ImGui::End();
    popHudStyle();
}

// ─── Drug Editor ────────────────────────────────────────────────────────────

void GuiManager::renderDrugEditor(Application& app) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float panelW = std::min(900.0f, vp->WorkSize.x - 40.0f);
    float panelH = std::min(700.0f, vp->WorkSize.y - 60.0f);

    ImGui::SetNextWindowPos(ImVec2(
        vp->WorkPos.x + (vp->WorkSize.x - panelW) * 0.5f,
        vp->WorkPos.y + (vp->WorkSize.y - panelH) * 0.4f
    ), ImGuiCond_Always);
    ImGui::SetNextWindowSize(ImVec2(panelW, panelH));

    pushHudStyle(0.92f);
    ImGui::Begin("##DrugEditor", nullptr, hudFlags() | ImGuiWindowFlags_NoTitleBar);

    // State
    static int selectedDrugIdx = 0;
    static DrugCompound editDrug;
    static bool editing = false;
    static bool createNew = false;
    static char nameBuffer[128] = "";
    static char idBuffer[64] = "";
    static char categoryBuffer[64] = "";
    static char feedbackMsg[128] = "";
    static float feedbackTimer = 0.0f;

    auto& registry = DrugRegistry::instance();
    auto& drugs = registry.getAllDrugs();

    // Header
    {
        const char* title = "Drug Compound Lab";
        float tw = ImGui::CalcTextSize(title).x;
        ImGui::SetCursorPosX((panelW - tw) * 0.5f);
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.6f, 0.4f, 0.9f, 1.0f));
        ImGui::TextUnformatted(title);
        ImGui::PopStyleColor();
    }
    ImGui::Spacing();
    ImGui::Separator();
    ImGui::Spacing();

    // Left: Drug list (200px) | Right: Drug details
    float listW = 200.0f;

    // === LEFT: Drug List ===
    ImGui::BeginChild("##DrugList", ImVec2(listW, -36), ImGuiChildFlags_Borders);

    ImGui::TextColored(ImVec4(0.5f, 0.55f, 0.7f, 1.0f), "COMPOUNDS");
    ImGui::Separator();

    for (int i = 0; i < static_cast<int>(drugs.size()); i++) {
        auto& d = drugs[i];
        ImVec4 color = d.isBuiltIn ? ImVec4(0.5f, 0.7f, 0.9f, 1.0f)
                                   : ImVec4(0.7f, 0.5f, 0.9f, 1.0f);

        ImGui::PushStyleColor(ImGuiCol_Text, color);
        bool selected = (i == selectedDrugIdx);
        if (ImGui::Selectable(d.name.c_str(), selected)) {
            selectedDrugIdx = i;
            editing = false;
            createNew = false;
        }
        ImGui::PopStyleColor();

        // Category hint
        ImGui::SameLine(listW - 50);
        ImGui::TextColored(ImVec4(0.4f, 0.42f, 0.5f, 0.7f), "%s",
            d.isBuiltIn ? "[built-in]" : "[custom]");
    }

    ImGui::EndChild();
    ImGui::SameLine();

    // === RIGHT: Drug Details / Editor ===
    ImGui::BeginChild("##DrugDetails", ImVec2(0, -36), ImGuiChildFlags_Borders);

    if (selectedDrugIdx >= 0 && selectedDrugIdx < static_cast<int>(drugs.size())) {
        auto& drug = drugs[selectedDrugIdx];

        if (!editing && !createNew) {
            // === VIEW MODE ===
            ImGui::TextColored(ImVec4(0.9f, 0.9f, 0.95f, 1.0f), "%s", drug.name.c_str());
            ImGui::SameLine();
            ImGui::TextColored(ImVec4(0.5f, 0.52f, 0.6f, 1.0f), "(%s)", drug.category.c_str());
            ImGui::Separator();
            ImGui::Spacing();

            // PK Parameters
            ImGui::TextColored(ImVec4(0.4f, 0.75f, 0.95f, 1.0f), "Pharmacokinetics");
            if (ImGui::BeginTable("##PKTable", 2, ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthFixed, 200);
                ImGui::TableSetupColumn("Value");

                auto pkRow = [](const char* label, const char* fmt, float val) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(ImVec4(0.6f, 0.63f, 0.72f, 1.0f), "%s", label);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text(fmt, val);
                };

                pkRow("Molecular Weight", "%.1f g/mol", drug.molecularWeight);
                pkRow("Half-Life", "%.2f hours", drug.halfLifeHours);
                pkRow("Volume of Distribution", "%.2f L/kg", drug.volumeOfDistribution);
                pkRow("Bioavailability (Oral)", "%.0f%%", drug.bioavailabilityOral * 100.0f);
                pkRow("Bioavailability (IV)", "%.0f%%", drug.bioavailabilityIV * 100.0f);
                pkRow("Bioavailability (IP)", "%.0f%%", drug.bioavailabilityIP * 100.0f);
                pkRow("Bioavailability (SC)", "%.0f%%", drug.bioavailabilitySC * 100.0f);
                pkRow("Bioavailability (Dermal)", "%.0f%%", drug.bioavailabilityDermal * 100.0f);

                ImGui::EndTable();
            }

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.4f, 1.0f), "Toxicity Profile");

            // Organ toxicity bars
            auto toxBar = [](const char* organ, float weight) {
                ImGui::Text("%-14s", organ);
                ImGui::SameLine(110);
                ImVec4 barColor = weight < 0.3f ? ImVec4(0.3f, 0.8f, 0.4f, 1.0f) :
                                  weight < 0.6f ? ImVec4(0.8f, 0.7f, 0.2f, 1.0f) :
                                                  ImVec4(0.9f, 0.3f, 0.3f, 1.0f);
                ImGui::PushStyleColor(ImGuiCol_PlotHistogram, barColor);
                char label[16];
                std::snprintf(label, sizeof(label), "%.0f%%", weight * 100.0f);
                ImGui::ProgressBar(weight, ImVec2(200, 14), label);
                ImGui::PopStyleColor();
            };

            toxBar("Liver", drug.hepatotoxicity);
            toxBar("Kidney", drug.nephrotoxicity);
            toxBar("Heart", drug.cardiotoxicity);
            toxBar("Brain", drug.neurotoxicity);
            toxBar("Lungs", drug.pulmonaryToxicity);
            toxBar("GI Tract", drug.giToxicity);
            toxBar("Bone Marrow", drug.hematotoxicity);
            toxBar("Skin", drug.dermalToxicity);

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Dose-Response");
            if (ImGui::BeginTable("##DoseTable", 2, ImGuiTableFlags_RowBg)) {
                ImGui::TableSetupColumn("Parameter", ImGuiTableColumnFlags_WidthFixed, 200);
                ImGui::TableSetupColumn("Value");
                auto drRow = [](const char* label, const char* fmt, float val) {
                    ImGui::TableNextRow();
                    ImGui::TableSetColumnIndex(0);
                    ImGui::TextColored(ImVec4(0.6f, 0.63f, 0.72f, 1.0f), "%s", label);
                    ImGui::TableSetColumnIndex(1);
                    ImGui::Text(fmt, val);
                };
                drRow("Therapeutic Dose", "%.1f mg/kg", drug.therapeuticDose);
                drRow("NOAEL", "%.1f mg/kg", drug.noael);
                drRow("LD50", "%.1f mg/kg", drug.ld50);
                ImGui::EndTable();
            }

            // Edit / Delete buttons (only for custom drugs)
            ImGui::Spacing();
            if (!drug.isBuiltIn) {
                if (ImGui::Button("Edit")) {
                    editDrug = drug;
                    std::snprintf(nameBuffer, sizeof(nameBuffer), "%s", drug.name.c_str());
                    std::snprintf(idBuffer, sizeof(idBuffer), "%s", drug.id.c_str());
                    std::snprintf(categoryBuffer, sizeof(categoryBuffer), "%s", drug.category.c_str());
                    editing = true;
                }
                ImGui::SameLine();
                ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.6f, 0.15f, 0.15f, 0.9f));
                if (ImGui::Button("Delete")) {
                    registry.removeDrug(drug.id);
                    registry.saveCustomDrugs(registry.getDefaultSavePath());
                    if (selectedDrugIdx >= static_cast<int>(drugs.size()))
                        selectedDrugIdx = static_cast<int>(drugs.size()) - 1;
                    std::snprintf(feedbackMsg, sizeof(feedbackMsg), "Drug deleted");
                    feedbackTimer = 3.0f;
                }
                ImGui::PopStyleColor();
            }
        } else {
            // === EDIT / CREATE MODE ===
            ImGui::TextColored(ImVec4(0.9f, 0.7f, 0.4f, 1.0f),
                               createNew ? "Create New Drug" : "Edit Drug");
            ImGui::Separator();
            ImGui::Spacing();

            ImGui::InputText("Name", nameBuffer, sizeof(nameBuffer));
            if (createNew) {
                ImGui::InputText("ID (unique)", idBuffer, sizeof(idBuffer));
            }
            ImGui::InputText("Category", categoryBuffer, sizeof(categoryBuffer));

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.75f, 0.95f, 1.0f), "Pharmacokinetics");
            ImGui::SliderFloat("Molecular Weight (g/mol)", &editDrug.molecularWeight, 10.0f, 2000.0f);
            ImGui::SliderFloat("Half-Life (hours)", &editDrug.halfLifeHours, 0.1f, 100.0f, "%.2f");
            ImGui::SliderFloat("Volume of Dist. (L/kg)", &editDrug.volumeOfDistribution, 0.01f, 50.0f, "%.2f");
            ImGui::SliderFloat("Bioavail. Oral", &editDrug.bioavailabilityOral, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Bioavail. IV", &editDrug.bioavailabilityIV, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Bioavail. IP", &editDrug.bioavailabilityIP, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Bioavail. SC", &editDrug.bioavailabilitySC, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Bioavail. Dermal", &editDrug.bioavailabilityDermal, 0.0f, 1.0f, "%.2f");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.95f, 0.5f, 0.4f, 1.0f), "Toxicity Profile (0 = none, 1 = primary target)");
            ImGui::SliderFloat("Hepatotoxicity (Liver)", &editDrug.hepatotoxicity, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Nephrotoxicity (Kidney)", &editDrug.nephrotoxicity, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Cardiotoxicity (Heart)", &editDrug.cardiotoxicity, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Neurotoxicity (Brain)", &editDrug.neurotoxicity, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Pulmonary Tox. (Lungs)", &editDrug.pulmonaryToxicity, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("GI Toxicity (Stomach)", &editDrug.giToxicity, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Hematotoxicity (Marrow)", &editDrug.hematotoxicity, 0.0f, 1.0f, "%.2f");
            ImGui::SliderFloat("Dermal Toxicity (Skin)", &editDrug.dermalToxicity, 0.0f, 1.0f, "%.2f");

            ImGui::Spacing();
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.6f, 1.0f), "Dose-Response");
            ImGui::SliderFloat("Therapeutic Dose (mg/kg)", &editDrug.therapeuticDose, 0.1f, 1000.0f, "%.1f");
            ImGui::SliderFloat("NOAEL (mg/kg)", &editDrug.noael, 0.1f, 5000.0f, "%.1f");
            ImGui::SliderFloat("LD50 (mg/kg)", &editDrug.ld50, 0.1f, 100000.0f, "%.1f");

            ImGui::Spacing();
            ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.2f, 0.55f, 0.3f, 0.9f));
            if (ImGui::Button("Save", ImVec2(100, 28))) {
                editDrug.name = nameBuffer;
                editDrug.category = categoryBuffer;
                editDrug.isBuiltIn = false;

                if (createNew) {
                    editDrug.id = idBuffer;
                    if (editDrug.id.empty()) editDrug.id = "custom_" + editDrug.name;
                    registry.addDrug(editDrug);
                    selectedDrugIdx = static_cast<int>(drugs.size()) - 1;
                } else {
                    registry.updateDrug(editDrug);
                }
                registry.saveCustomDrugs(registry.getDefaultSavePath());
                editing = false;
                createNew = false;
                std::snprintf(feedbackMsg, sizeof(feedbackMsg), "Drug saved successfully");
                feedbackTimer = 3.0f;
            }
            ImGui::PopStyleColor();

            ImGui::SameLine();
            if (ImGui::Button("Cancel", ImVec2(100, 28))) {
                editing = false;
                createNew = false;
            }
        }
    }

    ImGui::EndChild();

    // Bottom bar: Create New + Back
    {
        ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0.35f, 0.18f, 0.48f, 0.90f));
        if (ImGui::Button("+ New Drug", ImVec2(120, 28))) {
            createNew = true;
            editing = false;
            editDrug = DrugCompound{};
            editDrug.isBuiltIn = false;
            nameBuffer[0] = '\0';
            idBuffer[0] = '\0';
            std::snprintf(categoryBuffer, sizeof(categoryBuffer), "custom");
        }
        ImGui::PopStyleColor();

        ImGui::SameLine();
        if (ImGui::Button("Back to Dashboard", ImVec2(150, 28))) {
            app.goToDashboard();
            editing = false;
            createNew = false;
        }

        // Feedback message
        if (feedbackTimer > 0.0f) {
            feedbackTimer -= ImGui::GetIO().DeltaTime;
            ImGui::SameLine();
            float alpha = std::min(1.0f, feedbackTimer);
            ImGui::TextColored(ImVec4(0.4f, 0.9f, 0.5f, alpha), "%s", feedbackMsg);
        }
    }

    ImGui::End();
    popHudStyle();
}

// ─── Status Bar ─────────────────────────────────────────────────────────────

void GuiManager::renderStatusBar(Application& app) {
    ImGuiViewport* vp = ImGui::GetMainViewport();
    float barHeight = 26.0f;

    ImGui::SetNextWindowPos(ImVec2(vp->WorkPos.x,
                                   vp->WorkPos.y + vp->WorkSize.y - barHeight));
    ImGui::SetNextWindowSize(ImVec2(vp->WorkSize.x, barHeight));

    ImGuiWindowFlags flags =
        ImGuiWindowFlags_NoDecoration |
        ImGuiWindowFlags_NoMove |
        ImGuiWindowFlags_NoScrollWithMouse |
        ImGuiWindowFlags_NoDocking |
        ImGuiWindowFlags_NoSavedSettings |
        ImGuiWindowFlags_NoFocusOnAppearing;

    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
    ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0.04f, 0.04f, 0.06f, 0.85f));
    ImGui::PushStyleColor(ImGuiCol_Border, ImVec4(0.20f, 0.25f, 0.35f, 0.50f));

    ImGui::Begin("##StatusBar", nullptr, flags);

    const char* stateNames[] = {"Dashboard", "Setup", "Simulation", "Interactive", "Results"};
    ImGui::TextColored(ImVec4(0.4f, 0.75f, 0.95f, 1.0f), "%s",
                       stateNames[static_cast<int>(app.getState())]);

    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.35f, 0.38f, 0.48f, 1.0f), "|");
    ImGui::SameLine();
    ImGui::TextColored(ImVec4(0.40f, 0.42f, 0.52f, 0.8f),
                       "Right-drag: Orbit  Scroll: Zoom  Middle-drag: Pan");

    ImGui::SameLine(ImGui::GetWindowWidth() - 210);
    ImGui::TextColored(ImVec4(0.40f, 0.42f, 0.50f, 1.0f),
                       "FPS: %.0f | www.sjhwang.com", app.getTimer().getFPS());

    ImGui::End();
    ImGui::PopStyleColor(2);
    ImGui::PopStyleVar(2);
}

} // namespace animsim
