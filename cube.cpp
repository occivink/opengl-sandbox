#include "cube.hpp"
#include "engine.hpp"
#include "shader_functions.hpp"

#include <cstdio>
#include <vector>

Cube::Cube() {
    static const std::vector<glm::vec3> verts = {
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f,  1.0f),
        glm::vec3(-1.0f,  1.0f,  1.0f),
        glm::vec3( 1.0f,  1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f, -1.0f),
        glm::vec3( 1.0f, -1.0f,  1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3( 1.0f, -1.0f, -1.0f),
        glm::vec3( 1.0f,  1.0f, -1.0f),
        glm::vec3( 1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f,  1.0f),
        glm::vec3(-1.0f,  1.0f, -1.0f),
        glm::vec3( 1.0f, -1.0f,  1.0f),
        glm::vec3(-1.0f, -1.0f,  1.0f),
        glm::vec3(-1.0f, -1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f,  1.0f),
        glm::vec3(-1.0f, -1.0f,  1.0f),
        glm::vec3( 1.0f, -1.0f,  1.0f),
        glm::vec3( 1.0f,  1.0f,  1.0f),
        glm::vec3( 1.0f, -1.0f, -1.0f),
        glm::vec3( 1.0f,  1.0f, -1.0f),
        glm::vec3( 1.0f, -1.0f, -1.0f),
        glm::vec3( 1.0f,  1.0f,  1.0f),
        glm::vec3( 1.0f, -1.0f,  1.0f),
        glm::vec3( 1.0f,  1.0f,  1.0f),
        glm::vec3( 1.0f,  1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f, -1.0f),
        glm::vec3( 1.0f,  1.0f,  1.0f),
        glm::vec3(-1.0f,  1.0f, -1.0f),
        glm::vec3(-1.0f,  1.0f,  1.0f),
        glm::vec3( 1.0f,  1.0f,  1.0f),
        glm::vec3(-1.0f,  1.0f,  1.0f),
        glm::vec3( 1.0f, -1.0f,  1.0f)
    };
    glGenBuffers(1, &m_vertices);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glBufferData(GL_ARRAY_BUFFER, verts.size() * 3 * sizeof(float), &verts[0][0], GL_STATIC_DRAW);
}

namespace {
    GLuint program = 0;

    const char* vert = R"VERT(#version 300 es
precision mediump float;
layout (location = 0) in vec3 Position;
uniform mat4 mvp;
out vec4 color;
void main()
{
    color = vec4(Position.xyz, 1);
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

}

void Cube::render(const Camera& cam, const glm::mat4& other)
{
    if (program == 0) {
        if (!create_program(program, vert, frag) or program == 0)
            printf("Error creating program\n");
    }

    auto mvp = cam.projection_view() * m_model;

    glUseProgram(program);
    glEnableVertexAttribArray(0);
    GLuint MatrixID = glGetUniformLocation(program, "mvp");
    glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &mvp[0][0]);
    glBindBuffer(GL_ARRAY_BUFFER, m_vertices);
    glVertexAttribPointer(0, 3, GL_FLOAT, 0, 0, 0);

    glDrawArrays(GL_TRIANGLES, 0, 36);
}
