#include "Shader.hpp"
#include <fstream>
#include <string>
#include <iostream>

static GLuint createShader(const char* source, GLenum type)
{
    GLuint shader = glCreateShader(type);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    GLint success;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        char buffer[512];
        glGetShaderInfoLog(shader, 512, NULL, buffer);
        std::cerr << "vertex: " << buffer << std::endl;
    }
    return shader;
}

static GLint createProgram(GLuint vertexShader, GLuint fragmentShader)
{
    auto program = glCreateProgram();
    glAttachShader(program, vertexShader);
    glAttachShader(program, fragmentShader);
    glLinkProgram(program);
    GLint success;
    glGetProgramiv(program, GL_LINK_STATUS, &success);
    if (!success)
    {
        char buffer[512];
        glGetProgramInfoLog(program, 512, NULL, buffer);
        std::cerr << "link: " << buffer << std::endl;
    }
    return program;
}

Shader::Shader(const std::string vertexSource, const std::string fragmentSource)
{
    auto vertexShader = createShader(vertexSource.c_str(), GL_VERTEX_SHADER);
    auto fragmentShader = createShader(fragmentSource.c_str(), GL_FRAGMENT_SHADER);
    id = createProgram(vertexShader, fragmentShader);
    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);
}

void Shader::Use() const
{
    glUseProgram(id);
}

GLint Shader::GetUniformLocation(const char* name) const
{
    return glGetUniformLocation(id, name);
}

void Shader::SetInt(GLint location, GLint value) const
{
    glUniform1i(location, value);
}

void Shader::SetFloat(GLint location, GLfloat value) const
{
    glUniform1f(location, value);
}

