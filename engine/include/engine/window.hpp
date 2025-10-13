#pragma once

// clang-format off
#include <cstdlib>
#define GLM_FORCE_LEFT_HANDED
#define GLM_FORCE_DEPTH_ZERO_TO_ONE  
#include <glad/gl.h>
#include <GLFW/glfw3.h>
// clang-format on

#include "app.hpp"
#include "data.hpp"
#include "engine/app.hpp"
#include "errorHandling.hpp"
#include "glm/fwd.hpp"
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <numbers>
#include <thread>

// #ifdef WIN32 // use high performance GPU
// extern "C"
// {
//     __declspec(dllexport) DWORD NvOptimusEnablement = 0x00000001;
//     __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
// }
// #endif

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

        mouseLeft, mouseRight, mouseMiddle,
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

    static inline bool isMouseVisibility() noexcept;
    static inline void setMouseVisibility(const bool visible) noexcept;

    // 0,0 is center. top-right is positive
    static inline glm::ivec2 getMousePosition() noexcept
    {
        return s_mousePos;
    }

    // 0,0 is center. top-right is positive
    static inline void setMousePosition(const glm::ivec2 pos) noexcept;

    // returns whether mouse is within this window
    static inline bool isMouseInWindow() noexcept
    {
        return s_mouseInWindow;
    }

    // get the mouse wheel delta from the last frame
    static inline glm::vec2 getMouseWheelDelta() noexcept
    {
        return s_mouseWheelDelta;
    }

    // returns mouse delta from last frame till this frame in pixel space. positive X is right and positive Y is up
    static inline glm::ivec2 getMouseDelta() noexcept
    {
        return s_mouseDelta;
    }

  private:
    static inline state s_states[static_cast<size_t>(key::count)];
    static inline glm::ivec2 s_mousePos;
    static inline glm::ivec2 s_mouseDelta;
    static inline bool s_mouseInWindow = false;
    static inline glm::vec2 s_mouseWheelDelta;

    static inline void initialize_(GLFWwindow *window)
    {
        static auto &s_downKeys = *new quickVector<size_t>();
        static auto &s_upKeys = *new quickVector<size_t>();
        static auto &s_repeatKeys = *new quickVector<size_t>();
        static glm::vec2 s_mouseWheelDeltaQueue{};

        glfwSetKeyCallback(window, [](GLFWwindow *window, int glfwKey, int scancode, int action, int mods) {
            auto key = static_cast<size_t>(glfwKeyToEngineKey_(glfwKey));
            if (key <= static_cast<size_t>(key::unknown) || key >= static_cast<size_t>(key::count))
                return;

            // mark for update in the next tick
            if (action == GLFW_PRESS)
                s_downKeys.push_back(key);
            else if (action == GLFW_REPEAT)
            {
                s_downKeys.erase(key);
                s_repeatKeys.push_back(key);
            }
            else
            {
                s_downKeys.erase(key);
                s_upKeys.push_back(key);
            }
        });
        glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) {
            const auto key = static_cast<size_t>(glfwMouseKeyToEngineKey_(button));
            if (key <= static_cast<size_t>(key::unknown) || key >= static_cast<size_t>(key::count))
                return;

            // mark for update in the next tick
            if (action == GLFW_PRESS)
                s_downKeys.push_back(key);
            else if (action == GLFW_REPEAT)
            {
                s_downKeys.erase(key);
                s_repeatKeys.push_back(key);
            }
            else
            {
                s_downKeys.erase(key);
                s_upKeys.push_back(key);
            }
        });
        glfwSetScrollCallback(window, [](GLFWwindow *window, double xoffset, double yoffset) {
            s_mouseWheelDeltaQueue = {xoffset, yoffset};
        });
        application::preComponentHooks.push_back([window]() {
            s_downKeys.forEach([](const auto &key) {
                if (s_states[key] == state::up)
                    s_states[key] = state::justDown;
                else if (s_states[key] == state::justDown)
                    s_states[key] = state::heldDown;
            });
            s_repeatKeys.forEachAndClear([](const auto &key) {
                s_states[key] = state::justDown | state::heldDown;
            });
            s_upKeys.forEachAndClear([](const auto &key) {
                s_states[key] = state::up;
            });
            s_mouseWheelDelta = s_mouseWheelDeltaQueue;
            s_mouseWheelDeltaQueue = {};
            updateMouseInfo();
        });
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

    static inline int engineKeyToGlfwMouseKey_(const key key)
    {
        switch (key)
        {
        case key::mouseLeft:
            return GLFW_MOUSE_BUTTON_LEFT;
        case key::mouseRight:
            return GLFW_MOUSE_BUTTON_RIGHT;
        case key::mouseMiddle:
            return GLFW_MOUSE_BUTTON_MIDDLE;
        default:
            return 0;
        }
    }

    static inline key glfwMouseKeyToEngineKey_(const int glfwKey)
    {
        switch (glfwKey)
        {
        case GLFW_MOUSE_BUTTON_LEFT:
            return key::mouseLeft;
        case GLFW_MOUSE_BUTTON_RIGHT:
            return key::mouseRight;
        case GLFW_MOUSE_BUTTON_MIDDLE:
            return key::mouseMiddle;
        default:
            return key::unknown;
        }
    }

    static inline void updateMouseInfo();
};

