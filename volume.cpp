#include "volume.hpp"
#include "log.hpp"
#include "cube.hpp"
#include "shader_functions.hpp"
#include "engine.hpp"
#include "textured_quad.hpp"

Volume::Volume(const unsigned char* data, glm::ivec3 size, int c)
    : m_size(size)
    , m_channels(c)
    , m_ratio(glm::vec3(size) / glm::vec3(size.z))
{
    GLenum format = 0;
    switch (m_channels) {
        case 1: format = GL_RED; break;
        case 2: format = GL_RG; break;
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default: Log::Error("Invalid channels number"); break;
    }
    glGenTextures(1, &m_texture);

    glBindTexture(GL_TEXTURE_3D, m_texture);
    assert(m_texture != 0);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    unsigned char* other_data = new unsigned char[size.x * size.y * size.z * m_channels];
    int i = 0;
    for (int x = 0; x < size.x; ++x) {
        for (int y = 0; y < size.y; ++y) {
            for (int z = 0; z < size.z; ++z) {
                other_data[i++] = (float)x / size.x * 255.f;
                if (m_channels > 1) other_data[i++] = (float)y / size.y * 255.f;
                if (m_channels > 2) other_data[i++] = (float)z / size.z * 255.f;
                if (m_channels > 3) other_data[i++] = 30;
            }
        }
    }

    glTexImage3D(GL_TEXTURE_3D, 0, format, size.x, size.y, size.z, 0, format, GL_UNSIGNED_BYTE, other_data);

    delete[] other_data;

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
}

Volume::Volume(glm::ivec3 size, int c) : Volume(nullptr, size, c) {}

Volume::~Volume() {
    glDeleteTextures(1, &m_texture);
}

namespace { namespace VolumeShader {
    bool ok = false;

    GLuint program;

    const char* vert = R"VERT(#version 300 es
precision mediump float;
layout (location = 0) in vec3 Position;
out vec2 uv;
void main()
{
    uv = 0.5 * (Position.xy + vec2(1,1));
    gl_Position = vec4(Position.xyz, 1);
})VERT";

    const char* frag = R"FRAG(#version 300 es
precision mediump float;
precision mediump sampler3D;
layout (location = 0) out vec4 Out_Color;
in vec2 uv;
uniform sampler2D front;
uniform sampler2D back;
uniform sampler3D volume;
void main()
{
    vec4 tmp = texture(front, uv);
    if (tmp.a == 0.0)
        discard;
    vec3 pos = tmp.xyz;
    vec3 ray_end = texture(back, uv).xyz;
    vec3 dir = ray_end - pos;
    vec3 step = 0.15 * normalize(dir);
    float max_length2 = dot(dir, dir);
    float step_length2 = dot(step, step);
    int steps = clamp(int(floor(sqrt(max_length2 / step_length2))), 0, 50);

    vec4 dst = vec4(0.0);
    for (int i = 0; i < steps; ++i) {
        vec4 val = texture(volume, pos);

        val.rgb *= val.a;
        dst += (1.0f - dst.a) * val;

        if (dst.a > 0.95)
            break;
        pos += step;
    }
    Out_Color = dst;
})FRAG";

    GLuint FrontID;
    GLuint BackID;
    GLuint VolumeID;

    void init() {
        if (ok)
            return;
        if (!create_program(program, vert, frag) or program == 0) {
            Log::Error("Error creating program");
            return;
        }
        FrontID = glGetUniformLocation(program, "front");
        BackID = glGetUniformLocation(program, "back");
        VolumeID = glGetUniformLocation(program, "volume");

        ok = true;
    }
}}

void Volume::render(const glm::mat4& mvp) const {
    using namespace VolumeShader;
    VolumeShader::init();

    Cube cube;
    auto& in = Engine::input();
    // cache the textures if the viewport hasn't changed?
    TexturedQuad front(in.width, in.height, 4);
    cube.renderToTexture(mvp, m_ratio, true, front);
    TexturedQuad back(in.width, in.height, 4);
    cube.renderToTexture(mvp, m_ratio, false, back);

    glUseProgram(program);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, front.texture());
    glUniform1i(FrontID, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, back.texture());
    glUniform1i(BackID, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_3D, m_texture);
    glUniform1i(VolumeID, 2);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, TexturedQuad::verticesBuffer());
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}
