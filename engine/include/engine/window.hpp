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

namespace engine
{

// for later
struct opengl;
struct graphics;

struct input final
{
    friend graphics;
    input() = delete;

    // clang-format off
    enum class key : uint8_t
    {
        unknown = 0,

        key1, key2, key3, key4, key5, key6, key7, key8, key9, key0, 
        a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, 
        num0, num1, num2, num3, num4, num5, num6, num7, num8, num9, 
        f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, 
        escape, space, enter, tab, backspace, leftShift, rightShift, leftControl, rightControl, leftAlt, rightAlt, del, left, right, up, down, 

        gamePadA, gamePadB, gamePadX, gamePadY, gamePadRightShoulder, gamePadLeftShoulder, gamePadLeftTrigger, gamePadRightTrigger, gamePadDpadUp, gamePadDpadDown, gamePadDpadLeft, gamePadDpadRight, gamePadMenu, gamePadView, gamePadLeftThumbstickUp, gamePadLeftThumbstickDown, gamePadLeftThumbstickRight, gamePadLeftThumbstickLeft, gamePadRightThumbstickUp, gamePadRightThumbstickDown, gamePadRightThumbstickRight, gamePadRightThumbstickLeft, 

        count
    };
    // clang-format on

    enum class state : uint8_t
    {
        up = 0,
        justDown = 1,
        heldDown = 1 << 1
    };

    friend inline state operator|(state, state) noexcept;
    friend inline state operator&(state, state) noexcept;

    static inline state getKeyState(const key key)
    {
        return s_states[static_cast<size_t>(key)];
    }

    static inline bool isKeyHeldDown(const key key)
    {
        return static_cast<bool>(getKeyState(key) & state::heldDown);
    }

    static inline bool isKeyJustDown(const key key)
    {
        return static_cast<bool>(getKeyState(key) & state::justDown);
    }

    static inline bool isKeyUp(const key key)
    {
        return getKeyState(key) == state::up;
    }

  private:
    static inline state s_states[static_cast<size_t>(key::count)];

    static inline void initialize_(GLFWwindow *window)
    {
        static auto &s_downKeys = *new std::vector<size_t>();
        static auto &s_upKeys = *new std::vector<size_t>();
        static auto &s_repeatKeys = *new std::vector<size_t>();

        glfwSetKeyCallback(window, [](GLFWwindow *window, int glfwKey, int scancode, int action, int mods) {
            auto key = static_cast<size_t>(glfwKeyToEngineKey_(glfwKey));
            if (key <= static_cast<size_t>(key::unknown) || key >= static_cast<size_t>(key::count))
                return;

            // mark for update in the next tick
            if (action == GLFW_PRESS)
                s_downKeys.push_back(key);
            else if (action == GLFW_REPEAT)
            {
                s_downKeys.erase(std::remove(s_downKeys.begin(), s_downKeys.end(), key), s_downKeys.end());
                s_repeatKeys.push_back(key);
            }
            else
            {
                s_downKeys.erase(std::remove(s_downKeys.begin(), s_downKeys.end(), key), s_downKeys.end());
                s_upKeys.push_back(key);
            }
        });
        application::hooksMutex.lock();
        application::preComponentHooks.push_back([]() {
            for (auto key : s_downKeys)
                if (s_states[key] == state::up)
                    s_states[key] = state::justDown;
                else if (s_states[key] == state::justDown)
                    s_states[key] = state::heldDown;
            for (auto key : s_repeatKeys) // we want clicks to occur repeatedly here
                s_states[key] = state::justDown | state::heldDown;
            s_repeatKeys.clear();
            for (auto key : s_upKeys)
                s_states[key] = state::up;
            s_upKeys.clear();
        });
        application::hooksMutex.unlock();
    }

