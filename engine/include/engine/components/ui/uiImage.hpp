#pragma once

#include "engine/components/camera.hpp"
#include "engine/components/pointerRead.hpp"
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
    static inline GLuint s_vao;
    uiTransform *_uiTransform;
    pointerRead *_pointerRead;

    bool created_() override
    {
        initialize_();
        disallowMultipleComponents(uiImage);
        if (!(_uiTransform = getEntity()->ensureComponentExists<uiTransform>()))
            return false;
        if (!(_pointerRead = getEntity()->ensureComponentExists<pointerRead>()))
            return false;
        _uiTransform->pushLock();
        _pointerRead->pushLock();
        _pointerRead->setVertices(s_vao, 6);
        s_instances.push_back(this);
        return true;
    }

    void removed_() override
    {
        s_instances.erase(this);
        _uiTransform->popLock();
        _pointerRead->popLock();
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
            uniform mat4 viewMatrix;
            uniform mat4 projectionMatrix;

            void main()
            {
                gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 0.f, 1.f);
            }
            )";
            static const auto fragmentShader = R"(
            #version 460 core
            uniform vec4 color;
            out vec4 result;

            void main()
            {
                result = color;
            }
            )";

            static GLuint program;
            static GLint modelMatrixLocation;
            static GLint viewMatrixLocation;
            static GLint projectionMatrixLocation;
            static GLint colorLocation;
            
            // make program data ready
            program = graphics::opengl::fatalCreateProgram("uiImage", vertexShader, fragmentShader);
            modelMatrixLocation = graphics::opengl::fatalGetLocation(program, "modelMatrix");
            viewMatrixLocation = graphics::opengl::fatalGetLocation(program, "viewMatrix");
            projectionMatrixLocation = graphics::opengl::fatalGetLocation(program, "projectionMatrix");
            colorLocation = graphics::opengl::fatalGetLocation(program, "color");
            s_vao = graphics::opengl::getSquareVao();

            graphics::opengl::addRendererHook(1, []() {
                bench("rendering uiImage");

                const auto *mainCamera = camera::getMainCamera();
                if (!mainCamera)
                {
                    log::logWarning("not rendering uiImage because there's no camera to render to");
                    return;
                }
                const auto &viewMatrix = mainCamera->getViewMatrix();
                const auto &projectionMatrix = mainCamera->getProjectionMatrix();
                glBindVertexArray(s_vao);
                glUseProgram(program);
                glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(viewMatrix));
                glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(projectionMatrix));
                s_instances.forEach([&](const uiImage *instance) {
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