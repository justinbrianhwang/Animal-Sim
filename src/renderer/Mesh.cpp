#include "renderer/Mesh.h"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace animsim {

Mesh::~Mesh() { destroy(); }

Mesh::Mesh(Mesh&& other) noexcept
    : m_vao(other.m_vao), m_vbo(other.m_vbo), m_ebo(other.m_ebo), m_indexCount(other.m_indexCount) {
    other.m_vao = other.m_vbo = other.m_ebo = 0;
    other.m_indexCount = 0;
}

Mesh& Mesh::operator=(Mesh&& other) noexcept {
    if (this != &other) {
        destroy();
        m_vao = other.m_vao; m_vbo = other.m_vbo; m_ebo = other.m_ebo;
        m_indexCount = other.m_indexCount;
        other.m_vao = other.m_vbo = other.m_ebo = 0;
        other.m_indexCount = 0;
    }
    return *this;
}

void Mesh::upload(const std::vector<Vertex>& vertices, const std::vector<uint32_t>& indices) {
    destroy();
    m_indexCount = indices.size();

    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glGenBuffers(1, &m_ebo);

    glBindVertexArray(m_vao);

    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(vertices.size() * sizeof(Vertex)),
                 vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_ebo);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER,
                 static_cast<GLsizeiptr>(indices.size() * sizeof(uint32_t)),
                 indices.data(), GL_STATIC_DRAW);

    // position
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, position));
    // normal
    glEnableVertexAttribArray(1);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, normal));
    // texCoord
    glEnableVertexAttribArray(2);
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, texCoord));
    // color
    glEnableVertexAttribArray(3);
    glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (void*)offsetof(Vertex, color));

    glBindVertexArray(0);
}

void Mesh::draw() const {
    if (m_vao == 0 || m_indexCount == 0) return;
    glBindVertexArray(m_vao);
    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(m_indexCount), GL_UNSIGNED_INT, nullptr);
    glBindVertexArray(0);
}

void Mesh::destroy() {
    if (m_vao) { glDeleteVertexArrays(1, &m_vao); m_vao = 0; }
    if (m_vbo) { glDeleteBuffers(1, &m_vbo); m_vbo = 0; }
    if (m_ebo) { glDeleteBuffers(1, &m_ebo); m_ebo = 0; }
    m_indexCount = 0;
}

// ─── Procedural generators ─────────────────────────────────────────────────

Mesh Mesh::createCube(const glm::vec3& size, const glm::vec3& color) {
    float hx = size.x * 0.5f, hy = size.y * 0.5f, hz = size.z * 0.5f;
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;

    auto face = [&](glm::vec3 n, glm::vec3 a, glm::vec3 b, glm::vec3 c, glm::vec3 d) {
        uint32_t base = static_cast<uint32_t>(verts.size());
        verts.push_back({a, n, {0,0}, color});
        verts.push_back({b, n, {1,0}, color});
        verts.push_back({c, n, {1,1}, color});
        verts.push_back({d, n, {0,1}, color});
        idx.insert(idx.end(), {base, base+1, base+2, base, base+2, base+3});
    };

    // +Y (top)
    face({0,1,0}, {-hx,hy,-hz}, {hx,hy,-hz}, {hx,hy,hz}, {-hx,hy,hz});
    // -Y (bottom)
    face({0,-1,0}, {-hx,-hy,hz}, {hx,-hy,hz}, {hx,-hy,-hz}, {-hx,-hy,-hz});
    // +Z (front)
    face({0,0,1}, {-hx,-hy,hz}, {hx,-hy,hz}, {hx,hy,hz}, {-hx,hy,hz});
    // -Z (back)
    face({0,0,-1}, {hx,-hy,-hz}, {-hx,-hy,-hz}, {-hx,hy,-hz}, {hx,hy,-hz});
    // +X (right)
    face({1,0,0}, {hx,-hy,hz}, {hx,-hy,-hz}, {hx,hy,-hz}, {hx,hy,hz});
    // -X (left)
    face({-1,0,0}, {-hx,-hy,-hz}, {-hx,-hy,hz}, {-hx,hy,hz}, {-hx,hy,-hz});

    Mesh mesh;
    mesh.upload(verts, idx);
    return mesh;
}

