#pragma once

#include <string>
#include <functional>

struct GLFWwindow;

namespace animsim {

class Window {
public:
    Window(int width, int height, const std::string& title);
    ~Window();

    Window(const Window&) = delete;
    Window& operator=(const Window&) = delete;

    bool shouldClose() const;
    void pollEvents() const;
    void swapBuffers() const;

    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }
    float getAspectRatio() const;
    GLFWwindow* getHandle() const { return m_window; }

    // Input state
    bool isKeyPressed(int key) const;
    void getCursorPos(double& x, double& y) const;
    bool isMouseButtonPressed(int button) const;

    // Callbacks
    using ResizeCallback = std::function<void(int, int)>;
    void setResizeCallback(ResizeCallback cb) { m_resizeCb = cb; }

private:
    static void framebufferSizeCallback(GLFWwindow* window, int width, int height);

    GLFWwindow* m_window = nullptr;
    int m_width;
    int m_height;
    ResizeCallback m_resizeCb;
};

} // namespace animsim
