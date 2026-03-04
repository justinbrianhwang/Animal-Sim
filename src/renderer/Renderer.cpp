#include "renderer/Renderer.h"
#include <glm/gtc/matrix_transform.hpp>

namespace animsim {

// Embedded shaders (no file I/O needed)
static const char* PHONG_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;
layout (location = 1) in vec3 aNormal;
layout (location = 2) in vec2 aTexCoord;
layout (location = 3) in vec3 aColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;
uniform mat3 uNormalMatrix;

out vec3 vFragPos;
out vec3 vNormal;
out vec2 vTexCoord;
out vec3 vColor;

void main() {
    vec4 worldPos = uModel * vec4(aPos, 1.0);
    vFragPos = worldPos.xyz;
    vNormal = uNormalMatrix * aNormal;
    vTexCoord = aTexCoord;
    vColor = aColor;
    gl_Position = uProjection * uView * worldPos;
}
)";

static const char* PHONG_FRAG = R"(
#version 330 core
in vec3 vFragPos;
in vec3 vNormal;
in vec2 vTexCoord;
in vec3 vColor;

uniform vec3 uLightDir;
uniform vec3 uLightColor;
uniform vec3 uAmbientColor;
uniform vec3 uViewPos;
uniform vec3 uObjectColor;
uniform float uSpecularStrength;

out vec4 FragColor;

void main() {
    vec3 color = uObjectColor * vColor;
    vec3 normal = normalize(vNormal);
    vec3 lightDir = normalize(-uLightDir);

    // Ambient
    vec3 ambient = uAmbientColor * color;

    // Diffuse
    float diff = max(dot(normal, lightDir), 0.0);
    vec3 diffuse = diff * uLightColor * color;

    // Specular (Blinn-Phong)
    vec3 viewDir = normalize(uViewPos - vFragPos);
    vec3 halfDir = normalize(lightDir + viewDir);
    float spec = pow(max(dot(normal, halfDir), 0.0), 32.0);
    vec3 specular = uSpecularStrength * spec * uLightColor;

    // Fog (distance-based, subtle)
    float dist = length(uViewPos - vFragPos);
    float fog = clamp((dist - 15.0) / 30.0, 0.0, 0.4);
    vec3 fogColor = vec3(0.08, 0.08, 0.12);

    vec3 result = ambient + diffuse + specular;
    result = mix(result, fogColor, fog);

    FragColor = vec4(result, 1.0);
}
)";

static const char* GRID_VERT = R"(
#version 330 core
layout (location = 0) in vec3 aPos;

uniform mat4 uView;
uniform mat4 uProjection;

out vec3 vWorldPos;

void main() {
    vWorldPos = aPos;
    gl_Position = uProjection * uView * vec4(aPos, 1.0);
}
)";

static const char* GRID_FRAG = R"(
#version 330 core
in vec3 vWorldPos;

out vec4 FragColor;

void main() {
    // Grid lines
    vec2 coord = vWorldPos.xz;
    vec2 grid = abs(fract(coord - 0.5) - 0.5) / fwidth(coord);
    float line = min(grid.x, grid.y);
    float alpha = 1.0 - min(line, 1.0);
    alpha *= 0.15;

    // Fade with distance from origin
    float dist = length(coord);
    alpha *= max(0.0, 1.0 - dist / 12.0);

    FragColor = vec4(0.4, 0.45, 0.55, alpha);
}
)";

Renderer::Renderer() {}

void Renderer::init() {
    if (m_initialized) return;

    m_phongShader.loadFromSource(PHONG_VERT, PHONG_FRAG);
    m_gridShader.loadFromSource(GRID_VERT, GRID_FRAG);
    m_gridMesh = Mesh::createPlane(30.0f, 30.0f, glm::vec3(0.3f));
    m_framebuffer.create(800, 600);
    m_initialized = true;
}

void Renderer::renderInternal(const std::vector<RenderObject>& objects,
                               Camera& camera, float aspectRatio) {
    glClearColor(0.06f, 0.06f, 0.09f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    glm::mat4 view = camera.getViewMatrix();
    glm::mat4 proj = camera.getProjectionMatrix(aspectRatio);
    glm::vec3 viewPos = camera.getPosition();

    // Render grid floor
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    m_gridShader.bind();
    m_gridShader.setMat4("uView", view);
    m_gridShader.setMat4("uProjection", proj);
    m_gridMesh.draw();
    glDisable(GL_BLEND);

    // Render objects with Phong lighting
    m_phongShader.bind();
    m_phongShader.setMat4("uView", view);
    m_phongShader.setMat4("uProjection", proj);
    m_phongShader.setVec3("uLightDir", lightDirection);
    m_phongShader.setVec3("uLightColor", lightColor);
    m_phongShader.setVec3("uAmbientColor", ambientColor);
    m_phongShader.setVec3("uViewPos", viewPos);

    for (const auto& obj : objects) {
        if (!obj.mesh) continue;
        m_phongShader.setMat4("uModel", obj.transform);
        glm::mat3 normalMat = glm::transpose(glm::inverse(glm::mat3(obj.transform)));
        m_phongShader.setMat3("uNormalMatrix", normalMat);
        m_phongShader.setVec3("uObjectColor", obj.color);
        m_phongShader.setFloat("uSpecularStrength", obj.specularStrength);
        obj.mesh->draw();
    }
}

void Renderer::renderScene(const std::vector<RenderObject>& objects,
                            Camera& camera, float aspectRatio) {
    if (!m_initialized) init();
    m_framebuffer.bind();
    renderInternal(objects, camera, aspectRatio);
    m_framebuffer.unbind();
}

void Renderer::renderToScreen(const std::vector<RenderObject>& objects,
                               Camera& camera, int windowWidth, int windowHeight) {
    if (!m_initialized) init();
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glViewport(0, 0, windowWidth, windowHeight);
    float aspect = (windowHeight > 0) ? static_cast<float>(windowWidth) / windowHeight : 1.0f;
    renderInternal(objects, camera, aspect);
}

} // namespace animsim