struct graphics final
{
    friend input;

    enum renderer : uint8_t
    {
        opengl
    };

    graphics() = delete;

    static inline color clearColor{1.f, 1.f, 0.f, 1.f};

    // gets called when the framebuffer size is changed. cannot modify when iterating
    static inline quickVector<std::function<void()>> frameBufferSizeChanged;

    static inline void initialize(std::string name, const glm::vec2 center, const glm::vec2 size, bool useMica, bool useAcrylic, renderer renderer)
    {
        static bool _initialized = false;
        fatalAssert(!_initialized, "window already initialized");
        _initialized = true;
        s_name = name;
        s_renderer = renderer;

// create window
#ifdef DEBUG
        glfwSetErrorCallback([](int error, const char *description) {
            log::logError("(GLFW error code {}) \"{}\"", error, std::string(description));
        });
#endif
        fatalAssert(glfwInit() == GLFW_TRUE, "glfwInit() failed");
        glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
        glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
        glfwWindowHint(GLFW_SAMPLES, 2);
#if DEBUG
        glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);
#endif
        glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
        s_window = glfwCreateWindow(size.x, size.y, name.c_str(), NULL, NULL);
        fatalAssert(s_window != nullptr, "glfwCreateWindow failed");
        glfwMakeContextCurrent(s_window);

        input::initialize_(s_window);

        // initialize GLAD
        if (s_renderer == renderer::opengl)
            opengl::initialize_();

