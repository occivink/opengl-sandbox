#include "manipulator.hpp"
#include "shader_functions.hpp"
#include "utils.hpp"
#include "log.hpp"

#include <vec3.hpp>
#include <gtc/matrix_transform.hpp>
#include <vector>

#define PI 3.1415

using namespace glm;

namespace { namespace TranslationBuffer {
    bool ok = false;
    GLuint buffer;
    int count;
    int vertsPerElem;

    void init() {
        if (ok)
            return;
        int cylinder_faces = 16;
        float radius = 0.01f;
        vertsPerElem = 9 * cylinder_faces;
        count = vertsPerElem * 3;
        std::vector<vec3> verts(count);

        auto circle = [=](int i){
            float f = (float)i * (PI * 2) / (float)cylinder_faces;
            return radius * vec2{ cos(f), sin(f) };
        };
        int ind = 0;
        vec2 prev_circle_pos = circle(0);
        for (int i = 1; i <= cylinder_faces; ++i) {
            auto circle_pos = circle(i);
            // cylinder
            verts[ind++] = { 0.f, prev_circle_pos };
            verts[ind++] = { 0.f, circle_pos };
            verts[ind++] = { .9f, prev_circle_pos };
            verts[ind++] = { .9f, prev_circle_pos };
            verts[ind++] = { .9f, circle_pos };
            verts[ind++] = { 0.f, circle_pos };
            // cone
            verts[ind++] = { .9f, 2.0f * prev_circle_pos };
            verts[ind++] = { .9f, 2.0f * circle_pos };
            verts[ind++] = { 1.f, 0.f, 0.f };
            prev_circle_pos = circle_pos;
        }
        for (int i = 0; i < vertsPerElem; ++i) {
            const auto& vert = verts[i];
            verts[vertsPerElem+i] = {vert.z, vert.x, vert.y};
            verts[2*vertsPerElem+i] = {vert.y, vert.z, vert.x};
        }

        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float) * 3, &verts[0][0], GL_STATIC_DRAW);

        ok = true;
    }
}}

namespace { namespace RotationBuffers {
    bool ok = false;
    GLuint buffer;
    int count;
    int vertsPerElem;

    void init() {
        if (ok)
            return;
        int major_segments = 64;
        int minor_segments = 12;
        float major_radius = 1.f;
        float minor_radius = 0.01f;
        int fraction = 1;
        vertsPerElem = major_segments * minor_segments * 6;
        count = major_segments * minor_segments * 6 * 3;
        std::vector<vec3> verts(count);

        auto circle = [=](int i, int max){
            float f = (float)i * (PI * 2) / (float)max;
            return vec2{ cos(f), sin(f) };
        };

        int ind = 0;
        auto axis = vec3{1.f, 0.f, 0.f};
        auto major_prev = vec3{0.f, circle(0, major_segments) };
        for (int i = 1; i <= major_segments / fraction; ++i) {
            auto major = vec3{0.f, circle(i, major_segments) };

            auto minor_prev = minor_radius * circle(0, minor_segments);
            for (int j = 0; j < minor_segments / fraction; ++j){
                auto minor = minor_radius * circle(j, minor_segments);
                verts[ind++] = major_radius * major + minor[0] * major + minor[1] * axis;
                verts[ind++] = major_radius * major_prev + minor[0] * major_prev + minor[1] * axis;
                verts[ind++] = major_radius * major_prev + minor_prev[0] * major_prev + minor_prev[1] * axis;
                verts[ind++] = major_radius * major + minor[0] * major + minor[1] * axis;
                verts[ind++] = major_radius * major + minor_prev[0] * major + minor_prev[1] * axis;
                verts[ind++] = major_radius * major_prev + minor_prev[0] * major_prev + minor_prev[1] * axis;
                minor_prev = minor;
            }

            major_prev = major;
        }

        // swizzle to generate the other elements
        for (int i = 0; i < vertsPerElem; ++i) {
            const auto& vert = verts[i];
            verts[vertsPerElem+i] = {vert.z, vert.x, vert.y};
            verts[2*vertsPerElem+i] = {vert.y, vert.z, vert.x};
        }

        glGenBuffers(1, &buffer);
        glBindBuffer(GL_ARRAY_BUFFER, buffer);
        glBufferData(GL_ARRAY_BUFFER, verts.size() * sizeof(float) * 3, &verts[0][0], GL_STATIC_DRAW);

        ok = true;
    }
}}

