#include "textured_quad.hpp"
#include "log.hpp"
#include "engine.hpp"
#include "utils.hpp"
#include "shader_functions.hpp"

#include <vector>
#include <cstdio>

namespace { namespace Buffers {
    bool ok = false;
    GLuint verticesBuffer;

    void init() {
        if (ok)
            return;

        const auto verts = make_array<float>(
            -1.f, -1.f,  0.f,
            -1.f,  1.f,  0.f,
             1.f,  1.f,  0.f,
            -1.f, -1.f,  0.f,
             1.f, -1.f,  0.f,
             1.f,  1.f,  0.f
        );
        glGenBuffers(1, &verticesBuffer);
        glBindBuffer(GL_ARRAY_BUFFER, verticesBuffer);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float), verts.data(), GL_STATIC_DRAW);

        ok = true;
    }
}}

GLuint TexturedQuad::verticesBuffer() {
    Buffers::init();
    return Buffers::verticesBuffer;
}

TexturedQuad::TexturedQuad(const unsigned char* data, int w, int h, int c, bool nearest)
    : m_width(w)
    , m_height(h)
    , m_channels(c)
    , m_ratio((float)w / (float)h)
{
    Buffers::init();
    GLenum format = 0;
    switch (m_channels) {
        case 1: format = GL_RED; break;
        case 2: format = GL_RG; break;
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default: Log::Error("Invalid channels number"); break;
    }
    glGenTextures(1, &m_texture);

    glBindTexture(GL_TEXTURE_2D, m_texture);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, format, w, h, 0, format, GL_UNSIGNED_BYTE, data);

    if (nearest) {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    } else {
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    }
}

TexturedQuad::TexturedQuad(int w, int h, int c, bool nearest)
    : TexturedQuad(0, w, h, c, nearest)
{}

TexturedQuad::~TexturedQuad() {
    glDeleteTextures(1, &m_texture);
}

namespace { namespace DrawShader {
    GLuint program = 0;

    const char* vert = R"VERT(#version 300 es
precision mediump float;
layout (location = 0) in vec3 Position;
uniform float ratio;
uniform mat4 mvp;
out vec2 uv;
void main()
{
    uv = 0.5 * (Position.xy + vec2(1,1));
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
            if (!create_program(program, vert, frag) or program == 0) {
                Log::Error("Error creating program");
                return;
            }
            MvpID = glGetUniformLocation(program, "mvp");
            SamplerID = glGetUniformLocation(program, "sampler");
            RatioID = glGetUniformLocation(program, "ratio");
        }
    }
}}

void TexturedQuad::render(const Camera& cam, const glm::mat4* model) const
{
    using namespace DrawShader;
    DrawShader::init();

    auto mvp = model ? cam.projection_view() * *model : cam.projection_view();

    glUseProgram(program);

    glUniformMatrix4fv(MvpID, 1, GL_FALSE, &mvp[0][0]);
    glUniform1fv(RatioID, 1, &m_ratio);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glUniform1i(SamplerID, 0);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, Buffers::verticesBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

namespace { namespace PaintShader {
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
uniform vec2 segmentA;
uniform vec2 segmentB;
uniform float radius;
uniform float blob_ratio;
uniform float label_color;
void main()
{
    vec2 pos = vec2(gl_FragCoord.x * blob_ratio, gl_FragCoord.y); // put pos in original image coordinates
    float segmentLength = dot(segmentB -segmentA, segmentB - segmentA);
    if (segmentLength == 0.0) {
        if (dot(pos - segmentA, pos - segmentA) > radius * radius)
            discard;
        Out_Color = vec4(label_color, 0, 0, 1);
        return;
    }
    float t = clamp(dot(pos -segmentA, segmentB - segmentA) / segmentLength, 0.0, 1.0);
    vec2 projection = segmentA + t * (segmentB - segmentA);
    if (dot(pos -projection, pos - projection) > radius * radius)
        discard;
    Out_Color = vec4(label_color, 0, 0, 1);
})FRAG";

    GLuint SegmentAID;
    GLuint SegmentBID;
    GLuint RadiusID;
    GLuint BlobRatioID;
    GLuint LabelColorID;

    GLuint frameBuffer;

    void init() {
        if (ok)
            return;
        if (!create_program(program, vert, frag) or program == 0) {
            Log::Error("Error creating program");
            return;
        }
        SegmentAID = glGetUniformLocation(program, "segmentA");
        SegmentBID = glGetUniformLocation(program, "segmentB");
        RadiusID = glGetUniformLocation(program, "radius");
        BlobRatioID = glGetUniformLocation(program, "blob_ratio");
        LabelColorID = glGetUniformLocation(program, "label_color");

        glGenFramebuffers(1, &frameBuffer);
        ok = true;
    }
}}

bool TexturedQuad::unproject(const Camera& cam,
                             const glm::mat4* model,
                             const glm::vec2& cursor,
                             glm::vec2& outPicked) const
{
    auto mvpInv = glm::inverse(model ? cam.projection_view() * *model : cam.projection_view());
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
        outPicked = { intersection.x , intersection.y };
        return true;
    }
    return false;
}

