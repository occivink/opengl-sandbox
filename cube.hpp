#pragma once

#include <mat4x4.hpp>
#include "textured_quad.hpp"

#include "camera.hpp"

class Cube {
public:
    Cube();

    void render(const glm::mat4& mvp, const glm::vec3& ratio) const;

    void renderToTexture(const glm::mat4& mvp,
                         const glm::vec3& ratio,
                         bool front,
                         TexturedQuad& quad) const;
};
