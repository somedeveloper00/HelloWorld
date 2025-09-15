#pragma once

#include "GLFW/glfw3.h"
#include "common/componentUtils.hpp"
#include "engine/app.hpp"
#include "engine/components/transform.hpp"
#include "engine/errorHandling.hpp"
#include "engine/quickVector.hpp"
#include "engine/window.hpp"
#include "glm/fwd.hpp"
#include "glm/gtc/type_ptr.hpp"
#include <chrono>
#include <gl/gl.h>
#include <string>
#include <thread>

namespace engine
{
// causes this object to participate in the world-space pointer down recognition (useful for detecting clicks)
struct pointerRead : public component
{
    createTypeInformation(pointerRead, component);

    // gets called when pointer enters this object. executes before components update phase
    quickVector<std::function<void()>> onPointerEnter;

    // gets called when pointer exists this object. executes before components update phase
    quickVector<std::function<void()>> onPointerExit;

  private:
    struct entry
    {
        GLuint vao;
        GLsizei trianglesCount;
        glm::mat4 model;
        pointerRead *instance;
    };

  public:
    // setup a vao and triangles count to be used by this pointerRead
    // vao will not be deleted in this system, and it'll be expected to be available for the lifetime of this component
    void setVertices(const GLuint vao, const GLsizei trianglesCount)
    {
        s_hashes[_id].trianglesCount = trianglesCount;
        s_hashes[_id].vao = vao;
    }

  protected:
    transform *_transform;

    void created_() override
    {
        initialize_();
        disallowMultipleComponents(pointerRead);
        if (s_hashesCount == hashTypeMax)
        {
            log::logError("Removed the new component \"{}\" of entity \"{}\". Cannot have more than {} instances of this component.", getTypeName(), getEntity()->name, hashTypeMax);
            remove();
            return;
        }
        _transform = getEntity()->ensureComponentExists<transform>();
        _transform->pushLock();

        // add to hashes
        _id = s_hashesCount++;
        s_hashes[_id] = entry{.model = _transform->getGlobalMatrix(), .instance = this};
    }

    void removed_() override
    {
        _transform->popLock();

        // remove from hashes
        s_hashes[_id] = std::move(s_hashes[--s_hashesCount]);
        s_hashes[_id].instance->_id = _id;
    }

  private:
    using hashType = unsigned short;
    static inline constexpr hashType hashTypeMax = hashType(-1);
    static inline constexpr hashType invalidHash = hashType(-1);

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
    static inline GLuint s_texture = 0, s_fbo = 0;

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
            uniform mat4 model; 
            uniform uint hash;
            out uint outHash;

