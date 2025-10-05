#pragma once
#ifndef GAMEENGINE_WINDOWS_H
#define GAMEENGINE_WINDOWS_H

// clang-format off
#include <glad/gl.h>
#include <GLFW/glfw3.h>
// clang-format on

#include "app.hpp"
#include "benchmark.hpp"
#include "data.hpp"
#include "engine/benchmark.hpp"
#include "engine/quickVector.hpp"
#include "errorHandling.hpp"
#include "glm/fwd.hpp"
#include "log.hpp"
#include <cstddef>
#include <cstdint>
#include <glm/glm.hpp>
#include <string>
#include <vector>

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

    // 0,0 is top-left. max,max is bottom-right
    static inline glm::ivec2 getMousePosition() noexcept
    {
        return s_mousePos;
    }

    // returns whether mouse is within this window
    static inline bool isMouseInWindow() noexcept
    {
        return s_mouseInWindow;
    }

    // 0,0 is center. max,max is top-right
    static inline glm::ivec2 getMousePositionCentered() noexcept;

  private:
    static inline state s_states[static_cast<size_t>(key::count)];
    static inline glm::ivec2 s_mousePos;
    static inline bool s_mouseInWindow = false;

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
        glfwSetMouseButtonCallback(window, [](GLFWwindow *window, int button, int action, int mods) {
            const auto key = static_cast<size_t>(glfwMouseKeyToEngineKey_(button));
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
        application::preComponentHooks.push_back([window]() {
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

            static double x, y;
            glfwGetCursorPos(window, &x, &y);
            updateCursorPos(x, y);
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

    static inline void updateCursorPos(const double x, const double y);
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
    static inline glm::ivec2 getFrameBufferSize() noexcept
    {
        return s_frameBufferSize;
    }

    // sets mouse position relative to the top-right of the window
    static inline void setMousePosition(const glm::ivec2 pos) noexcept
    {
        glfwSetCursorPos(s_window, pos.x, pos.y);
        input::updateCursorPos(pos.x, pos.y);
    }

    // sets mouse position relative to the center of the window
    static inline void setMousePositionCentered(const glm::ivec2 pos) noexcept
    {
        glfwSetCursorPos(s_window, s_frameBufferSize.x / 2.f + pos.x, s_frameBufferSize.y / 2.f + pos.y);
        input::updateCursorPos(pos.x, pos.y);
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

        static inline GLuint fatalGetLocation(const GLuint program, const std::string &name)
        {
            const auto r = glGetUniformLocation(program, name.c_str());
            if (r == -1)
                fatalAssert(false, ("could not find \"" + name + "\" uniform variable location.").c_str());
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

        // returns the vao for a square from -1 to 1 across x/y (no z axis). it has 2 triangles and 6 indices
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
        static inline void initialize_()
        {
            fatalAssert(gladLoadGL((GLADloadfunc)glfwGetProcAddress) != 0, "gladLoadGL failed");
            glEnable(GL_DEPTH_TEST);
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
inline void input::updateCursorPos(const double x, const double y)
{
    if (x >= 0 && x < graphics::s_frameBufferSize.x &&
        y >= 0 && y < graphics::s_frameBufferSize.y)
    {
        input::s_mousePos.x = x;
        input::s_mousePos.y = y;
        input::s_mouseInWindow = true;
    }
    else
        input::s_mouseInWindow = false;
}

inline glm::ivec2 input::getMousePositionCentered() noexcept
{
    return {s_mousePos.x - graphics::getFrameBufferSize().x / 2, -s_mousePos.y + graphics::getFrameBufferSize().y / 2};
}
} // namespace engine
#endif