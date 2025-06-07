#pragma once

#include <glad/glad.h>
#include <string>
#include "glm/matrix.hpp"
#include <iostream>
#include "glm/gtc/type_ptr.hpp"

struct Shader
{
    /// <summary>
    /// program's ID
    /// </summary>
    GLuint id;

    /// <summary>
    /// whether linking worked correctly
    /// </summary>
    bool isValid;

    /// <summary>
    /// Create shader with vertex and fragment shader paths
    /// </summary>
    Shader(const std::string& vertexSource, const std::string& fragmentSource)
    {
        isValid = false;
        auto vertexShader = createShader(vertexSource.c_str(), GL_VERTEX_SHADER);
        auto fragmentShader = createShader(fragmentSource.c_str(), GL_FRAGMENT_SHADER);
        id = createProgram(vertexShader, fragmentShader);
        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);
    }

    /// <summary>
    /// Use the shader
    /// </summary>
    void Use() const
    {
        glUseProgram(id);
    }

    /// <summary>
    /// Find the location of the uniform
    /// </summary>
    GLint GetUniformLocation(const char* name) const
    {
        auto loc = glGetUniformLocation(id, name);
        if (loc == -1)
            std::cerr << "uniform \"" << name << "\" does not exist" << std::endl;
        return loc;
    }

    /// <summary>
    /// Set a int uniform's value
    /// </summary>
    void SetInt(GLint location, GLint value) const
    {
        glUniform1i(location, value);
    }

    /// <summary>
    /// Set a float uniform's value
    /// </summary>
    void SetFloat(GLint location, GLfloat value) const
    {
        glUniform1f(location, value);
    }

    /// <summary>
    /// Set a mat4 uniform's value
    /// </summary>
    void SetMat4(GLint location, glm::mat4 value) const
    {
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
    }

    /// <summary>
    /// Set a vec3 uniform's value
    /// </summary>
    void SetVec3(GLint location, glm::vec3 value) const
    {
        glUniform3f(location, value.x, value.y, value.z);
    }

    /// <summary>
    /// Set a vec4 uniform's value
    /// </summary>
    void SetVec4(GLint location, glm::vec4 value) const
    {
        glUniform4f(location, value.x, value.y, value.z, value.w);
    }

    explicit operator bool() const
    {
        return id > 0 && isValid;
    }

private:
    static inline GLuint createShader(const char* source, GLenum type)
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
            std::cerr << (type == GL_VERTEX_SHADER ? "vertex: " : "fragment: ") << buffer << std::endl;
        }
        return shader;
    }

    inline GLint createProgram(GLuint vertexShader, GLuint fragmentShader)
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
        else
            isValid = true;
        return program;
    }
};

