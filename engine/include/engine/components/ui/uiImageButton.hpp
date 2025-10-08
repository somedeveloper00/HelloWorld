#pragma once

#include "engine/app.hpp"
#include "engine/components/camera.hpp"
#include "engine/data.hpp"
#include "engine/window.hpp"
#include "glm/gtc/type_ptr.hpp"
#include "uiButton.hpp"

namespace engine::ui
{
struct uiImageButton : public uiButton
{
    // color for when the button is neither pressed nor hovered over
    color idleColor = {1, 1, 1, 1};

    // color for when the button is selected or hovered over but not pressed
    color selectedColor = {0.9f, 0.9f, 0.9f, 1};

    // color for when the button is pressed (different from when hovered over or selected but not pressed)
    color pressedColor = {0.5f, 0.5f, 0.5f, 1};

    // color for when the button is not enabled
    color disabledColor = {0.9f, 0.9f, 0.9f, 0.5f};

    // rate of color change per second (color range is 0-1, so change rate of 1 means it at worse takes 1 second to switch color)
    float colorSwitchSpeed = 20;

    createTypeInformation(uiImageButton, uiButton);

  protected:
    static inline quickVector<uiImageButton *> s_instances{};
    color _currentColor{};
    color _targetColor{};

    bool created_() override
    {
        initialize_();
        if (!uiButton::created_())
            return false;
        s_instances.push_back(this);
        if (graphics::getRenderer() == graphics::opengl)
            _pointerRead->setVertices(graphics::opengl::getSquareVao(), 6);
        _currentColor = getEnabled() ? idleColor : disabledColor;
        _targetColor = _currentColor;
        return true;
    }

    void disabled_() override
    {
        uiButton::disabled_();
        _targetColor = disabledColor;
    }

    void removed_() override
    {
        uiButton::removed_();
        s_instances.erase(this);
    }

    void onSelected() override
    {
        _targetColor = selectedColor;
    }

    void onUnselected() override
    {
        _targetColor = idleColor;
    }

    void onPointerDown() override
    {
        uiButton::onPointerDown();
        _targetColor = pressedColor;
    }

    void onPointerUp() override
    {
        uiButton::onPointerDown();
        _targetColor = selectedColor;
    }

    void onPointerEnter() override
    {
        select();
    }

    void onPointerExit() override
    {
        unselect();
    }

  private:
    static inline void initialize_()
    {
        ensureExecutesOnce();
        application::preComponentHooks.push_back([]() {
            bench("updating uiImageButtons");
            s_instances.forEachParallel([](uiImageButton *instance) {
                instance->_currentColor.lerp(instance->_targetColor, time::getDeltaTime() * instance->colorSwitchSpeed);
            });
        });
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

            // make program and vao/ebo ready
            static GLuint s_program = graphics::opengl::createProgram(vertexShader, fragmentShader);
            fatalAssert(s_program, "could not create opengl program for uiImageButton component");
            static GLint modelMatrixLocation = graphics::opengl::fatalGetLocation(s_program, "modelMatrix");
            static GLint viewMatrixLocation = graphics::opengl::fatalGetLocation(s_program, "viewMatrix");
            static GLint projectionMatrixLocation = graphics::opengl::fatalGetLocation(s_program, "projectionMatrix");
            static GLint colorLocation = graphics::opengl::fatalGetLocation(s_program, "color");
            static GLuint s_vao = graphics::opengl::getSquareVao();

            graphics::opengl::addRendererHook(1, []() {
                bench("rendering uiImageButtons");
                const auto *camera = camera::getMainCamera();
                if (!camera)
                    return;
                glBindVertexArray(s_vao);
                glUseProgram(s_program);
                glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(camera->getViewMatrix()));
                glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(camera->getProjectionMatrix()));
                s_instances.forEach([](const uiImageButton *instance) {
                    const auto matrix = instance->_uiTransform->getGlobalMatrix();
                    glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(matrix));
                    glUniform4f(colorLocation, instance->_currentColor.r, instance->_currentColor.g, instance->_currentColor.b, instance->_currentColor.a);
                    glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
                });
            });
        }
    }
};
} // namespace engine::ui