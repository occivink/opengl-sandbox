#pragma once

#include <mat4x4.hpp>
#include <optional>

#include <GLES3/gl3.h>
#include "camera.hpp"

class TexturedQuad {
public:
    TexturedQuad(const unsigned char* data, int w, int h, int c, bool nearest = false);
    TexturedQuad(int w, int h, int c, bool nearest = false);
    ~TexturedQuad();

    void render(const Camera& cam,
                const glm::mat4* model = nullptr);

    bool unproject(const Camera& cam,
                   const glm::mat4* model,
                   const glm::vec2& cursor, // in [-1, 1] screen-coordinates (0,0 is top-left)
                   glm::vec2& outPicked);   // in [-1, 1] texture-coordinates

    void paint(glm::vec2 from, // in [-1, 1] texture coordinates
               glm::vec2 to,   // same
               const glm::vec3& color,
               float radius,
               float blob_ratio);

    void renderWithLabels(const Camera& cam,
                          const TexturedQuad& labels,
                          float label_opacity,
                          const glm::mat4* model = nullptr);

    int width() const { return m_width; }
    int height() const { return m_height; }
    int channels() const { return m_channels; }
    float ratio() const { return m_ratio; }

private:
    int m_width;
    int m_height;
    int m_channels;
    float m_ratio;
    GLuint m_texture;
};
