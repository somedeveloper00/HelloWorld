#pragma once

#include "../app.hpp"
#include "../graphics.hpp"
#include <algorithm>

namespace engine::test
{
struct renderTriangle : public engine::component
{
    GLuint shaderProgram;
    GLuint VAO;
    std::function<void()> renderHook;

    renderTriangle()
    {
        if (graphics::getRenderer() == graphics::renderer::opengl)
        {
            // create shader
            auto vertSrc = R"(
            #version 330 core
            layout(location = 0) in vec3 position;
            out vec3 fragPosition;
            void main()
            {
                gl_Position = vec4(position, 1.0);
                fragPosition = gl_Position;
            }
            )";

            auto fragSrc = R"(
            #version 330 core
            in vec3 fragPosition;
            out vec4 color;
            void main()
            {
                color = vec4(fragPosition, 1.0);
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
            shaderProgram = glCreateProgram();
            glAttachShader(shaderProgram, vertexShader);
            glAttachShader(shaderProgram, fragmentShader);
            glLinkProgram(shaderProgram);
            // cleanup
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            // set up triangle vertices
            GLfloat vertices[] = {
                -0.5f, -0.5f, 0.0f, // bottom left
                0.5f, -0.5f, 0.0f,  // bottom right
                0.0f, 0.5f, 0.0f    // top
            };
            // create VAO and VBO
            GLuint VBO;
            glGenVertexArrays(1, &VAO);
            glGenBuffers(1, &VBO);
            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            renderHook = [this]() { render(); };
            graphics::opengl::onRendersMutex.lock();
            graphics::opengl::onRenders[0].push_back(renderHook);
            graphics::opengl::onRendersMutex.unlock();
        }
    }

    ~renderTriangle()
    {
        graphics::opengl::onRendersMutex.lock();
        auto &row = graphics::opengl::onRenders[0];
        row.erase(std::remove_if(row.begin(), row.end(), [_renderHook = renderHook](const std::function<void()> &func) {
                      return func.target<void (*)()>() == _renderHook.target<void (*)()>();
                  }),
                  row.end());
        graphics::opengl::onRendersMutex.unlock();
    }

    void render()
    {
        // draw
        glUseProgram(shaderProgram);
        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, 3);
        glBindVertexArray(0);
    }
};
} // namespace engine::test