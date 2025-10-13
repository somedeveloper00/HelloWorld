#pragma once

#include "engine/app.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/transform.hpp"
#include "engine/quickVector.hpp"
#include "engine/window.hpp"
#include "glm/fwd.hpp"
#include <intrin.h>
#include <limits>

namespace engine
{
// causes this object to participate in the world-space pointer down recognition (useful for detecting clicks)
// disabling this component will make it non-effective
struct pointerRead : public component
{
    createTypeInformation(pointerRead, component);

    // gets called when pointer enters this object. executes before components update phase, and after onPointerExit of the previous object.
    quickVector<std::function<void()>> onPointerEnter;

    // gets called when pointer exists this object. executes before components update phase, and before onPointerEnter of the next object.
    quickVector<std::function<void()>> onPointerExit;

  private:
    struct entry
    {
        GLuint vao;
        GLsizei verticesCount;
        glm::mat4 modelMatrix;
        pointerRead *instance;
    };

  public:
    // setup a vao and triangles count to be used by this pointerRead
    // vao will not be deleted in this system, and it'll be expected to be available for the lifetime of this component
    // verticesCount refers to the number of vertices
    void setVertices(const GLuint vao, const GLsizei verticesCount)
    {
        _verticesCount = verticesCount;
        _vao = vao;
        // apply immediately
        if (getEnabled())
        {
            s_id2Object[_id - 1].verticesCount = verticesCount;
            s_id2Object[_id - 1].vao = vao;
        }
    }

  protected:
    transform *_transform;
    GLuint _vao;
    GLsizei _verticesCount;

    bool created_() override
    {
        disallowMultipleComponents(pointerRead);
        if (s_id2Object.size() == maxId)
        {
            log::logError("cannot create more {} components because the maximum number ({}) has reached.", getTypeName(), maxId);
            return false;
        }
        if (!(_transform = getEntity()->ensureComponentExists<transform>()))
            return false;
        initialize_();
        _transform->pushLock();
        return true;
    }

    void removed_() override
    {
        _transform->popLock();
    }

    void enabled_() override
    {
        // add to map
        s_id2Object.push_back({.vao = _vao,
                               .verticesCount = _verticesCount,
                               .instance = this});
        _id = s_id2Object.size(); // we want to start from 1, since 0 is invalid
    }

    void disabled_() override
    {
        // swap with the last and remove last
        s_id2Object[_id - 1] = s_id2Object.pop_back_get();
        s_id2Object[_id - 1].instance->_id = _id;
    }

  private:
    // ID of objects
    using idType = unsigned int;

    // maximum number of IDs
    static inline constexpr idType maxId = std::numeric_limits<idType>::max();

    // shortcut for invalid ID
    static inline constexpr idType invalidId = static_cast<idType>(0);

    // entries for all IDs currently occupied. index is `ID-1` of the object it maps to
    static inline quickVector<entry> s_id2Object;

    // IDs of the last frame's screen. rows first
    static inline quickVector<idType> s_screenIds;

    static inline GLuint s_screenIdMapTexture = 0, s_screenIdMapFrameBuffer = 0, s_screenIdMapDepthBuffer = 0;

    // ID of the last object under the pointer (in the last frame). can be invalid
    static inline idType s_lastPointedId = invalidId;

    // ID and index of this instance in the s_id2Object
    idType _id;

    static inline void initialize_()
    {
        ensureExecutesOnce();
        updateScreenIdsBuffer_();
        graphics::frameBufferSizeChanged.push_back(updateScreenIdsBuffer_);
        if (graphics::getRenderer() == graphics::renderer::opengl)
        {
            const auto vertexShader = R"(
            #version 460 core
            layout(location = 0) in vec3 position;
            uniform mat4 modelMatrix;
            uniform mat4 viewMatrix;
            uniform mat4 projectionMatrix;
            
            void main()
            {
                gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.f);
            }
            )";
            const auto fragmentShader = R"(
            #version 460 core
            uniform uint id;
            out uint FragColor;

            void main()
            {
                FragColor = id;
            }
            )";

            // shader
            static const auto s_program = graphics::opengl::fatalCreateProgram("pointerRead", vertexShader, fragmentShader);
            static GLint s_modelMatrixLocation = graphics::opengl::fatalGetLocation(s_program, "modelMatrix");
            static GLint s_viewMatrixLocation = graphics::opengl::fatalGetLocation(s_program, "viewMatrix");
            static GLint s_projectionMatrixLocation = graphics::opengl::fatalGetLocation(s_program, "projectionMatrix");
            static GLint s_idLocation = graphics::opengl::fatalGetLocation(s_program, "id");