void TexturedQuad::paint(glm::vec2 from,
                         glm::vec2 to,
                         int color,
                         float radius,
                         float blob_ratio)
{
    blob_ratio /= m_ratio;
    from *= glm::vec2{ blob_ratio * m_width, m_height };
    to *= glm::vec2{ blob_ratio * m_width, m_height };
    if (   (from.x + radius < 0.0                  and to.x + radius < 0.0)
        or (from.x - radius > blob_ratio * m_width and to.x - radius > blob_ratio * m_width)
        or (from.y + radius < 0.0                  and to.y + radius < 0.0)
        or (from.y - radius > m_height             and to.y - radius > m_height))
    {
        Log::Debug("Early out");
        return;
    }

    using namespace PaintShader;
    init();

    glBindFramebuffer(GL_FRAMEBUFFER, frameBuffer);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

    glViewport(0, 0, m_width, m_height);
    glUseProgram(program);

    glUniform2fv(SegmentAID, 1, &to[0]);
    glUniform2fv(SegmentBID, 1, &from[0]);
    glUniform1fv(RadiusID, 1, &radius);
    glUniform1fv(BlobRatioID, 1, &blob_ratio);
    float color_floated = (float)color / 255.0f;
    glUniform1fv(LabelColorID, 1, &color_floated);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, Buffers::verticesBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // TODO restore
    glViewport(0, 0, Engine::input().width, Engine::input().height);
}

namespace { namespace TextureWithLabelsShader {
    bool okay = false;
    GLuint program;

    const char* vert = R"VERT(#version 300 es
precision mediump float;
layout (location = 0) in vec3 Position;
uniform float ratio;
uniform mat4 mvp;
out vec2 uv;
void main()
{
    uv = 0.5 * (Position.xy + vec2(1,1));
    gl_Position = mvp * vec4(ratio * Position.x, Position.y, Position.z, 1);
})VERT";

    const char* frag = R"FRAG(#version 300 es
precision mediump float;
layout (location = 0) out vec4 Out_Color;
in vec2 uv;
uniform float factor;
uniform sampler2D tex1Sampler;
uniform sampler2D tex2Sampler;
void main()
{
    vec4 sample1 = texture(tex1Sampler, uv);
    vec4 sample2 = texture(tex2Sampler, uv);
    int index = int(round(clamp(255.0f * sample2.r, 0.0f, 254.0f)));
    if (index == 1)
        Out_Color = mix(sample1, vec4(0,1,0,1), factor);
    else if (index == 2)
        Out_Color = mix(sample1, vec4(1,0,0,1), factor);
    else
        Out_Color = sample1;
})FRAG";

    GLuint MvpID;
    GLuint RatioID;
    GLuint FactorID;
    GLuint Tex1SamplerID;
    GLuint Tex2SamplerID;

    void init() {
        if (okay)
            return;
        if (!create_program(program, vert, frag) or program == 0) {
            Log::Error("Error creating program");
            return;
        }
        MvpID = glGetUniformLocation(program, "mvp");
        RatioID = glGetUniformLocation(program, "ratio");
        FactorID = glGetUniformLocation(program, "factor");
        Tex1SamplerID = glGetUniformLocation(program, "tex1Sampler");
        Tex2SamplerID = glGetUniformLocation(program, "tex2Sampler");
        okay = true;
    }
}}

void TexturedQuad::renderWithLabels(const Camera& cam,
                                   const TexturedQuad& labels,
                                   float label_opacity,
                                   const glm::mat4* model) const
{
    using namespace TextureWithLabelsShader;
    init();

    auto mvp = model ? cam.projection_view() * *model : cam.projection_view();

    glUseProgram(program);

    glUniformMatrix4fv(MvpID, 1, GL_FALSE, &mvp[0][0]);
    glUniform1fv(RatioID, 1, &m_ratio);
    glUniform1fv(FactorID, 1, &label_opacity);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_texture);
    glUniform1i(Tex1SamplerID, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, labels.m_texture);
    glUniform1i(Tex2SamplerID, 1);

    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, Buffers::verticesBuffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_TRIANGLES, 0, 6);
}

bool TexturedQuad::exportPixels(std::vector<unsigned char>& pixels) const
{
    GLuint fb;
    glGenFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, fb);

    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_texture, 0);

    glViewport(0, 0, m_width, m_height);

    GLenum format = 0;
    switch (m_channels) {
        case 1: format = GL_RED; break;
        case 2: format = GL_RG; break;
        case 3: format = GL_RGB; break;
        case 4: format = GL_RGBA; break;
        default: Log::Error("Invalid channels number"); break;
    }
    pixels.resize(m_width * m_height * m_channels);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, m_width, m_height, format, GL_UNSIGNED_BYTE, pixels.data());

    glDeleteFramebuffers(1, &fb);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
    // TODO restore
    glViewport(0, 0, Engine::input().width, Engine::input().height);
    return true;
}
