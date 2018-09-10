#pragma once

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <optional>
#include <vector>

#include <GLES3/gl3.h>
#include "camera.hpp"

class Volume {
public:
    Volume(const unsigned char* data, glm::ivec3 size, int c);
    Volume(glm::ivec3 size, int c);
    ~Volume();

    void render(const Camera& cam) const;

    const glm::ivec3& size() const { return m_size; }
    int channels() const { return m_channels; }
    const glm::vec3& ratio() const { return m_ratio; }
    GLuint texture() const { return m_texture; }

private:
    glm::ivec3 m_size; // (height, width, depth)-tuple
    int m_channels;
    glm::vec3 m_ratio; // ratio along the x, y and z axes
    GLuint m_texture;
};
