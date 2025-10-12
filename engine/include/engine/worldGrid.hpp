#pragma once

#include "common/componentUtils.hpp"
#include "data.hpp"
#include "engine/components/camera.hpp"
#include "engine/window.hpp"
#include "glm/common.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/geometric.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <cstdlib>

namespace engine
{
struct worldGridSystem final
{
    worldGridSystem() = delete;

    static constexpr float CameraDistanceMultipler = 0.002f;

    static constexpr float AxisThickness = 1;
    static constexpr float AxisLength = 10000;
    static constexpr color AxisXColor = {0, 0, 1, 1};
    static constexpr color AxisYColor = {0, 1, 0, 1};
    static constexpr color AxisZColor = {1, 0, 0, 1};

    static constexpr size_t GridCount = 10;
    static constexpr float GridThickness = 1;
    static constexpr color GridColor = {1, 1, 1, 0.3f};

    static inline void initialize()
    {
        ensureExecutesOnce();
        if (graphics::getRenderer() == graphics::renderer::opengl)
        {
            static GLuint vao = graphics::opengl::getCubeVao();
            static GLuint colorLocation, modelMatrixLocation, viewMatrixLocation, projectionMatrixLocation;
            static GLuint program = graphics::opengl::getBasicFlatShader(colorLocation, modelMatrixLocation, viewMatrixLocation, projectionMatrixLocation);

            // set defaults
            glUseProgram(program);
            glUniform4f(colorLocation, 0, 1, 0, 1);
            glUseProgram(0);

            static const glm::mat4 modelMatrixZ = glm::scale(glm::mat4(1), {AxisThickness, AxisThickness, AxisLength});
            static const glm::mat4 modelMatrixY = glm::scale(glm::mat4(1), {AxisThickness, AxisLength, AxisThickness});
            static const glm::mat4 modelMatrixX = glm::scale(glm::mat4(1), {AxisLength, AxisThickness, AxisThickness});
            static glm::mat4 centralGridMatrices[GridCount * 4];
            for (size_t i = 0; i < GridCount; i++)
            {
                // across x
                centralGridMatrices[i * 4] = glm::translate(glm::mat4{1}, {i, 0, 0}) *
                                             glm::scale(glm::mat4{1}, {GridThickness, GridThickness, GridCount});
                // across x (negative)
                centralGridMatrices[i * 4 + 1] = glm::translate(glm::mat4{1}, {-(float)i, 0, 0}) *
                                                 glm::scale(glm::mat4{1}, {GridThickness, GridThickness, GridCount});
                // across z
                centralGridMatrices[i * 4 + 2] = glm::translate(glm::mat4{1}, {0, 0, i}) *
                                                 glm::scale(glm::mat4{1}, {GridCount, GridThickness, GridThickness});
                // across z (negative)
                centralGridMatrices[i * 4 + 3] = glm::translate(glm::mat4{1}, {0, 0, -(float)i}) *
                                                 glm::scale(glm::mat4{1}, {GridCount, GridThickness, GridThickness});
            }

            graphics::opengl::addRendererHook(0, []() {
                auto draw = []() {
                    glDrawElements(GL_TRIANGLES, 6 * 2 * 3, GL_UNSIGNED_INT, 0);
                };
                camera *camera = camera::getMainCamera();
                if (!camera)
                    return;
                glm::vec3 cameraPositon = camera->getTransform()->getWorldPosition();

                auto getDistanceFromCameraAcrossZ = [&cameraPositon](const float z) {
                };
                auto getModelMatrixForLineAcrossX = [&cameraPositon](const float x, const float length) {
                    const auto point = glm::vec3(x, 0, 0);
                    const auto distanceFromCamera = glm::distance(cameraPositon, point);
                    return glm::translate(glm::mat4{1}, {x, 0, 0}) *
                           glm::scale(glm::mat4{1}, {distanceFromCamera * CameraDistanceMultipler * AxisThickness,
                                                     distanceFromCamera * CameraDistanceMultipler * AxisThickness,
                                                     length});
                };
                auto getModelMatrixForLineAcrossZ = [&cameraPositon](const float z, const float length) {
                    const auto point = glm::vec3(0, 0, z);
                    const auto distanceFromCamera = glm::distance(cameraPositon, point);
                    return glm::translate(glm::mat4{1}, {0, 0, z}) *
                           glm::scale(glm::mat4{1}, {length,
                                                     distanceFromCamera * CameraDistanceMultipler * AxisThickness,
                                                     distanceFromCamera * CameraDistanceMultipler * AxisThickness});
                };
                auto getModelMatrixForLineAcrossY = [&cameraPositon](const float y, const float length) {
                    const auto point = glm::vec3(0, y, 0);
                    const auto distanceFromCamera = glm::distance(cameraPositon, point);
                    return glm::translate(glm::mat4{1}, {0, y, 0}) *
                           glm::scale(glm::mat4{1}, {distanceFromCamera * CameraDistanceMultipler * AxisThickness,
                                                     length,
                                                     distanceFromCamera * CameraDistanceMultipler * AxisThickness});
                };

                glUseProgram(program);
                glBindVertexArray(vao);

                // commons
                glUniformMatrix4fv(viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(camera->getViewMatrix()));
                glUniformMatrix4fv(projectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(camera->getProjectionMatrix()));

                // draw Z axis
                glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(getModelMatrixForLineAcrossZ(0, AxisLength)));
                glUniform4f(colorLocation, AxisXColor.r, AxisXColor.g, AxisXColor.b, AxisXColor.a);
                draw();
                // draw Y axis
                glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(getModelMatrixForLineAcrossY(0, AxisLength)));
                glUniform4f(colorLocation, AxisYColor.r, AxisYColor.g, AxisYColor.b, AxisYColor.a);
                draw();
                // draw X axis
                glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(getModelMatrixForLineAcrossX(0, AxisLength)));
                glUniform4f(colorLocation, AxisZColor.r, AxisZColor.g, AxisZColor.b, AxisZColor.a);
                draw();
                // grid
                glUniform4f(colorLocation, GridColor.r, GridColor.g, GridColor.b, GridColor.a);
                for (int i = 1; i < GridCount; i++)
                {
                    // across x
                    glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(getModelMatrixForLineAcrossX(i, GridCount - 1)));
                    draw();
                    glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(getModelMatrixForLineAcrossX(-i, GridCount - 1)));
                    draw();
                    // across z
                    glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(getModelMatrixForLineAcrossZ(i, GridCount - 1)));
                    draw();
                    glUniformMatrix4fv(modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(getModelMatrixForLineAcrossZ(-i, GridCount - 1)));
                    draw();
                }

                glBindVertexArray(0);
                glUseProgram(0);
            });
        }
    }
};
} // namespace engine