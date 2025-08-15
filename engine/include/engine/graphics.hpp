#pragma once

// clang-format off
#include <glad/gl.h>
#include <GLFW/glfw3.h>
// clang-format on

#include "app.hpp"
#include "benchmark.hpp"
#include "data.hpp"
#include "errorHandling.hpp"
#include "log.hpp"
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <winnt.h>

namespace engine
{
struct opengl; // for later

struct graphics final
{
    enum renderer : uint8_t
    {
        opengl
    };

    graphics() = delete;

    static inline color clearColor{1.f, 1.f, 0.f, 1.f};

    static inline void initialize(std::string name, const glm::vec2 center, const glm::vec2 size, bool useMica, bool useAcrylic, renderer renderer)
    {
        bench(__FUNCTION__);
        static bool _initialized = false;
        fatalErrorIfNot(_initialized == false, "window already initialized");
        _initialized = true;
        s_name = name;
        s_renderer = renderer;
        s_size = size;

        // create window
        fatalErrorIfNot(glfwInit() == GLFW_TRUE, "glfwInit() failed");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        s_window = glfwCreateWindow(size.x, size.y, name.c_str(), NULL, NULL);
        fatalErrorIfNot(s_window != nullptr, "glfwCreateWindow failed");
        glfwMakeContextCurrent(s_window);
        // initialize GLAD
        if (s_renderer == renderer::opengl)
            opengl::initialize_();

        application::hooksMutex.lock();
        application::onExitHooks.push_back([]() {
            glfwTerminate();
        });
        application::preComponentHooks.push_back([]() {
            glfwPollEvents();
            if (glfwWindowShouldClose(s_window))
                application::close();
        });
        application::hooksMutex.unlock();
    }

    static inline renderer getRenderer() noexcept
    {
        return s_renderer;
    }

    static inline glm::vec2 getSize() noexcept
    {
        return s_size;
    }

    struct opengl final
    {
        opengl() = delete;
        friend graphics;

        // executes when rendering a frame. if you subscribe, you should also unsubscribe later.
        // order of execution is from 0 to last.
        // feel free to insert new elements.
        static inline auto &onRenders = *new std::vector<std::vector<std::function<void()>>>{{}};
        static inline std::mutex onRendersMutex;

      private:
        static inline void initialize_()
        {
            fatalErrorIfNot(gladLoadGL((GLADloadfunc)glfwGetProcAddress) != 0, "gladLoadGL failed");
            glEnable(GL_DEPTH_TEST);
            glViewport(0, 0, s_size.x, s_size.y);
            glfwSetWindowSizeCallback(s_window, [](GLFWwindow *window, GLsizei width, GLsizei height) {
                glViewport(0, 0, width, height);
                s_size.x = width;
                s_size.y = height;
                tick_();
            });
            application::hooksMutex.lock();
            application::postComponentHooks.push_back(tick_);
            application::hooksMutex.unlock();
        }

        static inline void updateWindowSize_(glm::vec2 size)
        {
            glViewport(0, 0, size.x, size.y);
        }

        static inline void tick_()
        {
            glClearColor(graphics::clearColor.r, graphics::clearColor.g, graphics::clearColor.b, graphics::clearColor.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // execute a copy of onRenders
            onRendersMutex.lock();
            auto onRendersCopy = onRenders;
            onRendersMutex.unlock();
            for (auto &row : onRendersCopy)
                for (auto &render : row)
                    render();
            glfwSwapBuffers(s_window);
        }
    };

  private:
    static inline renderer s_renderer;
    static inline glm::vec2 s_size;
    static inline GLFWwindow *s_window;
    static inline std::string s_name;
};
} // namespace engine