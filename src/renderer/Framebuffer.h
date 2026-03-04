#pragma once

#include <glad/gl.h>

namespace animsim {

class Framebuffer {
public:
    Framebuffer() = default;
    ~Framebuffer();

    Framebuffer(const Framebuffer&) = delete;
    Framebuffer& operator=(const Framebuffer&) = delete;

    void create(int width, int height);
    void resize(int width, int height);
    void bind() const;
    void unbind() const;

    GLuint getColorTexture() const { return m_colorTexture; }
    int getWidth() const { return m_width; }
    int getHeight() const { return m_height; }

private:
    void destroy();

    GLuint m_fbo = 0;
    GLuint m_colorTexture = 0;
    GLuint m_depthRbo = 0;
    int m_width = 0;
    int m_height = 0;
};

} // namespace animsim