Mesh Mesh::createPlane(float width, float depth, const glm::vec3& color) {
    float hw = width * 0.5f, hd = depth * 0.5f;
    std::vector<Vertex> verts = {
        {{-hw, 0, -hd}, {0,1,0}, {0,0}, color},
        {{ hw, 0, -hd}, {0,1,0}, {1,0}, color},
        {{ hw, 0,  hd}, {0,1,0}, {1,1}, color},
        {{-hw, 0,  hd}, {0,1,0}, {0,1}, color},
    };
    std::vector<uint32_t> idx = {0,1,2, 0,2,3};
    Mesh mesh;
    mesh.upload(verts, idx);
    return mesh;
}

Mesh Mesh::createSphere(float radius, int segments, const glm::vec3& color) {
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;

    for (int y = 0; y <= segments; y++) {
        for (int x = 0; x <= segments; x++) {
            float xSeg = static_cast<float>(x) / segments;
            float ySeg = static_cast<float>(y) / segments;
            float xPos = std::cos(xSeg * 2.0f * static_cast<float>(M_PI)) * std::sin(ySeg * static_cast<float>(M_PI));
            float yPos = std::cos(ySeg * static_cast<float>(M_PI));
            float zPos = std::sin(xSeg * 2.0f * static_cast<float>(M_PI)) * std::sin(ySeg * static_cast<float>(M_PI));

            glm::vec3 pos(xPos * radius, yPos * radius, zPos * radius);
            glm::vec3 norm(xPos, yPos, zPos);
            verts.push_back({pos, norm, {xSeg, ySeg}, color});
        }
    }

    for (int y = 0; y < segments; y++) {
        for (int x = 0; x < segments; x++) {
            uint32_t a = y * (segments + 1) + x;
            uint32_t b = a + segments + 1;
            idx.insert(idx.end(), {a, b, a+1, b, b+1, a+1});
        }
    }

    Mesh mesh;
    mesh.upload(verts, idx);
    return mesh;
}

Mesh Mesh::createCylinder(float radius, float height, int segments, const glm::vec3& color) {
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;
    float halfH = height * 0.5f;

    // Side
    for (int i = 0; i <= segments; i++) {
        float angle = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
        float x = std::cos(angle);
        float z = std::sin(angle);
        glm::vec3 n(x, 0, z);
        float u = static_cast<float>(i) / segments;

        verts.push_back({{x * radius, -halfH, z * radius}, n, {u, 0}, color});
        verts.push_back({{x * radius,  halfH, z * radius}, n, {u, 1}, color});
    }
    for (int i = 0; i < segments; i++) {
        uint32_t a = i * 2, b = a + 1, c = a + 2, d = a + 3;
        idx.insert(idx.end(), {a, c, b, b, c, d});
    }

    // Top cap
    uint32_t topCenter = static_cast<uint32_t>(verts.size());
    verts.push_back({{0, halfH, 0}, {0,1,0}, {0.5f, 0.5f}, color});
    for (int i = 0; i <= segments; i++) {
        float angle = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
        float x = std::cos(angle), z = std::sin(angle);
        verts.push_back({{x * radius, halfH, z * radius}, {0,1,0}, {x*0.5f+0.5f, z*0.5f+0.5f}, color});
    }
    for (int i = 0; i < segments; i++) {
        idx.insert(idx.end(), {topCenter, topCenter + 1 + static_cast<uint32_t>(i),
                               topCenter + 2 + static_cast<uint32_t>(i)});
    }

    // Bottom cap
    uint32_t botCenter = static_cast<uint32_t>(verts.size());
    verts.push_back({{0, -halfH, 0}, {0,-1,0}, {0.5f, 0.5f}, color});
    for (int i = 0; i <= segments; i++) {
        float angle = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
        float x = std::cos(angle), z = std::sin(angle);
        verts.push_back({{x * radius, -halfH, z * radius}, {0,-1,0}, {x*0.5f+0.5f, z*0.5f+0.5f}, color});
    }
    for (int i = 0; i < segments; i++) {
        idx.insert(idx.end(), {botCenter, botCenter + 2 + static_cast<uint32_t>(i),
                               botCenter + 1 + static_cast<uint32_t>(i)});
    }

    Mesh mesh;
    mesh.upload(verts, idx);
    return mesh;
}