    static inline int engineKeyToGlfwKey_(const key key)
    {
        switch (key)
        {
        case key::key0:
            return GLFW_KEY_0;
        case key::key1:
            return GLFW_KEY_1;
        case key::key2:
            return GLFW_KEY_2;
        case key::key3:
            return GLFW_KEY_3;
        case key::key4:
            return GLFW_KEY_4;
        case key::key5:
            return GLFW_KEY_5;
        case key::key6:
            return GLFW_KEY_6;
        case key::key7:
            return GLFW_KEY_7;
        case key::key8:
            return GLFW_KEY_8;
        case key::key9:
            return GLFW_KEY_9;

        case key::a:
            return GLFW_KEY_A;
        case key::b:
            return GLFW_KEY_B;
        case key::c:
            return GLFW_KEY_C;
        case key::d:
            return GLFW_KEY_D;
        case key::e:
            return GLFW_KEY_E;
        case key::f:
            return GLFW_KEY_F;
        case key::g:
            return GLFW_KEY_G;
        case key::h:
            return GLFW_KEY_H;
        case key::i:
            return GLFW_KEY_I;
        case key::j:
            return GLFW_KEY_J;
        case key::k:
            return GLFW_KEY_K;
        case key::l:
            return GLFW_KEY_L;
        case key::m:
            return GLFW_KEY_M;
        case key::n:
            return GLFW_KEY_N;
        case key::o:
            return GLFW_KEY_O;
        case key::p:
            return GLFW_KEY_P;
        case key::q:
            return GLFW_KEY_Q;
        case key::r:
            return GLFW_KEY_R;
        case key::s:
            return GLFW_KEY_S;
        case key::t:
            return GLFW_KEY_T;
        case key::u:
            return GLFW_KEY_U;
        case key::v:
            return GLFW_KEY_V;
        case key::w:
            return GLFW_KEY_W;
        case key::x:
            return GLFW_KEY_X;
        case key::y:
            return GLFW_KEY_Y;
        case key::z:
            return GLFW_KEY_Z;

        case key::num0:
            return GLFW_KEY_KP_0;
        case key::num1:
            return GLFW_KEY_KP_1;
        case key::num2:
            return GLFW_KEY_KP_2;
        case key::num3:
            return GLFW_KEY_KP_3;
        case key::num4:
            return GLFW_KEY_KP_4;
        case key::num5:
            return GLFW_KEY_KP_5;
        case key::num6:
            return GLFW_KEY_KP_6;
        case key::num7:
            return GLFW_KEY_KP_7;
        case key::num8:
            return GLFW_KEY_KP_8;
        case key::num9:
            return GLFW_KEY_KP_9;

        case key::f1:
            return GLFW_KEY_F1;
        case key::f2:
            return GLFW_KEY_F2;
        case key::f3:
            return GLFW_KEY_F3;
        case key::f4:
            return GLFW_KEY_F4;
        case key::f5:
            return GLFW_KEY_F5;
        case key::f6:
            return GLFW_KEY_F6;
        case key::f7:
            return GLFW_KEY_F7;
        case key::f8:
            return GLFW_KEY_F8;
        case key::f9:
            return GLFW_KEY_F9;
        case key::f10:
            return GLFW_KEY_F10;
        case key::f11:
            return GLFW_KEY_F11;
        case key::f12:
            return GLFW_KEY_F12;
        case key::f13:
            return GLFW_KEY_F13;
        case key::f14:
            return GLFW_KEY_F14;
        case key::f15:
            return GLFW_KEY_F15;
        case key::f16:
            return GLFW_KEY_F16;
        case key::f17:
            return GLFW_KEY_F17;
        case key::f18:
            return GLFW_KEY_F18;
        case key::f19:
            return GLFW_KEY_F19;
        case key::f20:
            return GLFW_KEY_F20;
        case key::f21:
            return GLFW_KEY_F21;
        case key::f22:
            return GLFW_KEY_F22;
        case key::f23:
            return GLFW_KEY_F23;
        case key::f24:
            return GLFW_KEY_F24;

        case key::escape:
            return GLFW_KEY_ESCAPE;
        case key::space:
            return GLFW_KEY_SPACE;
        case key::enter:
            return GLFW_KEY_ENTER;
        case key::tab:
            return GLFW_KEY_TAB;
        case key::backspace:
            return GLFW_KEY_BACKSPACE;
        case key::leftShift:
            return GLFW_KEY_LEFT_SHIFT;
        case key::rightShift:
            return GLFW_KEY_RIGHT_SHIFT;
        case key::leftControl:
            return GLFW_KEY_LEFT_CONTROL;
        case key::rightControl:
            return GLFW_KEY_RIGHT_CONTROL;
        case key::leftAlt:
            return GLFW_KEY_LEFT_ALT;
        case key::rightAlt:
            return GLFW_KEY_RIGHT_ALT;
        case key::del:
            return GLFW_KEY_DELETE;

        case key::left:
            return GLFW_KEY_LEFT;
        case key::right:
            return GLFW_KEY_RIGHT;
        case key::up:
            return GLFW_KEY_UP;
        case key::down:
            return GLFW_KEY_DOWN;

        case key::gamePadA:
            return GLFW_GAMEPAD_BUTTON_A;
        case key::gamePadB:
            return GLFW_GAMEPAD_BUTTON_B;
        case key::gamePadX:
            return GLFW_GAMEPAD_BUTTON_X;
        case key::gamePadY:
            return GLFW_GAMEPAD_BUTTON_Y;
        case key::gamePadRightShoulder:
            return GLFW_GAMEPAD_BUTTON_RIGHT_THUMB;
        case key::gamePadLeftShoulder:
            return GLFW_GAMEPAD_BUTTON_LEFT_THUMB;
        case key::gamePadLeftTrigger:
            return GLFW_GAMEPAD_BUTTON_LEFT_BUMPER;
        case key::gamePadRightTrigger:
            return GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER;
        case key::gamePadDpadUp:
            return GLFW_GAMEPAD_BUTTON_DPAD_UP;
        case key::gamePadDpadDown:
            return GLFW_GAMEPAD_BUTTON_DPAD_DOWN;
        case key::gamePadDpadLeft:
            return GLFW_GAMEPAD_BUTTON_DPAD_LEFT;
        case key::gamePadDpadRight:
            return GLFW_GAMEPAD_BUTTON_DPAD_RIGHT;
        case key::gamePadMenu:
            return GLFW_GAMEPAD_BUTTON_BACK;
        case key::gamePadView:
            return GLFW_GAMEPAD_BUTTON_START;
        default:
            return 0;
        }
    }

