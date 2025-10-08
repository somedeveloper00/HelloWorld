#pragma once

#include "engine/app.hpp"
#include "engine/components/camera.hpp"
#include "engine/components/transform.hpp"
#include "engine/window.hpp"
#include "glm/fwd.hpp"
#include <intrin.h>

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
        GLuint vao = 0;
        GLsizei verticesCount = 0;
        glm::mat4 model{};
        pointerRead *instance = nullptr;

        constexpr entry()
            : vao(0), verticesCount(0), model(), instance(nullptr)
        {
        }
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
            s_hashes[_id].verticesCount = verticesCount;
            s_hashes[_id].vao = vao;
        }
    }

  protected:
    transform *_transform;
    GLuint _vao;
    GLsizei _verticesCount;

    bool created_() override
    {
        initialize_();
        disallowMultipleComponents(pointerRead);
        if (s_hashesCount == hashTypeMax)
        {
            log::logError("Removed the new component \"{}\" of entity \"{}\". Cannot have more than {} instances of this component.", getTypeName(), getEntity()->name, hashTypeMax);
            return false;
        }
        if (!(_transform = getEntity()->ensureComponentExists<transform>()))
            return false;
        _transform->pushLock();
        return true;
    }

    void removed_() override
    {
        _transform->popLock();
    }

    void enabled_() override
    {
        // add to hashes
        _id = s_hashesCount++;
        s_hashes[_id].model = _transform->getGlobalMatrix();
        s_hashes[_id].instance = this;
        s_hashes[_id].vao = _vao;
        s_hashes[_id].verticesCount = _verticesCount;
    }

    void disabled_() override
    {
        // remove from hashes
        s_hashes[_id] = std::move(s_hashes[--s_hashesCount]);
        if (_id != s_hashesCount)
            s_hashes[_id].instance->_id = _id;
    }

  private:
    using hashType = unsigned short;

    // maximum number of hashes, also used as an invalid state for hashes
    static inline constexpr hashType hashTypeMax = hashType(-1);

    // entries for all hashes currently occupied
    // also works as a hash id itself, starting from 1 (because 0 is clear-color)
    static inline entry s_hashes[hashTypeMax];

    // count of hashes currently occupied
    static inline hashType s_hashesCount = 0;

    // hashes of the last frame's screen. rows first. hashes are +1 of the actual hashes, because 0 is clear color
    static inline hashType *s_screenHashes = nullptr;

    // x: outter size (columns) y: inner size (rows)
    static inline glm::ivec2 s_screenHashesSizes;

    // for hashes
    static inline GLuint s_texture = 0, s_frameBuffer = 0;

    // id of the last object under the pointer (in the last frame). starts from 0
    static inline hashType s_lastPointedId = hashTypeMax;

    // index of this instance in the s_screenHashes
    hashType _id;

    static inline void initialize_()
    {
        ensureExecutesOnce();
        updateScreenHashesBuffer_();
        graphics::frameBufferSizeChanged.push_back(updateScreenHashesBuffer_);
        if (graphics::getRenderer() == graphics::renderer::opengl)
        {
            const auto vertexShader = R"(
            #version 460 core
            layout(location = 0) in vec3 position;
            uniform mat4 modelMatrix; 
            uniform mat4 viewMatrix; 
            uniform mat4 projectionMatrix; 
            uniform uint hash;
            out uint outHash;

            void main()
            {
                gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(position, 1.f);
                outHash = hash;
            }
            )";
            const auto fragmentShader = R"(
            #version 460 core
            in flat uint outHash;
            layout(location = 0) out uint result;

            void main()
            {
                result = outHash;
            }
            )";

            // shader
            static const auto s_program = graphics::opengl::fatalCreateProgram("pointerRead", vertexShader, fragmentShader);
            static GLint s_modelMatrixLocation = graphics::opengl::fatalGetLocation(s_program, "modelMatrix");
            static GLint s_viewMatrixLocation = graphics::opengl::fatalGetLocation(s_program, "viewMatrix");
            static GLint s_projectionMatrixLocation = graphics::opengl::fatalGetLocation(s_program, "projectionMatrix");
            static GLint s_hashLocation = graphics::opengl::fatalGetLocation(s_program, "hash");

            application::preComponentHooks.push_back([]() {
                bench("update screen object hashes");
                if (input::isMouseInWindow())
                {
                    // update data
                    for (size_t i = 0; i < s_hashesCount; i++)
                        s_hashes[i].model = s_hashes[i].instance->_transform->getGlobalMatrix();

                    camera *camera = camera::getMainCamera();
                    if (!camera)
                        return;
                    // render to frame buffer
                    graphics::opengl::noBlendContext _; // disable blending (RAII)
                    glBindFramebuffer(GL_FRAMEBUFFER, s_frameBuffer);
                    glClearColor(0, 0, 0, 0);
                    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                    glUseProgram(s_program);
                    glUniformMatrix4fv(s_viewMatrixLocation, 1, GL_FALSE, glm::value_ptr(camera->getViewMatrix()));
                    glUniformMatrix4fv(s_projectionMatrixLocation, 1, GL_FALSE, glm::value_ptr(camera->getProjectionMatrix()));
                    for (size_t i = 0; i < s_hashesCount; i++)
                        if (s_hashes[i].vao)
                        {
                            glBindVertexArray(s_hashes[i].vao);
                            glUniformMatrix4fv(s_modelMatrixLocation, 1, GL_FALSE, glm::value_ptr(s_hashes[i].model));
                            glUniform1ui(s_hashLocation, static_cast<GLuint>(i + 1)); // 0 is clear color
                            glDrawElements(GL_TRIANGLES, s_hashes[i].verticesCount, GL_UNSIGNED_INT, 0);
                        }

                    // readback
                    glReadBuffer(GL_COLOR_ATTACHMENT0); // read s_texture
                    glReadPixels(0, 0, s_screenHashesSizes.x, s_screenHashesSizes.y, GL_RED_INTEGER, GL_UNSIGNED_SHORT, s_screenHashes);

                    glBindFramebuffer(GL_FRAMEBUFFER, 0);
                    glUseProgram(0);
                    glBindVertexArray(0);

                    // update objects
                    auto mousePos = input::getMousePosition();
                    fatalAssert(mousePos.x >= 0 && mousePos.y >= 0 && mousePos.x < graphics::getFrameBufferSize().x && mousePos.y < graphics::getFrameBufferSize().y, "mouse position outside of window");
                    const auto ind =
                        (graphics::getFrameBufferSize().y - mousePos.y - 1) // -1 to avoid false readPixels on the top-most row pixels
                            * s_screenHashesSizes.x +
                        mousePos.x;
                    const hashType newPointedId = s_screenHashes[ind] - static_cast<hashType>(1); // -1 because 0 is clear color
                    const hashType newPointdRaw = s_screenHashes[ind];
                    if (newPointedId == s_lastPointedId)
                        return;
                    if (newPointdRaw > s_hashesCount)
                    {
                        drawDebugFrameBuffer();
                        fatalAssert(false, ("invalid hash read: " + std::to_string(newPointedId)).c_str());
                    }
                    // call onPointerExit
                    if (s_lastPointedId != hashTypeMax)
                    {
                        const auto callback = s_hashes[s_lastPointedId].instance->onPointerExit;
                        callback.forEach([](const auto &func) { func(); });
                    }
                    // call onPointerEnter
                    if (newPointedId != hashTypeMax)
                    {
                        const auto callback = s_hashes[newPointedId].instance->onPointerEnter;
                        callback.forEach([](const auto &func) { func(); });
                    }
                    s_lastPointedId = newPointedId;
                }
                else // mouse not in window
                {
                    // call onPointerExit
                    if (s_lastPointedId != hashTypeMax)
                    {
                        const auto callback = s_hashes[s_lastPointedId].instance->onPointerExit;
                        callback.forEach([](const auto &func) { func(); });
                    }
                    s_lastPointedId = hashTypeMax;
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
    static inline void updateScreenHashesBuffer_()
    {
        s_screenHashesSizes = graphics::getFrameBufferSize();

        // create new texture readback buffer
        free(s_screenHashes); // it's ok to free nullptr
        s_screenHashes = static_cast<hashType *>(malloc(s_screenHashesSizes.x * s_screenHashesSizes.y * sizeof(hashType)));

        if (graphics::getRenderer() == graphics::renderer::opengl)
        {
            // texture
            if (s_texture)
                glDeleteTextures(1, &s_texture);
            glGenTextures(1, &s_texture);
            glBindTexture(GL_TEXTURE_2D, s_texture);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_R16UI /* because of unsigned short */,
                         static_cast<GLsizei>(s_screenHashesSizes.x), static_cast<GLsizei>(s_screenHashesSizes.y),
                         0, GL_RED_INTEGER, GL_UNSIGNED_SHORT, nullptr);
            // no filtering
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

            // frame buffer
            if (!s_frameBuffer)
                glGenFramebuffers(1, &s_frameBuffer);
            glBindFramebuffer(GL_FRAMEBUFFER, s_frameBuffer);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_texture, 0);

            // depth buffer
            static GLuint s_depth = 0;
            if (!s_depth)
                glGenRenderbuffers(1, &s_depth);
            glBindRenderbuffer(GL_RENDERBUFFER, s_depth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, s_screenHashesSizes.x, s_screenHashesSizes.y);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_depth);

            fatalAssert(glCheckFramebufferStatus(GL_FRAMEBUFFER) == GL_FRAMEBUFFER_COMPLETE, "could not create framebuffer for pointerRead component");

            // unbind
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }

        log::logInfo("{} size:{}", s_screenHashesSizes, s_screenHashesSizes.x * s_screenHashesSizes.y);
    }

    static inline void drawDebugFrameBuffer()
    {
        graphics::opengl::displayFrameBufferAsHashFullScreen(s_frameBuffer, 1);
    }
};

#ifndef GAMEENGINE_POINTERREAD_H
#define GAMEENGINE_POINTERREAD_H
#endif

} // namespace engine