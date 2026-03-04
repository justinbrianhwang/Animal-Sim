#pragma once

#include "renderer/Shader.h"
#include "renderer/Camera.h"
#include "renderer/Mesh.h"
#include "renderer/Framebuffer.h"
#include <glm/glm.hpp>
#include <vector>
#include <memory>

namespace animsim {

struct RenderObject {
    Mesh* mesh = nullptr;
    glm::mat4 transform{1.0f};
    glm::vec3 color{0.8f};
    float specularStrength = 0.5f;
};

class Renderer {
public:
    Renderer();
    ~Renderer() = default;

    void init();
    void renderScene(const std::vector<RenderObject>& objects, Camera& camera, float aspectRatio);
    void renderToScreen(const std::vector<RenderObject>& objects, Camera& camera, int windowWidth, int windowHeight);
    Framebuffer& getFramebuffer() { return m_framebuffer; }

    // Lighting
    glm::vec3 lightDirection{-0.4f, -0.8f, -0.6f};
    glm::vec3 lightColor{1.0f, 0.98f, 0.95f};
    glm::vec3 ambientColor{0.15f, 0.15f, 0.2f};

private:
    void renderInternal(const std::vector<RenderObject>& objects, Camera& camera, float aspectRatio);

    Shader m_phongShader;
    Shader m_gridShader;
    Framebuffer m_framebuffer;
    Mesh m_gridMesh;
    bool m_initialized = false;
};

} // namespace animsim
