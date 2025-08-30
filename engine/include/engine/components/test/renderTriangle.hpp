#pragma once

#include "engine/app.hpp"
#include "engine/benchmark.hpp"
#include "engine/components/componentUtility.hpp"
#include "engine/components/transform.hpp"
#include "engine/ref.hpp"
#include "engine/window.hpp"
#include "glm/ext/quaternion_transform.hpp"
#include <glm/gtc/type_ptr.hpp>
#include <mutex>

namespace engine::test
{
struct renderTriangle : public component
{
    friend entity;
    float swaySpeed = 1;

  private:
    static inline auto &s_instances = *new quickVector<weakRef<renderTriangle>>();
    float _startTime;
    weakRef<transform> _transform;

    static inline void initialize_()
    {
        executeOnce();
        // initialize
        if (graphics::getRenderer() == graphics::renderer::opengl)
        {
            // create shader
            auto vertSrc = R"(
            #version 460 core
            layout(location = 0) in vec3 position;
            layout(location = 1) in vec3 globalPosition;
            layout(location = 2) in vec4 globalRotation;
            layout(location = 3) in vec3 globalScale;
            out vec3 fragPosition;

            mat4 makeTranslation(vec3 t) {
                mat4 m = mat4(1.0);
                m[3].xyz = t; // fourth column is translation
                return m;
            }

            mat4 makeScale(vec3 s) {
                mat4 m = mat4(1.0);
                m[0][0] = s.x;
                m[1][1] = s.y;
                m[2][2] = s.z;
                return m;
            }

            mat4 makeRotationQuat(vec4 q) {
                // Ensure quaternion is normalized
                vec4 nq = normalize(q);
                float x = nq.x, y = nq.y, z = nq.z, w = nq.w;

                float xx = x * x;
                float yy = y * y;
                float zz = z * z;
                float xy = x * y;
                float xz = x * z;
                float yz = y * z;
                float wx = w * x;
                float wy = w * y;
                float wz = w * z;

                return mat4(
                    1.0 - 2.0 * (yy + zz), 2.0 * (xy + wz),       2.0 * (xz - wy),       0.0,
                    2.0 * (xy - wz),       1.0 - 2.0 * (xx + zz), 2.0 * (yz + wx),       0.0,
                    2.0 * (xz + wy),       2.0 * (yz - wx),       1.0 - 2.0 * (xx + yy), 0.0,
                    0.0,                   0.0,                   0.0,                   1.0
                );
            }
            
            void main()
            {
                gl_Position = makeTranslation(globalPosition) * makeRotationQuat(globalRotation) * makeScale(globalScale) * vec4(position, 1.f);
                fragPosition = position;
            }
            )";

            auto fragSrc = R"(
            #version 460 core
            in vec3 fragPosition;
            out vec4 color;
            void main()
            {
                color = vec4(fragPosition + 0.5, 1.0);
            }
            )";

            struct InstanceData
            {
                glm::vec3 position;
                glm::quat rotation;
                glm::vec3 scale;
            };

            // compile and link shaders
            static auto s_shaderProgram = graphics::opengl::createProgram(vertSrc, fragSrc);
            if (s_shaderProgram == GL_FALSE)
            {
                application::close();
                return;
            }

            static auto s_matrixLocation = glGetUniformLocation(s_shaderProgram, "matrix");

            // set up triangle vertices
            GLfloat vertices[] = {
                -0.5f, -0.5f, 0.0f, // bottom left
                0.5f, -0.5f, 0.0f,  // bottom right
                0.0f, 0.5f, 0.0f    // top
            };
            // create _vao and VBO
            static GLuint s_vao;
            glGenVertexArrays(1, &s_vao);
            glBindVertexArray(s_vao);
            GLuint VBO;
            glGenBuffers(1, &VBO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
            glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid *)0);
            glEnableVertexAttribArray(0);
            glBindBuffer(GL_ARRAY_BUFFER, 0);

            // instance buffer
            static GLuint s_instancedVbo;
            glGenBuffers(1, &s_instancedVbo);
            glBindBuffer(GL_ARRAY_BUFFER, s_instancedVbo);

            glEnableVertexAttribArray(1);
            glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)(offsetof(InstanceData, position)));
            glVertexAttribDivisor(1, 1);
            glEnableVertexAttribArray(2);
            glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)(offsetof(InstanceData, rotation)));
            glVertexAttribDivisor(2, 1);
            glEnableVertexAttribArray(3);
            glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(InstanceData), (void *)(offsetof(InstanceData, scale)));
            glVertexAttribDivisor(3, 1);

            glBindVertexArray(0);

            static std::vector<InstanceData> s_instanceDataList{};

            application::hooksMutex.lock();
            application::postComponentHooks.push_back([]() {
                bench("update triangles");
                auto *ptr = s_instances.data();
                auto count = s_instances.size();
                auto time = time::getTotalTime();
                auto deltaTime = time::getDeltaTime();
#pragma omp parallel for schedule(static)
                for (signed long long i = 0; i < count; i++)
                {
                    renderTriangle &instance = *(renderTriangle *)ptr[i];
                    transform &transformRef = *(transform *)instance._transform;
                    transformRef.position.x = glm::sin((time - instance._startTime) * instance.swaySpeed);
                    transformRef.rotation = glm::rotate(transformRef.rotation, deltaTime, glm::vec3(0.f, 0.f, 1.f));
                    transformRef.markDirtyRecursively();
                }
            });
            application::hooksMutex.unlock();

            graphics::opengl::onRendersMutex.lock();
            graphics::opengl::onRenders[0].push_back([]() {
                bench("drawing render triangles");
                // Upload matrices to GPU
                s_instanceDataList.clear();
                s_instanceDataList.resize(s_instances.size());
                auto *dataPtr = s_instanceDataList.data();
                auto size = s_instanceDataList.size();
                auto *instancesPtr = s_instances.data();
#pragma omp parallel for schedule(static)
                for (signed long long i = 0; i < size; i++)
                {
                    transform &transformRef = *(transform *)instancesPtr[i]->_transform;
                    dataPtr[i].position = transformRef.position;
                    dataPtr[i].rotation = transformRef.rotation;
                    dataPtr[i].scale = transformRef.scale;
                }
                glBindBuffer(GL_ARRAY_BUFFER, s_instancedVbo);
                glBufferData(GL_ARRAY_BUFFER, s_instanceDataList.size() * sizeof(InstanceData), s_instanceDataList.data(), GL_DYNAMIC_DRAW);
                glUseProgram(s_shaderProgram);

                // draw
                glBindVertexArray(s_vao);
                glDrawArraysInstanced(GL_TRIANGLES, 0, 3, static_cast<int>(s_instances.size()));
                glUseProgram(0);
                glBindVertexArray(0);
            });
            graphics::opengl::onRendersMutex.unlock();
        }
    }

    void created_() override
    {
        initialize_();
        s_instances.push_back(getWeakRefAs<renderTriangle>());
        _startTime = time::getTotalTime();
        _transform = getEntity()->ensureComponentExists<transform>();
    }

    void removed_() override
    {
        s_instances.erase(getWeakRefAs<renderTriangle>());
    }
};
} // namespace engine::test