    static inline key glfwKeyToEngineKey_(const int glfwKey)
    {
        switch (glfwKey)
        {
        case GLFW_KEY_0:
            return key::key0;
        case GLFW_KEY_1:
            return key::key1;
        case GLFW_KEY_2:
            return key::key2;
        case GLFW_KEY_3:
            return key::key3;
        case GLFW_KEY_4:
            return key::key4;
        case GLFW_KEY_5:
            return key::key5;
        case GLFW_KEY_6:
            return key::key6;
        case GLFW_KEY_7:
            return key::key7;
        case GLFW_KEY_8:
            return key::key8;
        case GLFW_KEY_9:
            return key::key9;

        case GLFW_KEY_A:
            return key::a;
        case GLFW_KEY_B:
            return key::b;
        case GLFW_KEY_C:
            return key::c;
        case GLFW_KEY_D:
            return key::d;
        case GLFW_KEY_E:
            return key::e;
        case GLFW_KEY_F:
            return key::f;
        case GLFW_KEY_G:
            return key::g;
        case GLFW_KEY_H:
            return key::h;
        case GLFW_KEY_I:
            return key::i;
        case GLFW_KEY_J:
            return key::j;
        case GLFW_KEY_K:
            return key::k;
        case GLFW_KEY_L:
            return key::l;
        case GLFW_KEY_M:
            return key::m;
        case GLFW_KEY_N:
            return key::n;
        case GLFW_KEY_O:
            return key::o;
        case GLFW_KEY_P:
            return key::p;
        case GLFW_KEY_Q:
            return key::q;
        case GLFW_KEY_R:
            return key::r;
        case GLFW_KEY_S:
            return key::s;
        case GLFW_KEY_T:
            return key::t;
        case GLFW_KEY_U:
            return key::u;
        case GLFW_KEY_V:
            return key::v;
        case GLFW_KEY_W:
            return key::w;
        case GLFW_KEY_X:
            return key::x;
        case GLFW_KEY_Y:
            return key::y;
        case GLFW_KEY_Z:
            return key::z;

        case GLFW_KEY_KP_0:
            return key::num0;
        case GLFW_KEY_KP_1:
            return key::num1;
        case GLFW_KEY_KP_2:
            return key::num2;
        case GLFW_KEY_KP_3:
            return key::num3;
        case GLFW_KEY_KP_4:
            return key::num4;
        case GLFW_KEY_KP_5:
            return key::num5;
        case GLFW_KEY_KP_6:
            return key::num6;
        case GLFW_KEY_KP_7:
            return key::num7;
        case GLFW_KEY_KP_8:
            return key::num8;
        case GLFW_KEY_KP_9:
            return key::num9;

        case GLFW_KEY_F1:
            return key::f1;
        case GLFW_KEY_F2:
            return key::f2;
        case GLFW_KEY_F3:
            return key::f3;
        case GLFW_KEY_F4:
            return key::f4;
        case GLFW_KEY_F5:
            return key::f5;
        case GLFW_KEY_F6:
            return key::f6;
        case GLFW_KEY_F7:
            return key::f7;
        case GLFW_KEY_F8:
            return key::f8;
        case GLFW_KEY_F9:
            return key::f9;
        case GLFW_KEY_F10:
            return key::f10;
        case GLFW_KEY_F11:
            return key::f11;
        case GLFW_KEY_F12:
            return key::f12;
        case GLFW_KEY_F13:
            return key::f13;
        case GLFW_KEY_F14:
            return key::f14;
        case GLFW_KEY_F15:
            return key::f15;
        case GLFW_KEY_F16:
            return key::f16;
        case GLFW_KEY_F17:
            return key::f17;
        case GLFW_KEY_F18:
            return key::f18;
        case GLFW_KEY_F19:
            return key::f19;
        case GLFW_KEY_F20:
            return key::f20;
        case GLFW_KEY_F21:
            return key::f21;
        case GLFW_KEY_F22:
            return key::f22;
        case GLFW_KEY_F23:
            return key::f23;
        case GLFW_KEY_F24:
            return key::f24;

        case GLFW_KEY_ESCAPE:
            return key::escape;
        case GLFW_KEY_SPACE:
            return key::space;
        case GLFW_KEY_ENTER:
            return key::enter;
        case GLFW_KEY_TAB:
            return key::tab;
        case GLFW_KEY_BACKSPACE:
            return key::backspace;
        case GLFW_KEY_LEFT_SHIFT:
            return key::leftShift;
        case GLFW_KEY_RIGHT_SHIFT:
            return key::rightShift;
        case GLFW_KEY_LEFT_CONTROL:
            return key::leftControl;
        case GLFW_KEY_RIGHT_CONTROL:
            return key::rightControl;
        case GLFW_KEY_LEFT_ALT:
            return key::leftAlt;
        case GLFW_KEY_RIGHT_ALT:
            return key::rightAlt;
        case GLFW_KEY_DELETE:
            return key::del;

        case GLFW_KEY_LEFT:
            return key::left;
        case GLFW_KEY_RIGHT:
            return key::right;
        case GLFW_KEY_UP:
            return key::up;
        case GLFW_KEY_DOWN:
            return key::down;

        case GLFW_GAMEPAD_BUTTON_A:
            return key::gamePadA;
        case GLFW_GAMEPAD_BUTTON_B:
            return key::gamePadB;
        case GLFW_GAMEPAD_BUTTON_X:
            return key::gamePadX;
        case GLFW_GAMEPAD_BUTTON_Y:
            return key::gamePadY;
        case GLFW_GAMEPAD_BUTTON_RIGHT_THUMB:
            return key::gamePadRightShoulder;
        case GLFW_GAMEPAD_BUTTON_LEFT_THUMB:
            return key::gamePadLeftShoulder;
        case GLFW_GAMEPAD_BUTTON_LEFT_BUMPER:
            return key::gamePadLeftTrigger;
        case GLFW_GAMEPAD_BUTTON_RIGHT_BUMPER:
            return key::gamePadRightTrigger;
        case GLFW_GAMEPAD_BUTTON_DPAD_UP:
            return key::gamePadDpadUp;
        case GLFW_GAMEPAD_BUTTON_DPAD_DOWN:
            return key::gamePadDpadDown;
        case GLFW_GAMEPAD_BUTTON_DPAD_LEFT:
            return key::gamePadDpadLeft;
        case GLFW_GAMEPAD_BUTTON_DPAD_RIGHT:
            return key::gamePadDpadRight;
        case GLFW_GAMEPAD_BUTTON_BACK:
            return key::gamePadMenu;
        case GLFW_GAMEPAD_BUTTON_START:
            return key::gamePadView;
        default:
            return key::unknown;
        }
    }
};

inline input::state operator|(input::state a, input::state b) noexcept
{
    using underlying = std::underlying_type_t<engine::input::input::state>;
    return static_cast<input::state>(static_cast<underlying>(a) | static_cast<underlying>(b));
}
inline input::state operator&(input::state a, input::state b) noexcept
{
    using underlying = std::underlying_type_t<engine::input::input::state>;
    return static_cast<input::state>(static_cast<underlying>(a) & static_cast<underlying>(b));
}

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
        fatalAssert(!_initialized, "window already initialized");
        _initialized = true;
        s_name = name;
        s_renderer = renderer;
        s_size = size;

// create window
#ifndef NDEBUG
        glfwSetErrorCallback([](int error, const char *description) {
            log::logError("(GLFW error code {}) \"{}\"", error, std::string(description));
        });
#endif
        fatalAssert(glfwInit() == GLFW_TRUE, "glfwInit() failed");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        s_window = glfwCreateWindow(size.x, size.y, name.c_str(), NULL, NULL);
        fatalAssert(s_window != nullptr, "glfwCreateWindow failed");
        glfwMakeContextCurrent(s_window);

        input::initialize_(s_window);

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
        // executes during application::postComponentHooks phase
        static inline auto &onRenders = *new std::vector<std::vector<std::function<void()>>>{{}};
        static inline std::mutex onRendersMutex;

      private:
        static inline void initialize_()
        {
            fatalAssert(gladLoadGL((GLADloadfunc)glfwGetProcAddress) != 0, "gladLoadGL failed");
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