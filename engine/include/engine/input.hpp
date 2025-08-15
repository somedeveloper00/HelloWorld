#pragma once
#include "app.hpp"
#include "benchmark.hpp"
#include "log.hpp"
#include <array>
#include <cstdint>

#if WIN32
#include <Windows.h>
#else
#error "platform not supported"
#endif

namespace engine
{
class input final
{
  public:
    // clang-format off
    enum class key : uint8_t
    {
        unknown = 0,

        key1, key2, key3, key4, key5, key6, key7, key8, key9, key0, 
        a, b, c, d, e, f, g, h, i, j, k, l, m, n, o, p, q, r, s, t, u, v, w, x, y, z, 
        num0, num1, num2, num3, num4, num5, num6, num7, num8, num9, 
        f0, f1, f2, f3, f4, f5, f6, f7, f8, f9, f10, f11, f12, f13, f14, f15, f16, f17, f18, f19, f20, f21, f22, f23, f24, 
        escape, space, enter, tab, backspace, shift, leftShift, rightShift, control, leftControl, rightControl, alt, leftAlt, rightAlt, del, left, right, up, down, 
        gamePadA, gamePadB, gamePadX, gamePadY, gamePadRightShoulder, gamePadLeftShoulder, gamePadLeftTrigger, gamePadRightTrigger, gamePadDpadUp, gamePadDpadDown, gamePadDpadLeft, gamePadDpadRight, gamePadMenu, gamePadView, gamePadLeftThumbstickButton, gamePadRightThumbstickButton, gamePadLeftThumbstickUp, gamePadLeftThumbstickDown, gamePadLeftThumbstickRight, gamePadLeftThumbstickLeft, gamePadRightThumbstickUp, gamePadRightThumbstickDown, gamePadRightThumbstickRight, gamePadRightThumbstickLeft, 

        count
    };
    // clang-format on

    enum class state : uint8_t
    {
        up,
        justDown,
        heldDown
    };

    enum class MouseButton : uint8_t
    {
        Left,
        Right,
        Middle,
        Count
    };

    static inline void initialize()
    {
        bench(__FUNCTION__);
        static bool s_initialized = false;
        if (s_initialized)
        {
            log::logError("window already initialized");
            return;
        }
        s_initialized = true;

        // for console keyboard input
        HANDLE hStdin = GetStdHandle(STD_INPUT_HANDLE);
        DWORD mode;
        GetConsoleMode(hStdin, &mode);
        mode &= ~ENABLE_QUICK_EDIT_MODE;
        SetConsoleMode(hStdin, mode);

        application::hooksMutex.lock();
        application::preComponentHooks.push_back(internal_tick_);
        application::hooksMutex.unlock();
    }

    static inline state getKeyState(const key key)
    {
        return states[static_cast<size_t>(key)];
    }

    static inline bool isKeyHeldDown(const key key)
    {
        return getKeyState(key) == state::heldDown;
    }

    static inline bool isKeyJustDown(const key key)
    {
        return getKeyState(key) == state::justDown;
    }

    static inline bool isKeyUp(const key key)
    {
        return getKeyState(key) == state::up;
    }

  private:
    static inline std::array<state, static_cast<size_t>(key::count)> initializeStates_()
    {
        std::array<state, static_cast<size_t>(key::count)> r{};
        for (size_t i = 0; i < static_cast<size_t>(key::count); i++)
            r[i] = state::up;
        return r;
    }

    static inline std::array<state, static_cast<size_t>(key::count)> states = initializeStates_();

    static constexpr SHORT WinKeyDown = SHORT(0x8000); // wrapped to remove C4309

    static inline void internal_tick_()
    {
        for (size_t i = 0; i < states.size(); i++)
        {
            auto &state = states[i];
            auto key = getKeyFromIndex_(i);
            auto vCode = getWindowsVirtualKeyCode_(key);
            auto r = GetAsyncKeyState(vCode);
            if (r & WinKeyDown)
                state = state == state::up ? state::justDown : state::heldDown;
            else
                state = state::up;
        }
    }

    static inline key getKeyFromIndex_(const size_t index)
    {
        if (index < static_cast<size_t>(key::count))
            return static_cast<key>(index);
        return key::unknown;
    }

