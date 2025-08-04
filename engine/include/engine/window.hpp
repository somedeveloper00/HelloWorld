#pragma once

#include "os.hpp"
#include "vector2.hpp"
#include "app.hpp"
#include "log.hpp"

#ifdef WIN32
#include <dwmapi.h>
#pragma comment(lib, "dwmapi.lib")
#else
#error "Platform not supported"
#endif

namespace engine
{
    class window final
    {
    public:
        using handle = void*;

        static inline void initialize(std::string name, const vector2<size_t> center, const vector2<size_t> size, bool useMica, bool useAcrylic)
        {
            static bool _initialized = false;
            if (_initialized)
            {
                logError("window already initialized");
                return;
            }
            _initialized = true;
            _name = name;


            createWindow_(_name.c_str(), center, size, useMica, useAcrylic);
            engine::application::addPreComponentHook(tick_);
        }

    private:
        static inline std::string _name;
        static inline handle _handle;
        static inline MSG _msg{};

        static inline void tick_()
        {
            while (PeekMessage(&_msg, NULL, 0, 0, PM_REMOVE))
            {
                if (_msg.message == WM_QUIT)
                    engine::application::close();
                TranslateMessage(&_msg);
                DispatchMessageW(&_msg);
            }
        }

        static inline handle createWindow_(const char* name, const engine::vector2<size_t> center, const engine::vector2<size_t> size, bool useMica, bool useAcrylic)
        {
#ifdef WIN32
            WNDCLASS wc{};
            wc.lpfnWndProc = WndProc;
            wc.hInstance = GetModuleHandle(NULL);
            wc.lpszClassName = name;
            if (!RegisterClass(&wc))
            {
                DWORD err = GetLastError();
                engine::logError("Failed to register window class \"{}\": {}", name, err);
                return nullptr;
            }
            HWND hwnd = CreateWindowEx(
                0,
                name,
                name,
                WS_OVERLAPPEDWINDOW,
                center.x - size.x / 2,
                center.y - size.y / 2,
                size.x,
                size.y,
                NULL, NULL, NULL, NULL
            );
            if (!hwnd)
            {
                DWORD err = GetLastError();
                engine::logError("Failed to create window class \"{}\": {}", name, err);
                return nullptr;
            }

            if (useMica || useAcrylic)
            {
                const auto backdrop = useMica ? DWMSBT_MAINWINDOW : DWMSBT_TRANSIENTWINDOW;
                HRESULT hr = DwmSetWindowAttribute(
                    hwnd,
                    DWMWA_SYSTEMBACKDROP_TYPE,
                    &backdrop,
                    sizeof(backdrop)
                );
                if (FAILED(hr))
                    engine::logWarning("Failed to enable Mica backdrop");

            }

            // extend client area (avoid black areas)
            MARGINS margins{ -1 };
            DwmExtendFrameIntoClientArea(hwnd, &margins);

            ShowWindow(hwnd, SW_SHOWNORMAL);
            UpdateWindow(hwnd);
            return hwnd;
#endif
        }

        static inline LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            switch (msg)
            {
                // handle cursor shape
            case WM_SETCURSOR:
                if (LOWORD(lParam) == HTCLIENT)
                {
                    SetCursor(LoadCursor(nullptr, IDC_ARROW));
                    return TRUE;
                }
                break;
            case WM_DESTROY:
                PostQuitMessage(0);
                return TRUE;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }
    };
}