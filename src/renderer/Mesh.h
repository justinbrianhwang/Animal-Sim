#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <vector>

namespace animsim {

struct Vertex {
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoord;
    glm::vec3 color{1.0f}; // default white
};

class Mesh {
public:
    Mesh() = default;
    ~Mesh();

    Mesh(const Mesh&) = delete;
    Mesh& operator=(const Mesh&) = delete;
    Mesh(Mesh&& other) noexcept;
    Mesh& operator=(Mesh&& other) noexcept;

    void upload(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices);
    void draw() const;

    size_t getIndexCount() const { return m_indexCount; }

    // Procedural mesh generators
    static Mesh createCube(const glm::vec3& size = glm::vec3(1.0f), const glm::vec3& color = glm::vec3(1.0f));
    static Mesh createPlane(float width, float depth, const glm::vec3& color = glm::vec3(1.0f));
    static Mesh createSphere(float radius, int segments = 32, const glm::vec3& color = glm::vec3(1.0f));
    static Mesh createCylinder(float radius, float height, int segments = 32, const glm::vec3& color = glm::vec3(1.0f));
    static Mesh createCapsule(float radius, float length, int segments = 16, const glm::vec3& color = glm::vec3(1.0f));
    static Mesh createTaperedCylinder(float rBot, float rTop, float height, int segments = 12, const glm::vec3& color = glm::vec3(1.0f));

private:
    void destroy();

    GLuint m_vao = 0;
    GLuint m_vbo = 0;
    GLuint m_ebo = 0;
    size_t m_indexCount = 0;
};

} // namespace animsim
