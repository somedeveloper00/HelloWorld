#pragma once

#include "engine/app.hpp"
#include "engine/components/componentUtility.hpp"
#include "engine/components/ui/uiTransform.hpp"
#include "engine/window.hpp"
#include <memory>

namespace engine::ui
{
struct uiRenderer : public component
{
    // could be null
    std::shared_ptr<uiTransform> getUiTransform() const noexcept
    {
        return _uiTransform.lock();
    }

  private:
    // gets called during opengl drawings
    // the parameter function calls glDrawElements, so should be called per object
    static inline auto &s_openGlShaderHandlers = *new std::vector<std::function<void(std::function<void()>)>>();
    std::weak_ptr<uiTransform> _uiTransform;

    static inline void initialize_()
    {
        executeOnce();
        // initialize
        if (graphics::getRenderer() == graphics::renderer::opengl)
        {
            // initialize common data
            GLfloat vertices[] = {
                -0.5f, -0.5f, 0.0f, // bottom left
                0.5f, -0.5f, 0.0f,  // bottom right
                0.5f, 0.5f, 0.0f,   // top right
                0.5f, -0.5f, 0.0f   // top left
            };
            static GLuint s_indices[] = {
                3,
                1,
                0,
                3,
                2,
                1,
            };
            static GLuint s_vao;
            GLuint vbo;
            glGenVertexArrays(1, &s_vao);
            glGenBuffers(1, &vbo);
            glBindVertexArray(s_vao);
            glBindBuffer(GL_ARRAY_BUFFER, vbo);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);
            glBindVertexArray(0);

            graphics::opengl::onRendersMutex.lock();
            graphics::opengl::onRenders[0].push_back([]() {
                // draw
                glBindVertexArray(s_vao);
                static auto drawCallback = []() {
                    glDrawElements(GL_TRIANGLES, static_cast<GLsizei>(sizeof(s_indices) / sizeof(decltype(s_indices[0]))), GL_UNSIGNED_INT, static_cast<void *>(s_indices));
                };
                for (auto &handler : s_openGlShaderHandlers)
                    handler(drawCallback);
                glUseProgram(0);
                glBindVertexArray(0);
            });
            graphics::opengl::onRendersMutex.unlock();
        }
    }

    void created_() override
    {
        initialize_();

        // ensure uiTransform component exists
        auto sharedPtr = getEntity()->getComponent<uiTransform>();
        if (!sharedPtr)
            sharedPtr = getEntity()->addComponent<uiTransform>();
        _uiTransform = sharedPtr;
    }
};
} // namespace engine::ui