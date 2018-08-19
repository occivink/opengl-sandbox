#pragma once

#include <mat4x4.hpp>

#include <GLES3/gl3.h>
#include "camera.hpp"

class Cube {
public:
    Cube();

    void set_model(glm::mat4 model) { m_model = std::move(model); }
    const glm::mat4& model() const { return m_model; }

    void render(const Camera& cam, const glm::mat4& other);

private:
    GLuint m_vertices;

    glm::mat4 m_model = {
        1.f, 0.f, 0.f, 0.f,
        0.f, 1.f, 0.f, 0.f,
        0.f, 0.f, 1.f, 0.f,
        0.f, 0.f, 0.f, 1.f
    };
};
