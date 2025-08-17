#pragma once

#include "../app.hpp"
#include "../window.hpp"
#include <algorithm>
#include <memory>
#include <mutex>

namespace engine::test
{
struct renderTriangle : public component
{
    friend entity;
    float swaySpeed = 1;

    renderTriangle()
    {
        static bool s_initialized;
        if (s_initialized)
            return;
        s_initialized = true;

        // initialize
        if (graphics::getRenderer() == graphics::renderer::opengl)
        {
            // create shader
            auto vertSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 position;
            uniform vec3 offset;
            out vec3 fragPosition;
            void main()
            {
                gl_Position = vec4(position + offset, 1.0);
                fragPosition = position + offset;
            }
            )";

            auto fragSrc = R"(
            #version 330 core
            in vec3 fragPosition;
            out vec4 color;
            void main()
            {
                color = vec4(fragPosition+0.5, 1.0);
            }
            )";

            // compile shaders
            GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
            glShaderSource(vertexShader, 1, &vertSrc, nullptr);
            glCompileShader(vertexShader);
            GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
            glShaderSource(fragmentShader, 1, &fragSrc, nullptr);
            glCompileShader(fragmentShader);
            // link shaders
            s_shaderProgram = glCreateProgram();
            glAttachShader(s_shaderProgram, vertexShader);
            glAttachShader(s_shaderProgram, fragmentShader);
            glLinkProgram(s_shaderProgram);
            // cleanup
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);

            s_offsetLocation = glGetUniformLocation(s_shaderProgram, "offset");

            // set up triangle vertices
            GLfloat vertices[] = {
                -0.5f, -0.5f, 0.0f, // bottom left
                0.5f, -0.5f, 0.0f,  // bottom right
                0.0f, 0.5f, 0.0f    // top
            };
            // create _vao and VBO
            GLuint VBO;
            glGenVertexArrays(1, &s_vao);
            glGenBuffers(1, &VBO);
            glBindVertexArray(s_vao);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            graphics::opengl::onRendersMutex.lock();
            graphics::opengl::onRenders[0].push_back([]() {
                // draw
                glUseProgram(s_shaderProgram);
                glBindVertexArray(s_vao);
                for (auto &instance : s_instances)
                {
                    auto &offset = dynamic_cast<renderTriangle *>(instance.lock().get())->_offset;
                    glUniform3f(s_offsetLocation, offset.x, offset.y, offset.z);
                    glDrawArrays(GL_TRIANGLES, 0, 3);
                }
                glUseProgram(0);
                glBindVertexArray(0);
            });
            graphics::opengl::onRendersMutex.unlock();
        }
    }

  private:
    static inline auto &s_instances = *new std::vector<std::weak_ptr<component>>();
    static inline GLuint s_shaderProgram;
    static inline GLuint s_vao;
    static inline GLint s_offsetLocation;
    glm::vec3 _offset = {0, 0, 0};
    float _startTime;

    void created_() override
    {
        s_instances.push_back(getWeakPtr());
        _startTime = time::getTotalTime();
    }

    void removed_() override
    {
        s_instances.erase(std::remove(s_instances.begin(), s_instances.end(), getWeakPtr()));
    }

    void update_() override
    {
        _offset.x = sin((time::getTotalTime() - _startTime) * swaySpeed);
    }
};
} // namespace engine::test