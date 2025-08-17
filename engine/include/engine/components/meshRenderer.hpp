#pragma once
#include "../app.hpp"
#include "../window.hpp"

namespace engine
{
struct meshRenderer : public component
{
  private:
    static inline auto &s_instances = *new std::vector<std::weak_ptr<component>>();

    static inline void ensureInitialize_()
    {
        static bool s_initialized;
        if (s_initialized)
            return;
        s_initialized = true;

        // initialize
        if (graphics::getRenderer() == graphics::renderer::opengl)
        {
            graphics::opengl::onRendersMutex.lock();
            graphics::opengl::onRenders[0].push_back([]() {
                // draw
                for (auto &instance : s_instances)
                {
                    auto &ref = *dynamic_cast<meshRenderer *>(instance.lock().get());
                    // render...
                }
                glUseProgram(0);
                glBindVertexArray(0);
            });
            graphics::opengl::onRendersMutex.unlock();
        }
    }

    void created_() override
    {
        ensureInitialize_();
        s_instances.push_back(getWeakPtr());
    }
};
} // namespace engine