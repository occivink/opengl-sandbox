#include "textured_quad.hpp"
#include "log.hpp"
#include "engine.hpp"
#include "shader_functions.hpp"

#define GLM_ENABLE_EXPERIMENTAL
#include <gtx/string_cast.hpp>
#include <gtx/intersect.hpp>

#include <vector>
#include <cstdio>

TexturedQuad::TexturedQuad(const unsigned char* data, int w, int h)
    : m_width(w)
    , m_height(h)
    , m_ratio((float)w / (float)h)
{
    {
        glGenTextures(1, &m_draw_texture);

        // "Bind" the newly created texture : all future texture functions will modify this texture
        glBindTexture(GL_TEXTURE_2D, m_draw_texture);

        // Give the image to OpenGL
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, w, h, 0,  GL_RGBA, GL_UNSIGNED_BYTE, data);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }

	/*{
        glGenTextures(1, &m_paint_texture);

        glBindTexture(GL_TEXTURE_2D, m_paint_texture);

        glTexImage2D(GL_TEXTURE_2D, 0, GL_RED, w, h, 0,  GL_RED, GL_UNSIGNED_BYTE, 0);

        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	}*/

    static const std::vector<glm::vec3> verts = {
        glm::vec3(-1.f, -1.f, 0.f),
        glm::vec3(-1.f,  1.f, 0.f),
        glm::vec3( 1.f,  1.f, 0.f),
        glm::vec3(-1.f, -1.f, 0.f),
        glm::vec3( 1.f, -1.f, 0.f),
        glm::vec3( 1.f,  1.f, 0.f),
    };
    glGenBuffers(1, &m_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * 3 * sizeof(float), &verts[0][0], GL_STATIC_DRAW);

    static const std::vector<glm::vec2> UVs = {
        glm::vec2(0.0f, 1.0f),
        glm::vec2(0.0f, 0.0f),
        glm::vec2(1.0f, 0.0f),
        glm::vec2(0.0f, 1.0f),
        glm::vec2(1.0f, 1.0f),
        glm::vec2(1.0f, 0.0f),
    };
    glGenBuffers(1, &m_uvs);
    glBindBuffer(GL_ARRAY_BUFFER, m_uvs);
    glBufferData(GL_ARRAY_BUFFER, UVs.size() * 2 * sizeof(float), &UVs[0][0], GL_STATIC_DRAW);
}

TexturedQuad::~TexturedQuad() {
	glDeleteBuffers(1, &m_vertices);
	glDeleteBuffers(1, &m_uvs);
    glDeleteTextures(1, &m_draw_texture);
}

namespace {
    namespace DrawShader {
        GLuint program = 0;

        const char* vert = R"VERT(#version 300 es
precision mediump float;
layout (location = 0) in vec3 Position;
layout(location = 1) in vec2 vertexUV;
uniform float ratio;
uniform mat4 mvp;
out vec2 uv;
void main()
{
    uv = vertexUV;
    gl_Position = mvp * vec4(ratio * Position.x, Position.y, Position.z, 1);
})VERT";

        const char* frag = R"FRAG(#version 300 es
precision mediump float;
layout (location = 0) out vec4 Out_Color;
in vec2 uv;
uniform sampler2D sampler;
void main()
{
    Out_Color = texture(sampler, uv);
})FRAG";

        GLuint MvpID;
        GLuint SamplerID;
        GLuint RatioID;

        void init() {
            if (program == 0) {
                if (!create_program(program, vert, frag) || program == 0) {
                    Log::Error("Error creating program\n");
                    return;
                }
                MvpID = glGetUniformLocation(program, "mvp");
                SamplerID = glGetUniformLocation(program, "sampler");
                RatioID = glGetUniformLocation(program, "ratio");
            }
        }
    }

    namespace PaintShader {
        bool ok = false;

        GLuint program;
        const char* vert = R"VERT(#version 300 es
precision mediump float;
layout (location = 0) in vec3 Position;
void main()
{
    gl_Position = vec4(Position, 1);
})VERT";

        const char* frag = R"FRAG(#version 300 es
precision mediump float;
layout (location = 0) out vec4 Out_Color;
uniform vec2 center;
uniform float radius;
uniform float ratio;
void main()
{
    vec2 diff = gl_FragCoord.xy - center;
    if (diff.x * diff.x + diff.y * diff.y > radius * radius)
        discard;
    Out_Color = vec4(1, 0, 0, 1);
})FRAG";

        GLuint CenterID;
        GLuint RadiusID;
        GLuint RatioID;

        GLuint frameBuffer;

        void init() {
            if (ok)
                return;
            if (!create_program(program, vert, frag) || program == 0) {
                Log::Error("Error creating program\n");
                return;
            }
            CenterID = glGetUniformLocation(program, "center");
            RadiusID = glGetUniformLocation(program, "radius");
            RatioID = glGetUniformLocation(program, "ratio");

            glGenFramebuffers(1, &frameBuffer);
            ok = true;
        }
    }

}

void TexturedQuad::doStuff(const Camera& cam, const glm::vec2 cursor) {
    PaintShader::init();
    using namespace PaintShader;

    auto mvpInv = glm::inverse(cam.projection_view() * m_model);
    glm::vec4 close = mvpInv * glm::vec4(cursor, -1.0f, 1.0f);
    close /= close.w;
    glm::vec4 far = mvpInv * glm::vec4(cursor, 0.0f, 1.0f);
    far /= far.w;
    glm::vec4 dirNormalized = glm::normalize(far - close);
    const auto plane_normal = glm::vec4{0.f,0.f,1.0f,0.f};
    auto d = glm::dot(dirNormalized, plane_normal);
    if (std::abs(d) > std::numeric_limits<float>::epsilon())
    {
        auto intersection = close + glm::dot(glm::vec4{0.f,0.f,0.f,1.f} - close, plane_normal) / d * dirNormalized;
        glm::vec2 center = { intersection.x / m_ratio, intersection.y };
        if (std::abs(center.x) < 1.0f && std::abs(center.y) < 1.0f) {
            center += 1.0f;
            center /= 2.0f;
            center.y = 1.0f - center.y; // gl_FragCoord's origin is lower-left for some reason
            center *= glm::vec2{m_width, m_height};
            glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_draw_texture, 0);

            glViewport(0, 0, m_width, m_height);
            glUseProgram(program);

            float radius = 20.0f;
            glUniform2fv(CenterID, 1, &center[0]);
            glUniform1fv(RadiusID, 1, &radius);
            glUniform1fv(RatioID, 1, &m_ratio);

            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

            glDrawArrays(GL_TRIANGLES, 0, 6);

            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, Engine::width(), Engine::height());
        } else
            Log::Debug("Out");
    } else
        Log::Debug("No intersection");
}

void TexturedQuad::render(const Camera& cam, const glm::mat4& other)
{
    using namespace DrawShader;
    DrawShader::init();

    auto mvp = cam.projection_view() * m_model;

    glUseProgram(program);

    glUniformMatrix4fv(MvpID, 1, GL_FALSE, &mvp[0][0]);
    glUniform1fv(RatioID, 1, &m_ratio);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_draw_texture);
    glUniform1i(SamplerID, 0);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

	glEnableVertexAttribArray(1);
	glBindBuffer(GL_ARRAY_BUFFER, m_uvs);
	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}
