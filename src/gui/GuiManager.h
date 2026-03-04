#pragma once

namespace animsim {

class Application;

class GuiManager {
public:
    static void applyTheme();

    static void beginDockspace();
    static void endDockspace();

    static void renderDashboard(Application& app);
    static void renderSetup(Application& app);
    static void renderSimulation(Application& app);
    static void renderInteractive(Application& app);
    static void renderResults(Application& app);
    static void renderDrugEditor(Application& app);
    static void renderStatusBar(Application& app);

private:
    // Dashboard helpers
    static bool experimentCard(const char* title, const char* description,
                               const char* icon, float width);

    // 3D Viewport rendering helper
    static void renderViewport(Application& app, const char* label);
};

} // namespace animsim