namespace { namespace ManipShader {
    GLuint program = 0;

    const char* vert = R"VERT(#version 300 es
precision mediump float;
layout (location = 0) in vec3 Position;
uniform mat4 mvp;
uniform int vertsPerElem;
uniform vec3 colors[3];
out vec4 color;
void main()
{
    color = vec4(colors[gl_VertexID / vertsPerElem], 0.5);
    gl_Position = mvp * vec4(Position.xyz, 1);
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
    GLuint ColorsID;
    GLuint VerticesPerElementID;

    void init() {
        if (program == 0) {
            if (!create_program(program, vert, frag) or program == 0) {
                Log::Error("Error creating program");
                return;
            }
            MvpID = glGetUniformLocation(program, "mvp");
            ColorsID = glGetUniformLocation(program, "colors");
            VerticesPerElementID = glGetUniformLocation(program, "vertsPerElem");
        }
    }
}}

Manipulator::Manipulator() {
    TranslationBuffer::init();
    RotationBuffers::init();
}

bool Manipulator::handle_input(const mat4& mvp, Input& in) {
    if (m_mode == Inactive)
        return false;

    enum {
        Start,
        Continue,
        End,
    } state = End;
    if (in.mouseStateChanged[0] and in.mouseDown[0]) {
        state = Start;
    } else if (in.mouseDown[0] and m_old_state) {
        state = Continue;
    }

    if (state == End) {
        m_activeAxis = None;
        m_old_state.reset();
        return false;
    }

    vec2 cursor = {
        2.0f * (float)in.mousePos[0] / (float)in.width - 1.0f,
        1.0f - (2.0f * (float)in.mousePos[1] / (float)in.height),
    };
    auto mvpInv = inverse(mvp * (state == Continue ? m_old_state->model : m_model));
    vec4 close = mvpInv * vec4(cursor, -1.0f, 1.0f);
    close /= close.w;
    vec4 far = mvpInv * vec4(cursor, 0.0f, 1.0f);
    far /= far.w;
    vec4 dir = normalize(far - close);
    vec4 center = vec4(0,0,0,1);

    if (state == Continue) {
        in.mouseCaptured = true;
        if (m_mode == Translation) {
            const auto& axis = m_old_state->axis;
            float d = dot(dir, axis);
            if (std::abs(d) > std::numeric_limits<float>::epsilon())
            {
                float t = (dot(dir, axis) * dot(dir, center-close) - dot(axis, center-close) * dot(dir,dir)) / (dot(dir,dir) * dot(axis,axis) - d * d);
                vec4 new_pos = center + t * axis;
                m_model = translate(m_old_state->model, vec3(new_pos - m_old_state->start_point));
            }
        } else if (m_mode == Rotation) {
            const auto& plane_normal = m_old_state->axis;
            float d = dot(dir, plane_normal);
            if (std::abs(d) > std::numeric_limits<float>::epsilon()) {
                auto intersection = close + dot(center - close, plane_normal) / d * dir;
                // something is fucked here
                float angle = acos(dot(normalize(m_old_state->start_point - center), normalize(intersection - center)));
                if (angle > std::numeric_limits<float>::epsilon()) {
                    vec3 rotation_vec = cross(vec3(m_old_state->start_point), vec3(intersection));
                    //Log::Error(rotation_vec);
                    m_model = rotate(m_old_state->model, angle, rotation_vec);
                }
            }
        }
        return true;
    } else if (state == Start) {
        // rough sphere test to early out
        auto stuff = (close - center) - dot((close - center), dir) * dir;
        float distance2 = dot(stuff, stuff);
        if (distance2 > 1.05)
            return false;

        auto set_old_state = [&](vec4 v, vec4 axis) {
            in.mouseCaptured = true;
            m_old_state.emplace();
            m_old_state->model = m_model;
            m_old_state->start_point = std::move(v);
            m_old_state->axis = std::move(axis);
        };

        int i = 0;
        if (m_mode == Translation) {
            for (auto&& axis : { vec4{1,0,0,0}, vec4{0,1,0,0}, vec4{0,0,1,0}}) {
                ++i;
                // test that the ray are not parallel
                float d = dot(dir, axis);
                if (not (std::abs(d) > std::numeric_limits<float>::epsilon()))
                    continue;
                {
                    // check that the ray-ray distance is small enough
                    auto c = cross(vec3(dir), vec3(axis));
                    float distance = abs(dot(vec3(close - center), c) / length(c));
                    if (distance > 0.03)
                        continue;
                }

                // check that the closest point on the axis falls on the [0,1] range
                float t = (dot(dir, axis) * dot(dir, center-close) - dot(axis, center-close) * dot(dir,dir)) / (dot(dir,dir) * dot(axis,axis) - d * d);
                if (t > 0.0 and t < 1.0) {
                    m_activeAxis = (ActiveAxis)i;
                    set_old_state(center + t * axis, std::move(axis));
                    return true;
                }
            }
        } else if (m_mode == Rotation) {
            for (auto&& plane_normal : { vec4{1,0,0,0}, vec4{0,1,0,0}, vec4{0,0,1,0} }) {
                ++i;
                // test that the normal the ray are not parallel
                float d = dot(dir, plane_normal);
                if (not (std::abs(d) > std::numeric_limits<float>::epsilon()))
                    continue;
                // plane-ray intersection
                auto intersection = close + dot(center - close, plane_normal) / d * dir;
                auto distance2 = sqrt(dot(intersection - center, intersection - center));
                // check that center-intersection length is ~1
                if (distance2 > 0.97f && distance2 < 1.03f) {
                    m_activeAxis = (ActiveAxis)i;
                    set_old_state(intersection, std::move(plane_normal));
                    return true;
                }
            }
        }
    }
    return false;
}

void Manipulator::render(mat4 mvp) const {
    if (m_mode == Inactive)
        return;

    mvp *= m_model;

    GLuint buffer;
    int count;
    int vertsPerElem;
    if (m_mode == Translation)
    {
        buffer = TranslationBuffer::buffer;
        count = TranslationBuffer::count;
        vertsPerElem = TranslationBuffer::vertsPerElem;
    }
    else if (m_mode == Rotation)
    {
        buffer = RotationBuffers::buffer;
        count = RotationBuffers::count;
        vertsPerElem = RotationBuffers::vertsPerElem;
    }
    else
        assert(false);

    using namespace ManipShader;
    init();

    glUseProgram(program);
    glEnableVertexAttribArray(0);
    glUniformMatrix4fv(MvpID, 1, GL_FALSE, &mvp[0][0]);
    const auto colors = make_array<vec3>(
        m_activeAxis == X ? vec3(1,0,0) : vec3(0.67,0.27,0.26),
        m_activeAxis == Y ? vec3(0,1,0) : vec3(0.36,0.66,0.13),
        m_activeAxis == Z ? vec3(0,0,1) : vec3(0.24,0.52,0.77)
    );
    glUniform3fv(ColorsID, 3, &colors[0][0]);
    glUniform1iv(VerticesPerElementID, 1, &vertsPerElem);
    glBindBuffer(GL_ARRAY_BUFFER, buffer);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, count);

}