            void main()
            {
                gl_Position = model * vec4(position, 1.f);
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
            static const auto s_program = graphics::opengl::createProgram(vertexShader, fragmentShader);
            fatalAssert(s_program, "could not create opengl program for pointerRead component");
            static GLint s_modelLocation = glGetUniformLocation(s_program, "model");
            fatalAssert(s_modelLocation != -1, "could not find \"model\" uniform variable.");
            static GLint s_hashLocation = glGetUniformLocation(s_program, "hash");
            fatalAssert(s_hashLocation != -1, "could not find \"hash\" uniform variable.");

            application::hooksMutex.lock();
            application::preComponentHooks.push_back([]() {
                frameBufferDebug_();
                bench("update screen object hashes");

                // update data
                for (size_t i = 0; i < s_hashesCount; i++)
                    s_hashes[i].model = s_hashes[i].instance->_transform->getGlobalMatrix();

                // render to frame buffer
                graphics::opengl::noBlendContext _; // disable blending (RAII)
                glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
                glClearColor(0, 0, 0, 0);
                glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
                glUseProgram(s_program);
                for (size_t i = 0; i < s_hashesCount; i++)
                    if (s_hashes[i].vao)
                    {
                        glBindVertexArray(s_hashes[i].vao);
                        glUniformMatrix4fv(s_modelLocation, 1, GL_FALSE, glm::value_ptr(s_hashes[i].model));
                        glUniform1ui(s_hashLocation, static_cast<GLuint>(i + 1)); // 0 is clear color
                        glDrawElements(GL_TRIANGLES, s_hashes[i].trianglesCount, GL_UNSIGNED_INT, 0);
                    }
                glUseProgram(0);
                glBindVertexArray(0);

                // readback
                glPixelStorei(GL_PACK_ALIGNMENT, 1); // avoid "row packing" issue
                glReadPixels(0, 0, s_screenHashesSizes.x, s_screenHashesSizes.y, GL_RED_INTEGER, GL_UNSIGNED_SHORT, s_screenHashes);
                glBindFramebuffer(GL_FRAMEBUFFER, 0);
                glPixelStorei(GL_PACK_ALIGNMENT, 4);

                // update objects
                const auto mousePos = input::getMousePosition();
                const auto ind = mousePos.x * s_screenHashesSizes.y + mousePos.y;
                const hashType newPointedId = s_screenHashes[ind] - 1;
                if (newPointedId == s_lastPointedId)
                    return;
                // call onPointerExit
                if (s_lastPointedId != hashTypeMax && s_lastPointedId != invalidHash)
                {
                    const auto callback = s_hashes[s_lastPointedId].instance->onPointerExit;
                    callback.forEach([](const auto &func) { func(); });
                }
                // call onPointerEnter
                if (newPointedId != invalidHash)
                {
                    const auto callback = s_hashes[newPointedId].instance->onPointerEnter;
                    callback.forEach([](const auto &func) { func(); });
                }
                s_lastPointedId = newPointedId;
            });
            application::hooksMutex.unlock();
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
            if (!s_fbo)
                glGenFramebuffers(1, &s_fbo);
            glBindFramebuffer(GL_FRAMEBUFFER, s_fbo);
            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, s_texture, 0);
            GLenum db[] = {GL_COLOR_ATTACHMENT0};
            glDrawBuffers(1, db);

            // depth buffer
            static GLuint s_depth = 0;
            if (!s_depth)
                glGenRenderbuffers(1, &s_depth);
            glBindRenderbuffer(GL_RENDERBUFFER, s_depth);
            glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT24, s_screenHashesSizes.x, s_screenHashesSizes.y);
            glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, s_depth);

            // unbind
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glBindTexture(GL_TEXTURE_2D, 0);
            glBindRenderbuffer(GL_RENDERBUFFER, 0);
        }

        log::logInfo("{} size:{}", s_screenHashesSizes, s_screenHashesSizes.x * s_screenHashesSizes.y);
    }

    static inline void frameBufferDebug_()
    {
        if (input::isKeyJustDown(input::key::k))
        {
            graphics::opengl::debugModeContext _;
            
            constexpr auto vertexShader = R"(
            #version 460 core
            const vec2 verts[3] = vec2[3](
                vec2(-1.0, -1.0),
                vec2( 3.0, -1.0),
                vec2(-1.0,  3.0)
            );
            void main() {
                gl_Position = vec4(verts[gl_VertexID], 0.0, 1.0);
            }
            )";
            constexpr auto fragmentShader = R"(
            #version 460 core
            layout(binding = 0) uniform usampler2D uTex;
            out vec4 FragColor;

            void main() {
                // Convert window coords to integer texel coordinates
                ivec2 texel = ivec2(gl_FragCoord.xy);

                // Fetch the 16-bit unsigned value
                uint val = texelFetch(uTex, texel, 0).r;

                // Normalize to [0,1] for display (65535 = max of uint16)
                float gray = float(val) / 1;//65535.0;

                FragColor = vec4(gray, gray, gray, 1.0);
            }
            )";

            static auto s_program = graphics::opengl::createProgram(vertexShader, fragmentShader);
            fatalAssert(s_program, "could not create opengl program for pointerRead component's debug");
            static auto texLocation = glGetUniformLocation(s_program, "tex");

            static GLuint vao = 0;
            if (vao == 0)
                glGenVertexArrays(1, &vao);

            graphics::opengl::noDepthTestContext __;
            glUseProgram(s_program);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, s_texture);
            glUniform1i(texLocation, 0);

            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);

            glfwSwapBuffers(glfwGetCurrentContext());

            glBindVertexArray(0);
            glUseProgram(0);

            std::this_thread::sleep_for(std::chrono::duration<float>(1));
        }
    }
};
} // namespace engine