            application::preComponentHooks.push_back([]() {
                bench("update screen object ids");
                if (input::isMouseInWindow())
                {
                    camera *camera = camera::getMainCamera();
                    if (!camera)
                        return;

                    // update model matrices
                    s_id2Object.forEachParallel([](entry &entry) {
                        entry.modelMatrix = entry.instance->_transform->getGlobalMatrix();
                    });

                    // render to frame buffer
                    graphics::opengl::noBlendContext _; // disable blending (RAII)
                    glBindFramebuffer(GL_FRAMEBUFFER, s_screenIdMapFrameBuffer);
                    glClearColor(0, 0, 0, 0);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glUseProgram(s_program);
                    glUniformMatrix4fv(s_viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(camera->getViewMatrix()));
                    glUniformMatrix4fv(s_projectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(camera->getProjectionMatrix()));
                    s_id2Object.forEachIndexed([](const size_t index, const entry &entry) {
                        if (entry.vao)
                        {
                            glBindVertexArray(entry.vao);
                            glUniformMatrix4fv(s_modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(entry.modelMatrix));
                            glUniform1ui(s_idLocation, index + 1); // 0 is clear color
                            glDrawElements(GL_TRIANGLES, entry.verticesCount, GL_UNSIGNED_INT, 0);
                        }
                    });

                    // read back
                    const auto &frameBufferSize = graphics::getFrameBufferSize();
                    glReadBuffer(GL_COLOR_ATTACHMENT0);
                    glReadPixels(0, 0, frameBufferSize.x, frameBufferSize.y, GL_RED_INTEGER, GL_UNSIGNED_INT, s_screenIds.data());

                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glBindVertexArray(0);
                    glUseProgram(0);

                    // update objects
                    auto mousePos = input::getMousePosition() + (graphics::getFrameBufferSize() / 2); // convert to opengl conventions
                    fatalAssert(mousePos.x >= 0 && mousePos.y >= 0 && mousePos.x < graphics::getFrameBufferSize().x && mousePos.y < graphics::getFrameBufferSize().y, "mouse position outside of window");
                    const auto ind = mousePos.y * frameBufferSize.x + mousePos.x;
                    const idType newPointedId = s_screenIds[ind];
                    if (newPointedId == s_lastPointedId)
                        return;
                    if (newPointedId > s_id2Object.size())
                    {
                        drawDebugFrameBuffer();
                        fatalAssert(false, ("invalid id read: " + std::to_string(newPointedId)).c_str());
                    }
                    // call onPointerExit
                    if (s_lastPointedId != invalidId)
                    {
                        const auto callback = s_id2Object[s_lastPointedId - 1].instance->onPointerExit;
                        callback.forEach([](const auto &func) { func(); });
                    }
                    // call onPointerEnter
                    if (newPointedId != invalidId)
                    {
                        const auto callback = s_id2Object[newPointedId - 1].instance->onPointerEnter;
                        callback.forEach([](const auto &func) { func(); });
                    }
                    s_lastPointedId = newPointedId;
                }
                else // mouse not in window
                {
                    // call onPointerExit
                    if (s_lastPointedId != invalidId)
                    {
                        const auto callback = s_id2Object[s_lastPointedId - 1].instance->onPointerExit;
                        callback.forEach([](const auto &func) { func(); });
                    }
                    s_lastPointedId = invalidId;
                }
            });

#if DEBUG
            application::postComponentHooks.push_back([]() {
                if (input::isKeyJustDown(input::key::k))
                    drawDebugFrameBuffer();
            });
#endif
        }
    }

    // deletes/updates buffers (both for CPU-side and GPU-side)
    static inline void updateScreenIdsBuffer_()
    {
        const auto bufferSize = graphics::getFrameBufferSize();

        s_screenIds.reserve(bufferSize.x * bufferSize.y);
        s_screenIds.setSize(s_screenIds.getCapacity());

        if (graphics::getRenderer() == graphics::renderer::opengl)
        {
            // texture
            if (s_screenIdMapTexture)
                glDeleteTextures(1, &s_screenIdMapTexture);
            glGenTextures(1, &s_screenIdMapTexture);
            glBindTexture(GL_TEXTURE_2D, s_screenIdMapTexture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R32UI /* because of unsigned int */,
                         bufferSize.x, bufferSize.y,
                         0, GL_RED_INTEGER, GL_UNSIGNED_INT, nullptr);
            // no filtering
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            // frame buffer
            if (!s_screenIdMapFrameBuffer)
                glGenFramebuffers(1, &s_screenIdMapFrameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, s_screenIdMapFrameBuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_screenIdMapTexture, 0);

            // depth buffer
            if (s_screenIdMapDepthBuffer)
                glDeleteBuffers(1, &s_screenIdMapDepthBuffer);
            glGenRenderbuffers(1, &s_screenIdMapDepthBuffer);
            glBindRenderbuffer(GL_RENDERBUFFER, s_screenIdMapDepthBuffer);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, bufferSize.x, bufferSize.y);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_screenIdMapDepthBuffer);

            fatalAssert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "could not create framebuffer for pointerRead component");

            // unbind
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }
    }

    static inline void drawDebugFrameBuffer()
    {
        graphics::opengl::displayFrameBufferAsHashFullScreen(s_screenIdMapFrameBuffer, 1);
    }
};

#ifndef GAMEENGINE_POINTERREAD_H
#define GAMEENGINE_POINTERREAD_H
#endif

} // namespace engine