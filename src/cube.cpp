#include "cube.hpp"
#include "utils.hpp"
#include "engine.hpp"
#include "log.hpp"
#include "shader_functions.hpp"

namespace {
namespace Buffers {
    bool ok = false;
    GLuint verticesBuffer;
    int verticesCount;

    void init() {
        if (ok)
            return;
        const auto verts = make_array<float>(
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
             1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
            -1.0f, -1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
            -1.0f, -1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f, -1.0f,
             1.0f, -1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
             1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f, -1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f, -1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f,  1.0f,  1.0f,
            -1.0f,  1.0f,  1.0f,
             1.0f, -1.0f,  1.0f
        );
        verticesCount = verts.size() / 3;

        glGenBuffers(1, &verticesBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, verticesBuffer);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), &verts[0], GL_STATIC_DRAW);

        ok = true;
    }
}}

Cube::Cube() {
    Buffers::init();
}

namespace { namespace RgbShader {
    GLuint program = 0;

    const char* vert = R"VERT(#version 300 es
precision mediump float;
layout (location = 0) in vec3 Position;
uniform mat4 mvp;
uniform vec3 ratio;
out vec4 color;
void main()
{
    color = 0.5 * (vec4(Position.xyz, 1) + vec4(1));
    gl_Position = mvp * vec4(ratio * Position.xyz, 1);
})VERT";

    const char* frag = R"FRAG(#version 300 es
precision mediump float;
layout (location = 0) out vec4 Out_Color;
in vec4 color;
void main()
{
    Out_Color = color;
})FRAG";

    GLuint MvpID;
    GLuint RatioID;

    void init() {
        if (program == 0) {
            if (!create_program(program, vert, frag) or program == 0) {
                Log::Error("Error creating program");
                return;
            }
            MvpID = glGetUniformLocation(program, "mvp");
            RatioID = glGetUniformLocation(program, "ratio");
        }
    }
}}


void Cube::render(const Camera& cam, const glm::vec3& ratio) const
{
    using namespace RgbShader;
    init();

    glUseProgram(program);
    glEnableVertexAttribArray(0);
    auto& mvp = cam.projection_view();
    glUniformMatrix4fv(MvpID, 1, GL_FALSE, &mvp[0][0]);
    glUniform3fv(RatioID, 1, &ratio[0]);
    glBindBuffer(GL_ARRAY_BUFFER, Buffers::verticesBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, Buffers::verticesCount);
}

void Cube::render(const glm::mat4& mvp) const
{
    using namespace RgbShader;
    init();

    glUseProgram(program);
    glEnableVertexAttribArray(0);
    glUniformMatrix4fv(MvpID, 1, GL_FALSE, &mvp[0][0]);
    glm::vec3 ratio(1.f);
    glUniform3fv(RatioID, 1, &ratio[0]);
    glBindBuffer(GL_ARRAY_BUFFER, Buffers::verticesBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, Buffers::verticesCount);
}

void Cube::renderToTexture(const Camera& cam, const glm::vec3& ratio, bool front, TexturedQuad& quad) const
{
    GLuint fb;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, quad.texture(), 0);

    glViewport(0, 0, quad.width(), quad.height());
    glEnable(GL_CULL_FACE);
    if (front)
        glCullFace(GL_BACK);
    else
        glCullFace(GL_FRONT);
    render(cam, ratio);
    glDisable(GL_CULL_FACE);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    glDeleteFramebuffers(1, &fb);
}