        application::onExitHooks.push_back([]() {
            glfwTerminate();
        });
        application::preComponentHooks.push_back([]() {
            glfwPollEvents();
            if (glfwWindowShouldClose(s_window))
                application::close();
        });
    }

    static inline renderer getRenderer() noexcept
    {
        return s_renderer;
    }

    // get frame buffer size
    static inline const glm::ivec2& getFrameBufferSize() noexcept
    {
        return s_frameBufferSize;
    }

    struct opengl final
    {
        opengl() = delete;
        friend graphics;

        // enables opengl debug mode during the lifetime of this object (RAII), and restores previous state afterwards
        // by default, debug mode is turned off
        // uses glDebugMessageCallback
        struct debugModeContext
        {
            debugModeContext()
                : _enabled(glIsEnabled(GL_DEBUG_OUTPUT))
            {
                glEnable(GL_DEBUG_OUTPUT);
                glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
                if (s_count++ == 0)
                    glDebugMessageCallback([](GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam) {
                        if (severity == GL_DEBUG_SEVERITY_HIGH)
                        {
                            log::logError(
                                "[OPENGL DEBUG ERROR] id: {} type:{} severity:{} message:\"{}\"",
                                id,
                                type == GL_DEBUG_TYPE_ERROR ? "TYPE ERROR" : std::to_string(type),
                                "high", message);
#ifdef DEBUG
                            assert(false);
#endif
                        }
                        else
                        {
                            log::logWarning(
                                "[OPENGL DEBUG ERROR] id: {} type:{} severity:{} message:\"{}\"",
                                id,
                                type == GL_DEBUG_TYPE_ERROR ? "TYPE ERROR" : std::to_string(type),
                                severity == GL_DEBUG_SEVERITY_MEDIUM ? "medium" : severity == GL_DEBUG_SEVERITY_LOW ? "low"
                                                                                                                    : "notification",
                                message);
                        }
                    },
                                           0);
            }

            ~debugModeContext()
            {
                if (_enabled)
                    glEnable(GL_DEBUG_OUTPUT);
                else
                    glDisable(GL_DEBUG_OUTPUT);
                if (--s_count == 0)
                    glDebugMessageCallback(nullptr, 0);
            }

          private:
            static inline unsigned short s_count = 0;
            const bool _enabled;
        };

        // disables opengl depth test during the lifetime of this object (RAII), and restores previous state afterwards
        // by default, depth test is turned on
        struct noDepthTestContext
        {
            noDepthTestContext()
                : _enabled(glIsEnabled(GL_DEPTH_TEST))
            {
                glDisable(GL_DEPTH_TEST);
            }

            ~noDepthTestContext()
            {
                if (_enabled)
                    glEnable(GL_DEPTH_TEST);
                else
                    glDisable(GL_DEPTH_TEST);
            }

          private:
            const bool _enabled;
        };

        // disables opengl blending during the lifetime of this object (RAII), and restores previous state afterwards
        // by default, blend is turned on
        struct noBlendContext
        {
            noBlendContext()
                : _enabled(glIsEnabled(GL_BLEND))
            {
                glDisable(GL_BLEND);
            }

            ~noBlendContext()
            {
                if (_enabled)
                    glEnable(GL_BLEND);
                else
                    glDisable(GL_BLEND);
            }

          private:
            const bool _enabled;
        };

        // disables opengl face culling during the lifetime of this object (RAII), and restores previous state afterwards
        // by default, face culling is turned on
        struct noFaceCullingContext
        {
            noFaceCullingContext()
                : _enabled(glIsEnabled(GL_CULL_FACE))
            {
                glDisable(GL_CULL_FACE);
            }

            ~noFaceCullingContext()
            {
                if (_enabled)
                    glEnable(GL_CULL_FACE);
                else
                    glDisable(GL_CULL_FACE);
            }

          private:
            const bool _enabled;
        };

        // best not to use it directly. see addRendererHook and removeRendererHook.
        // executes when rendering a frame. if you subscribe, you should also unsubscribe later.
        // order of execution is from 0 to last.
        // feel free to insert new elements.
        // executes during application::postComponentHooks phase
        // cannot be modified during its execution (markable by waiting for onRendersMutex)
        static inline auto &onRenders = *new quickVector<quickVector<std::function<void()>>>{
            true,
            quickVector<std::function<void()>>{8},
            quickVector<std::function<void()>>{8},
            quickVector<std::function<void()>>{8}};
        static inline std::mutex onRendersMutex;

        // displays the fb as hash (color coded randomly based on its values)
        static inline void displayFrameBufferAsHashFullScreen(const GLuint fb, const float secondsPause)
        {
            debugModeContext _;
            constexpr auto vertexShader = R"(
            #version 460 core
            const vec2 verts[3] = vec2[3](
                vec2(-1.0, -1.0),
                vec2( 3.0, -1.0),
                vec2(-1.0,  3.0)
            );
            void main() 
            {
                gl_Position = vec4(verts[gl_VertexID], 0.0, 1.0);
            }
            )";
            constexpr auto fragmentShader = R"(
            #version 460 core
            layout(binding = 0) uniform usampler2D uTex;
            out vec4 FragColor;

            // 32-bit integer hash -> vec3 in [0,1]
            vec3 hash3(uint n)
            {
                // integer scrambling: different large odd constants for each channel
                n ^= n >> 16u;
                n *= 0x7feb352du;  // large odd prime
                n ^= n >> 15u;
                n *= 0x846ca68bu;
                n ^= n >> 16u;

                // create three different seeds by bit rotation & mixing
                uint x = n;
                uint y = n * 0x9e3779b1u;  // golden-ratio constant
                uint z = n * 0x85ebca77u;

                // convert to [0,1] floats using 2^-32
                const float inv32 = 1.0 / 4294967296.0; // 1/2^32
                return vec3(float(x), float(y), float(z)) * inv32;
            }

            void main() 
            {
                ivec2 texel = ivec2(gl_FragCoord.xy);
                uint hash = texelFetch(uTex, texel, 0).r;
                vec3 result = hash3(hash);
                FragColor = vec4(result, 1.0);
            }
            )";

            static auto s_program = graphics::opengl::fatalCreateProgram("pointerRead component's debug", vertexShader, fragmentShader);
            static auto s_texLocation = graphics::opengl::fatalGetLocation(s_program, "uTex");

            // read fb's color
            glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);
            GLint colorAttachment;
            glGetFramebufferAttachmentParameteriv(GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_FRAMEBUFFER_ATTACHMENT_OBJECT_NAME, &colorAttachment);
            glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);

            if (colorAttachment == 0)
            {
                log::logError("displayFrameBufferFullScreen: framebuffer {} has no color attachment.", fb);
                std::this_thread::sleep_for(std::chrono::duration<float>(secondsPause));
                return;
            }

            // draw
            static GLuint vao = 0;
            if (!vao)
                glGenVertexArrays(1, &vao);
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glUseProgram(s_program);
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, colorAttachment);
            glUniform1i(s_texLocation, 0);
            glBindVertexArray(vao);
            glDrawArrays(GL_TRIANGLES, 0, 3);
            glfwSwapBuffers(graphics::s_window);

            // cleanup
            glBindVertexArray(0);
            glUseProgram(0);
            glBindTexture(GL_TEXTURE_2D, 0);

            // sleep
            std::this_thread::sleep_for(std::chrono::duration<float>(secondsPause));
        }

        // same as createProgram, but it'll cause a fatal assert
        static inline GLuint fatalCreateProgram(const char *const debugName, const std::string &vertexShaderSource, const std::string &fragmentShaderSource)
        {
            const auto program = createProgram(vertexShaderSource, fragmentShaderSource);
            if (!program)
                fatalAssert(false, ("could not create opengl program for \"" + std::string(debugName) + "\"").c_str());
            return program;
        }

        // compiles the source vertex shader and the source fragment shader, links them to a program and returns the program, or 0/GL_FALSE if failed
        static inline GLuint createProgram(const std::string &vertexShaderSource, const std::string &fragmentShaderSource)
        {
            auto vertexShader = compileVertexShader(vertexShaderSource);
            if (vertexShader == GL_FALSE)
                return GL_FALSE;
            auto fragmentShader = compileFragmentShader(fragmentShaderSource);
            if (fragmentShader == GL_FALSE)
                return GL_FALSE;
            auto program = glCreateProgram();
            glAttachShader(program, vertexShader);
            glAttachShader(program, fragmentShader);
            glLinkProgram(program);
            glDeleteShader(vertexShader);
            glDeleteShader(fragmentShader);
            GLint success;
            glGetProgramiv(program, GL_LINK_STATUS, &success);
            if (success != GL_TRUE)
            {
                char buffer[512];
                glGetProgramInfoLog(program, 512, NULL, buffer);
                log::logError("opengl shader linking failed: {}", buffer);
                return GL_FALSE;
            }
            return program;
        }

        static inline GLuint fatalGetLocation(const GLuint program, const char *const name)
        {
            const auto r = glGetUniformLocation(program, name);
            if (r == -1)
                fatalAssert(false, ("could not find \"" + std::string(name) + "\" uniform variable location.").c_str());
            return r;
        }

        // compiles the source vertex shader and returns the shader object or 0/GL_FALSE if failed
        static inline GLuint compileVertexShader(const std::string &source)
        {
            auto shader = glCreateShader(GL_VERTEX_SHADER);
            auto *cstr = source.c_str();
            glShaderSource(shader, 1, &cstr, nullptr);
            glCompileShader(shader);
            GLint success;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (success != GL_TRUE)
            {
                char buffer[512];
                glGetShaderInfoLog(shader, 512, NULL, buffer);
                log::logError("opengl vertex shader compilation error: {}", buffer);
                return GL_FALSE;
            }
            return shader;
        }

        // compiles the source fragment shader and returns the shader object or 0/GL_FALSE if failed
        static inline GLuint compileFragmentShader(const std::string &source)
        {
            auto shader = glCreateShader(GL_FRAGMENT_SHADER);
            auto *cstr = source.c_str();
            glShaderSource(shader, 1, &cstr, nullptr);
            glCompileShader(shader);
            GLint success;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (success != GL_TRUE)
            {
                char buffer[512];
                glGetShaderInfoLog(shader, 512, NULL, buffer);
                log::logError("opengl fragment shader compilation error: {}", buffer);
                return GL_FALSE;
            }
            return shader;
        }

        // insert a new render hook
        template <typename Func>
        static inline void addRendererHook(const size_t order, Func &&func)
        {
            onRendersMutex.lock();
            while (onRenders.size() <= order)
                onRenders.emplace_back(4);
            onRenders[order].emplace_back(std::forward<Func>(func));
            onRendersMutex.unlock();
        }

        // remove a render hook
        template <typename Func>
        static inline void removeRendererHook(const size_t order, Func &&func)
        {
            onRendersMutex.lock();
            while (onRenders.size() <= order)
                return;
            auto target = std::function<void()>(std::forward<Func>(func)).target<void (*)()>();
            onRenders[order].eraseIf([target](const std::function<void()> &hook) {
                auto otherTarget = hook.target<void (*)()>();
                return target == otherTarget;
            });
            onRendersMutex.unlock();
        }

        // returns a basic flat shader's program
        static inline GLuint getBasicFlatShader(GLuint &colorLocation, GLuint &modelMatrixLocation, GLuint &viewMatrixLocation, GLuint &projectionMatrixLocation)
        {
            const auto vertexSource = R"(
            #version 460 core
            layout(location = 0) in vec3 aPos;

            uniform mat4 modelMatrix;
            uniform mat4 viewMatrix;
            uniform mat4 projectionMatrix;

            void main()
            {
                gl_Position = projectionMatrix * viewMatrix * modelMatrix * vec4(aPos, 1.0);
            }
            )";
            const auto fragmentSource = R"(
            #version 460 core
            uniform vec4 color;
            out vec4 FragColor;

            void main()
            {
                FragColor = color;
            }
            )";
            static GLuint s_program = 0;
            static GLuint s_colorLocation;
            static GLuint s_modelMatrixLocation;
            static GLuint s_viewMatrixLocation;
            static GLuint s_projectionMatrixLocation;
            if (!s_program)
            {
                s_program = fatalCreateProgram("basic flat", vertexSource, fragmentSource);
                s_colorLocation = fatalGetLocation(s_program, "color");
                s_modelMatrixLocation = fatalGetLocation(s_program, "modelMatrix");
                s_viewMatrixLocation = fatalGetLocation(s_program, "viewMatrix");
                s_projectionMatrixLocation = fatalGetLocation(s_program, "projectionMatrix");
            }
            colorLocation = s_colorLocation;
            modelMatrixLocation = s_modelMatrixLocation;
            viewMatrixLocation = s_viewMatrixLocation;
            projectionMatrixLocation = s_projectionMatrixLocation;
            return s_program;
        }

        // returns the vao for a capsule from -1 to 1 across x/y/z
        // triangles: SideLineCount * 4
        // indices: SideLineCount * 4 * 3
        // vertices: SideLineCount * 2 + 2
        template <size_t SideLineCount = 32>
        static inline GLuint getCapsuleVao()
        {
            static GLuint s_vao = 0;
            if (s_vao == 0)
            {
                using VertexData = std::array<float, 6 * (SideLineCount * 2 + 2)>;
                using Indices = std::array<size_t, 3 * (SideLineCount * 4)>;
                static constexpr VertexData vertexData = capsuleCreateVertexData_<SideLineCount>();
                static constexpr Indices indices = capsuleCreateIndices_<SideLineCount>();
                GLuint vbo, ebo;
                glGenVertexArrays(1, &s_vao);
                glGenBuffers(1, &vbo);
                glGenBuffers(1, &ebo);

                glBindVertexArray(s_vao);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertexData), vertexData.data(), GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices.data(), GL_STATIC_DRAW);

                // position
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, 0);
                glEnableVertexAttribArray(1);
                glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 6, (void *)(3 * sizeof(float)));

                glBindVertexArray(0);
            }
            return s_vao;
        }

        // returns the vao for a cube from -1 to 1 across x/y/z
        // triangles: 6 * 2
        // indices: 6 * 2 * 3
        // vertices: 8
        static inline GLuint getCubeVao()
        {
            static GLuint s_vao = 0;
            if (s_vao == 0)
            {
                static const float vertices[] = {
                    // back
                    -1, -1, -1,
                    -1, 1, -1,
                    1, 1, -1,
                    1, -1, -1,
                    // front
                    -1, -1, 1,
                    -1, 1, 1,
                    1, 1, 1,
                    1, -1, 1};
                static const GLuint indices[] = {
                    // back
                    0, 1, 2,
                    2, 3, 0,
                    // front
                    4, 5, 6,
                    6, 7, 4,
                    // top
                    3, 2, 6,
                    6, 7, 3,
                    // bottom
                    1, 0, 4,
                    4, 5, 1,
                    // right
                    1, 2, 6,
                    6, 5, 1,
                    // left
                    0, 3, 7,
                    7, 4, 0};
                GLuint vbo, ebo;
                glGenVertexArrays(1, &s_vao);
                glGenBuffers(1, &vbo);
                glGenBuffers(1, &ebo);

                glBindVertexArray(s_vao);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

                // position
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(float) * 3, 0);

                glBindVertexArray(0);
                glBindBuffer(GL_ARRAY_BUFFER, 0);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
            }
            return s_vao;
        }

        // returns the vao for a square from -1 to 1 across x/y (no z axis)
        // triangles: 2
        // indices: 6
        // vertices: 4
        static inline GLuint getSquareVao()
        {
            static GLuint s_vao = 0;
            if (s_vao == 0)
            {
                static const float vertices[] = {
                    1, 1,
                    1, -1,
                    -1, -1,
                    -1, 1};
                static const GLuint indices[] = {
                    0, 1, 2,
                    2, 3, 0};
                GLuint vbo, ebo;
                glGenVertexArrays(1, &s_vao);
                glGenBuffers(1, &vbo);
                glGenBuffers(1, &ebo);

                glBindVertexArray(s_vao);
                glBindBuffer(GL_ARRAY_BUFFER, vbo);
                glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
                glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
                glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

                // position
                glEnableVertexAttribArray(0);
                glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, sizeof(float) * 2, 0);

                glBindVertexArray(0);
            }
            return s_vao;
        }

      private:
        // in order: top center, bottom center, bottom circle, top circle
        template <size_t SideLineCount>
        static consteval std::array<float, 6 * (SideLineCount * 2 + 2)> capsuleCreateVertexData_()
        {
            std::array<float, 6 * (SideLineCount * 2 + 2)> r;
            constexpr size_t vertexCount = SideLineCount * 2 + 2;
            auto setPosition = [&](size_t index, float x, float y, float z) {
                r[index * 6 + 0] = x;
                r[index * 6 + 1] = y;
                r[index * 6 + 2] = z;
            };
            auto setNormal = [&](size_t index, float x, float y, float z) {
                r[index * 6 + 3 + 0] = x;
                r[index * 6 + 3 + 1] = y;
                r[index * 6 + 3 + 2] = z;
            };
            // bottom vertex
            setPosition(0, 0, -1, 0);
            setNormal(0, 0, -1, 0);
            // top vertex
            setPosition(1, 0, 1, 0);
            setNormal(1, 0, 1, 0);

            auto indexToRadian = [](size_t index) {
                return std::lerp(0, 2 * std::numbers::pi, (float)index / (float)SideLineCount);
            };
            // bottom circle
            for (size_t i = 0; i < SideLineCount; i++)
            {
                setPosition(
                    2 + i,
                    constSin(indexToRadian(i)), // x
                    -1,                         // y
                    constCos(indexToRadian(i))  // z
                );
                setNormal(
                    2 + i,
                    constSin(indexToRadian(i)), // x
                    0,                          // y
                    constCos(indexToRadian(i))  // z
                );
            }
            // top circle
            for (size_t i = 0; i < SideLineCount; i++)
            {
                setPosition(
                    2 + SideLineCount + i,
                    constSin(indexToRadian(i)), // x
                    1,                          // y
                    constCos(indexToRadian(i))  // z
                );
                setNormal(
                    2 + SideLineCount + i,
                    constSin(indexToRadian(i)), // x
                    0,                          // y
                    constCos(indexToRadian(i))  // z
                );
            }
            return r;
        }

        template <size_t SideLineCount>
        static consteval std::array<size_t, 3 * (SideLineCount * 4)> capsuleCreateIndices_()
        {
            std::array<size_t, 3 * (SideLineCount * 4)> r;
            size_t nextIndex = 0;
            auto setTriangle = [&](size_t vert1, size_t vert2, size_t vert3) {
                r[nextIndex++] = vert1;
                r[nextIndex++] = vert2;
                r[nextIndex++] = vert3;
            };
            auto getBottomVertexAtIndex = [&](size_t index) {
                return 2 + index;
            };
            auto getTopVertexAtIndex = [&](size_t index) {
                return 2 + SideLineCount + index;
            };
            // bottom circle
            for (size_t i = 0; i < SideLineCount; i++)
                setTriangle(0,
                            getBottomVertexAtIndex(i),
                            getBottomVertexAtIndex((i + 1) % SideLineCount));
            // top circle
            for (size_t i = 0; i < SideLineCount; i++)
                setTriangle(1,
                            getTopVertexAtIndex(i),
                            getTopVertexAtIndex((i + 1) % SideLineCount));
            // sides
            for (size_t i = 0; i < SideLineCount; i++)
                setTriangle(getBottomVertexAtIndex(i),
                            getBottomVertexAtIndex((i + 1) % SideLineCount),
                            getTopVertexAtIndex(i));
            for (size_t i = 0; i < SideLineCount; i++)
                setTriangle(getTopVertexAtIndex(i),
                            getTopVertexAtIndex((i + 1) % SideLineCount),
                            getBottomVertexAtIndex((i + 1) % SideLineCount));
            return r;
        }

        static inline void initialize_()
        {
            fatalAssert(gladLoadGL((GLADloadfunc)glfwGetProcAddress) != 0, "gladLoadGL failed");
            glEnable(GL_DEPTH_TEST);
            glClipControl(GL_LOWER_LEFT, GL_ZERO_TO_ONE); // depth between 0 and 1 (no more negatives)
            glDepthFunc(GL_LESS);
            glDepthMask(GL_TRUE);
            glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
            glEnable(GL_BLEND);

            // handle frame buffer size
            glfwGetFramebufferSize(s_window, &s_frameBufferSize.x, &s_frameBufferSize.y);
            glViewport(0, 0, s_frameBufferSize.x, s_frameBufferSize.y);
            glfwSetFramebufferSizeCallback(s_window, [](GLFWwindow *window, const int width, const int height) {
                glViewport(0, 0, width, height);
                s_frameBufferSize.x = width;
                s_frameBufferSize.y = height;
                frameBufferSizeChanged.forEach([](const auto &func) { func(); });
                tick_();
            });

            application::postComponentHooks.push_back(tick_);
        }

        static inline void tick_()
        {
            bench("opengl rendering");
            glClearColor(graphics::clearColor.r, graphics::clearColor.g, graphics::clearColor.b, graphics::clearColor.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // execute a copy of onRenders
            onRendersMutex.lock();
            onRenders.forEach([](const quickVector<std::function<void()>> &funcs) {
                funcs.forEach([](const auto &func) {
                    func();
                });
            });
            onRendersMutex.unlock();
            glfwSwapBuffers(s_window);
        }
    };

  private:
    static inline renderer s_renderer;

    // size of the frame buffer in pixels
    static inline glm::ivec2 s_frameBufferSize;

    static inline GLFWwindow *s_window;
    static inline std::string s_name;
    static inline float s_dpi;
};