Mesh Mesh::createCapsule(float radius, float length, int segments, const glm::vec3& color) {
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;
    float halfLen = length * 0.5f;

    // Hemisphere caps + cylinder body
    for (int y = 0; y <= segments; y++) {
        for (int x = 0; x <= segments; x++) {
            float xSeg = static_cast<float>(x) / segments;
            float ySeg = static_cast<float>(y) / segments;
            float theta = xSeg * 2.0f * static_cast<float>(M_PI);
            float phi = ySeg * static_cast<float>(M_PI);

            float nx = std::cos(theta) * std::sin(phi);
            float ny = std::cos(phi);
            float nz = std::sin(theta) * std::sin(phi);

            float px = nx * radius;
            float py = ny * radius;
            float pz = nz * radius;

            // Shift hemisphere caps apart
            if (ny > 0.0f) py += halfLen;
            else py -= halfLen;

            verts.push_back({{px, py, pz}, {nx, ny, nz}, {xSeg, ySeg}, color});
        }
    }

    for (int y = 0; y < segments; y++) {
        for (int x = 0; x < segments; x++) {
            uint32_t a = y * (segments + 1) + x;
            uint32_t b = a + segments + 1;
            idx.insert(idx.end(), {a, b, a+1, b, b+1, a+1});
        }
    }

    Mesh mesh;
    mesh.upload(verts, idx);
    return mesh;
}

Mesh Mesh::createTaperedCylinder(float rBot, float rTop, float height, int segments, const glm::vec3& color) {
    std::vector<Vertex> verts;
    std::vector<uint32_t> idx;
    float halfH = height * 0.5f;

    // Side vertices
    for (int i = 0; i <= segments; i++) {
        float angle = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
        float cs = std::cos(angle), sn = std::sin(angle);
        float u = static_cast<float>(i) / segments;

        // Normal accounts for taper
        float dr = rBot - rTop;
        float slopeLen = std::sqrt(dr * dr + height * height);
        float ny = dr / slopeLen;
        float nr = height / slopeLen;

        glm::vec3 n(cs * nr, ny, sn * nr);
        verts.push_back({{cs * rBot, -halfH, sn * rBot}, n, {u, 0}, color});
        verts.push_back({{cs * rTop,  halfH, sn * rTop}, n, {u, 1}, color});
    }
    for (int i = 0; i < segments; i++) {
        uint32_t a = i * 2, b = a + 1, c = a + 2, d = a + 3;
        idx.insert(idx.end(), {a, c, b, b, c, d});
    }

    // Top cap
    uint32_t topC = static_cast<uint32_t>(verts.size());
    verts.push_back({{0, halfH, 0}, {0,1,0}, {0.5f, 0.5f}, color});
    for (int i = 0; i <= segments; i++) {
        float angle = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
        float cs = std::cos(angle), sn = std::sin(angle);
        verts.push_back({{cs * rTop, halfH, sn * rTop}, {0,1,0}, {cs*0.5f+0.5f, sn*0.5f+0.5f}, color});
    }
    for (int i = 0; i < segments; i++)
        idx.insert(idx.end(), {topC, topC+1+(uint32_t)i, topC+2+(uint32_t)i});

    // Bottom cap
    uint32_t botC = static_cast<uint32_t>(verts.size());
    verts.push_back({{0, -halfH, 0}, {0,-1,0}, {0.5f, 0.5f}, color});
    for (int i = 0; i <= segments; i++) {
        float angle = static_cast<float>(i) / segments * 2.0f * static_cast<float>(M_PI);
        float cs = std::cos(angle), sn = std::sin(angle);
        verts.push_back({{cs * rBot, -halfH, sn * rBot}, {0,-1,0}, {cs*0.5f+0.5f, sn*0.5f+0.5f}, color});
    }
    for (int i = 0; i < segments; i++)
        idx.insert(idx.end(), {botC, botC+2+(uint32_t)i, botC+1+(uint32_t)i});

    Mesh mesh;
    mesh.upload(verts, idx);
    return mesh;
}

} // namespace animsim
