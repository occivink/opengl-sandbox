#include "shader_functions.hpp"
#include "log.hpp"

GLuint load_shader(GLenum type, const char *shaderSrc)
{
    GLuint shader;
    GLint compiled;

    shader = glCreateShader(type);
    if (shader == 0)
        return 0;

    glShaderSource(shader, 1, &shaderSrc, nullptr);
    glCompileShader(shader);
    glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
    if (!compiled)
    {
        GLint infoLen = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            char* infoLog = new char[infoLen];
            glGetShaderInfoLog(shader, infoLen, nullptr, infoLog);
            Log::Error(std::string("Error compiling shader: ") + infoLog);
            delete[] infoLog;
        }
        glDeleteShader(shader);
        return 0;
    }
    return shader;
}

bool create_program(GLuint& prog, const char* vert, const char* frag)
{
    GLuint vertexShader = load_shader(GL_VERTEX_SHADER, vert);
    if (!vertexShader)
        return false;
    GLuint fragmentShader = load_shader(GL_FRAGMENT_SHADER, frag);
    if (!fragmentShader)
        return false;
    GLint linked;

    prog = glCreateProgram();
    if (prog == 0)
        return false;

    glAttachShader(prog, vertexShader);
    glAttachShader(prog, fragmentShader);
    glBindAttribLocation(prog, 0, "vPosition");
    glLinkProgram(prog);
    glGetProgramiv(prog, GL_LINK_STATUS, &linked);
    if (!linked)
    {
        GLint infoLen = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &infoLen);
        if (infoLen > 1)
        {
            char* infoLog = new char[infoLen];
            glGetProgramInfoLog(prog, infoLen, nullptr, infoLog);
            Log::Error(std::string("Error linking program: ") + infoLog);
            delete[] infoLog;
        }
        glDeleteProgram(prog);
        return false;
    }
    return true;
}
