#pragma once

#include <glad/gl.h>
#include <glm/glm.hpp>
#include <string>

namespace animsim {

class Shader {
public:
    Shader() = default;
    ~Shader();

    Shader(const Shader&) = delete;
    Shader& operator=(const Shader&) = delete;
    Shader(Shader&& other) noexcept;
    Shader& operator=(Shader&& other) noexcept;

    bool loadFromFile(const std::string& vertPath, const std::string& fragPath);
    bool loadFromSource(const std::string& vertSrc, const std::string& fragSrc);

    void bind() const;
    void unbind() const;

    GLuint getProgram() const { return m_program; }

    // Uniform setters
    void setInt(const std::string& name, int value) const;
    void setFloat(const std::string& name, float value) const;
    void setVec3(const std::string& name, const glm::vec3& value) const;
    void setVec4(const std::string& name, const glm::vec4& value) const;
    void setMat3(const std::string& name, const glm::mat3& value) const;
    void setMat4(const std::string& name, const glm::mat4& value) const;

private:
    GLuint compileShader(GLenum type, const std::string& source);
    GLint getUniformLocation(const std::string& name) const;
    void destroy();

    GLuint m_program = 0;
};

} // namespace animsim