    static inline int getWindowsVirtualKeyCode_(const key key)
    {
        switch (key)
        {
        case key::key0:
            return '0';
        case key::key1:
            return '1';
        case key::key2:
            return '2';
        case key::key3:
            return '3';
        case key::key4:
            return '4';
        case key::key5:
            return '5';
        case key::key6:
            return '6';
        case key::key7:
            return '7';
        case key::key8:
            return '8';
        case key::key9:
            return '9';

        case key::a:
            return 'A';
        case key::b:
            return 'B';
        case key::c:
            return 'C';
        case key::d:
            return 'D';
        case key::e:
            return 'E';
        case key::f:
            return 'F';
        case key::g:
            return 'G';
        case key::h:
            return 'H';
        case key::i:
            return 'I';
        case key::j:
            return 'J';
        case key::k:
            return 'K';
        case key::l:
            return 'L';
        case key::m:
            return 'M';
        case key::n:
            return 'N';
        case key::o:
            return 'O';
        case key::p:
            return 'P';
        case key::q:
            return 'Q';
        case key::r:
            return 'R';
        case key::s:
            return 'S';
        case key::t:
            return 'T';
        case key::u:
            return 'U';
        case key::v:
            return 'V';
        case key::w:
            return 'W';
        case key::x:
            return 'X';
        case key::y:
            return 'Y';
        case key::z:
            return 'Z';

        case key::num0:
            return VK_NUMPAD0;
        case key::num1:
            return VK_NUMPAD1;
        case key::num2:
            return VK_NUMPAD2;
        case key::num3:
            return VK_NUMPAD3;
        case key::num4:
            return VK_NUMPAD4;
        case key::num5:
            return VK_NUMPAD5;
        case key::num6:
            return VK_NUMPAD6;
        case key::num7:
            return VK_NUMPAD7;
        case key::num8:
            return VK_NUMPAD8;
        case key::num9:
            return VK_NUMPAD9;

        case key::f1:
            return VK_F1;
        case key::f2:
            return VK_F2;
        case key::f3:
            return VK_F3;
        case key::f4:
            return VK_F4;
        case key::f5:
            return VK_F5;
        case key::f6:
            return VK_F6;
        case key::f7:
            return VK_F7;
        case key::f8:
            return VK_F8;
        case key::f9:
            return VK_F9;
        case key::f10:
            return VK_F10;
        case key::f11:
            return VK_F11;
        case key::f12:
            return VK_F12;
        case key::f13:
            return VK_F13;
        case key::f14:
            return VK_F14;
        case key::f15:
            return VK_F15;
        case key::f16:
            return VK_F16;
        case key::f17:
            return VK_F17;
        case key::f18:
            return VK_F18;
        case key::f19:
            return VK_F19;
        case key::f20:
            return VK_F20;
        case key::f21:
            return VK_F21;
        case key::f22:
            return VK_F22;
        case key::f23:
            return VK_F23;
        case key::f24:
            return VK_F24;

        case key::escape:
            return VK_ESCAPE;
        case key::space:
            return VK_SPACE;
        case key::enter:
            return VK_RETURN;
        case key::tab:
            return VK_TAB;
        case key::backspace:
            return VK_BACK;
        case key::shift:
            return VK_SHIFT;
        case key::leftShift:
            return VK_LSHIFT;
        case key::rightShift:
            return VK_RSHIFT;
        case key::control:
            return VK_CONTROL;
        case key::leftControl:
            return VK_LCONTROL;
        case key::rightControl:
            return VK_RCONTROL;
        case key::alt:
            return VK_MENU;
        case key::leftAlt:
            return VK_LMENU;
        case key::rightAlt:
            return VK_RMENU;
        case key::del:
            return VK_DELETE;

        case key::left:
            return VK_LEFT;
        case key::right:
            return VK_RIGHT;
        case key::up:
            return VK_UP;
        case key::down:
            return VK_DOWN;

        case key::gamePadA:
            return VK_GAMEPAD_A;
        case key::gamePadB:
            return VK_GAMEPAD_B;
        case key::gamePadX:
            return VK_GAMEPAD_X;
        case key::gamePadY:
            return VK_GAMEPAD_Y;
        case key::gamePadRightShoulder:
            return VK_GAMEPAD_RIGHT_SHOULDER;
        case key::gamePadLeftShoulder:
            return VK_GAMEPAD_LEFT_SHOULDER;
        case key::gamePadLeftTrigger:
            return VK_GAMEPAD_LEFT_TRIGGER;
        case key::gamePadRightTrigger:
            return VK_GAMEPAD_RIGHT_TRIGGER;
        case key::gamePadDpadUp:
            return VK_GAMEPAD_DPAD_UP;
        case key::gamePadDpadDown:
            return VK_GAMEPAD_DPAD_DOWN;
        case key::gamePadDpadLeft:
            return VK_GAMEPAD_DPAD_LEFT;
        case key::gamePadDpadRight:
            return VK_GAMEPAD_DPAD_RIGHT;
        case key::gamePadMenu:
            return VK_GAMEPAD_MENU;
        case key::gamePadView:
            return VK_GAMEPAD_VIEW;
        case key::gamePadLeftThumbstickButton:
            return VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON;
        case key::gamePadRightThumbstickButton:
            return VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON;
        case key::gamePadLeftThumbstickUp:
            return VK_GAMEPAD_LEFT_THUMBSTICK_UP;
        case key::gamePadLeftThumbstickDown:
            return VK_GAMEPAD_LEFT_THUMBSTICK_DOWN;
        case key::gamePadLeftThumbstickRight:
            return VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT;
        case key::gamePadLeftThumbstickLeft:
            return VK_GAMEPAD_LEFT_THUMBSTICK_LEFT;
        case key::gamePadRightThumbstickUp:
            return VK_GAMEPAD_RIGHT_THUMBSTICK_UP;
        case key::gamePadRightThumbstickDown:
            return VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN;
        case key::gamePadRightThumbstickRight:
            return VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT;
        case key::gamePadRightThumbstickLeft:
            return VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT;

        default:
            return 0;
        }
    }
};
} // namespace engine