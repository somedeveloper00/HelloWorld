#pragma once

#include "engine/components/ui/canvasRendering.hpp"
#include "engine/data.hpp"
#include "engine/errorHandling.hpp"
#include "engine/window.hpp"
#include "glm/gtc/type_ptr.hpp"

namespace engine::ui
{
// drawa simple 2D image on the UI
struct uiImage : public component
{
    createTypeInformation(uiImage, component);
    color color{1, 1, 1, 1};

  protected:
    static inline quickVector<uiImage *> s_instances;
    weakRef<uiTransform> _uiTransform;

    void created_() override
    {
        initialize_();
        _uiTransform = getEntity()->ensureComponentExists<uiTransform>();
        s_instances.push_back(this);
    }

    void removed_() override
    {
        s_instances.erase(this);
    }

  private:
    static inline void initialize_()
    {
        ensureExecutesOnce();
        if (graphics::getRenderer() == graphics::renderer::opengl)
        {
            static const auto vertexShader = R"(
            #version 460 core
            layout(location = 0) in vec2 position;
            uniform mat4 modelMatrix;
            uniform vec4 color;
            out vec4 Color;

            void main()
            {
                gl_Position = modelMatrix * vec4(position, 0.f, 1.f);
                Color = color;
            }
            )";
            static const auto fragmentShader = R"(
            #version 460 core
            in vec4 Color;
            out vec4 result;

            void main()
            {
                result = Color;
            }
            )";

            // make program and vao/ebo ready
            static GLuint program = graphics::opengl::createProgram(vertexShader, fragmentShader);
            fatalAssert(program, "could not create opengl program for uiImage component");
            static GLint modelMatrixLocation = glGetUniformLocation(program, "modelMatrix");
            static GLint colorLocation = glGetUniformLocation(program, "color");

            static const float vertices[] = {
                1, 1,
                1, -1,
                -1, -1,
                -1, 1};
            static const GLuint indices[] = {
                0, 1, 2,
                2, 3, 0};
            static GLuint vao;
            GLuint vbo, ebo;
            glGenVertexArrays(1, &vao);
            glGenBuffers(1, &vbo);
            glGenBuffers(1, &ebo);

            glBindVertexArray(vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
            glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

            // position
            glEnableVertexAttribArray(0);
            glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

            glBindVertexArray(0);

            graphics::opengl::addRendererHook(1, []() {
                bench("rendering uiImage");
                glBindVertexArray(vao);
                glUseProgram(program);
                s_instances.forEach([](const uiImage *instance) {
                    const auto &matrix = instance->_uiTransform->getGlobalMatrix();
                    const auto &color = instance->color;
                    glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(matrix));
                    glUniform4f(colorLocation, color.r, color.g, color.b, color.a);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                });
            });
        };
    }
};
} // namespace engine::ui