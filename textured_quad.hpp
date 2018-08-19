#pragma once

#include <mat4x4.hpp>

#include <GLES3/gl3.h>
#include "camera.hpp"

class TexturedQuad {
public:
    TexturedQuad(const unsigned char* data, int w, int h);
    ~TexturedQuad();

    void set_model(glm::mat4 model) { m_model = std::move(model); }
    const glm::mat4& model() const { return m_model; }

    void render(const Camera& cam, const glm::mat4& other);

    void doStuff(const Camera& cam, const glm::vec2 cursor);

private:
    int m_width;
    int m_height;
    float m_ratio;
    GLuint m_draw_texture;
    GLuint m_paint_texture;
    GLuint m_vertices;
    GLuint m_uvs;

    glm::mat4 m_model = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
};
