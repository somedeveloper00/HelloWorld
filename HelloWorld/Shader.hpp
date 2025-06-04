#ifndef SHADER_H
#define SHADER_H

#include <glad/glad.h>
#include <string>

struct Shader
{
    /// <summary>
    /// program's ID
    /// </summary>
    GLuint id;

    /// <summary>
    /// Create shader with vertex and fragment shader paths
    /// </summary>
    Shader(const std::string vertexSource, const std::string fragmentSource);

    /// <summary>
    /// Use the shader
    /// </summary>
    void Use() const;

    /// <summary>
    /// Find the location of the uniform
    /// </summary>
    GLint GetUniformLocation(const char* name) const;

    /// <summary>
    /// Set a int uniform's value
    /// </summary>
    void SetInt(GLint location, GLint value) const;

    /// <summary>
    /// Set a float uniform's value
    /// </summary>
    void SetFloat(GLint location, GLfloat value) const;
};

#endif // SHADER_H