#ifndef GAMEENGINE_WINDOWS_H
#define GAMEENGINE_WINDOWS_H
inline input::state operator|(input::state a, input::state b) noexcept
{
    using underlying = std::underlying_type_t<input::state>;
    return static_cast<input::state>(static_cast<underlying>(a) | static_cast<underlying>(b));
}
inline input::state operator&(input::state a, input::state b) noexcept
{
    using underlying = std::underlying_type_t<input::state>;
    return static_cast<input::state>(static_cast<underlying>(a) & static_cast<underlying>(b));
}
inline void input::updateMouseInfo()
{
    static double x, y;
    glfwGetCursorPos(graphics::s_window, &x, &y);

    // convert glfw (relative to window's top-left and negated Y) to human readable format (relative to window's center)
    x -= graphics::s_frameBufferSize.x / 2;
    y -= graphics::s_frameBufferSize.y / 2;
    y *= -1;

    const glm::ivec2 newMousePos = {x, y};
    s_mouseDelta = newMousePos - s_mousePos;
    s_mousePos = newMousePos;
    s_mouseInWindow = s_mousePos.x > -graphics::s_frameBufferSize.x / 2 && s_mousePos.x < graphics::s_frameBufferSize.x / 2 &&
                      s_mousePos.y > -graphics::s_frameBufferSize.y / 2 && s_mousePos.y < graphics::s_frameBufferSize.y / 2;
}
inline void input::setMousePosition(glm::ivec2 pos) noexcept
{
    // convert human readable format (relative to window's center) to glfw (relative to window's top-left and negated Y)
    glfwSetCursorPos(graphics::s_window,
                     pos.x + graphics::s_frameBufferSize.x / 2,
                     -pos.y + graphics::s_frameBufferSize.y / 2);
    s_mouseDelta = pos - s_mousePos;
    s_mousePos = pos;
    s_mouseInWindow = s_mousePos.x > -graphics::s_frameBufferSize.x / 2 && s_mousePos.x < graphics::s_frameBufferSize.x / 2 &&
                      s_mousePos.y > -graphics::s_frameBufferSize.y / 2 && s_mousePos.y < graphics::s_frameBufferSize.y / 2;
}
inline void input::setMouseVisibility(const bool visible) noexcept
{
    glfwSetInputMode(graphics::s_window, GLFW_CURSOR, visible ? GLFW_CURSOR_NORMAL : GLFW_CURSOR_HIDDEN);
}
inline bool input::isMouseVisibility() noexcept
{
    return glfwGetInputMode(graphics::s_window, GLFW_CURSOR) != GLFW_CURSOR_HIDDEN;
}
} // namespace engine
#endif
