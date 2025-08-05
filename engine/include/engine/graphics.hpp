#pragma once

#include "app.hpp"
#include "benchmark.hpp"
#include "log.hpp"
#include "vector2.hpp"
#include <cassert>
#include <cstddef>
#include <glad/wgl.h>
#include <string>
#include <windef.h>

#ifdef WIN32
#include <dwmapi.h>
#include <errhandlingapi.h>
#include <minwindef.h>
#include <winerror.h>
#include <wingdi.h>
#include <winuser.h>

#pragma comment(lib, "dwmapi.lib")
#endif

namespace engine
{
class graphics final
{
  public:
    static inline void initialize(
        std::string name, const vector2<size_t> center, const vector2<size_t> size, bool useMica, bool useAcrylic)
    {
        bench(__FUNCTION__);
        static bool _initialized = false;
        if (_initialized)
        {
            engine::log::logError("window already initialized");
            return;
        }
        _initialized = true;
        _name = name;

        _handle = createWindow_(_name.c_str(), center, size, useMica, useAcrylic);
        _hdc = GetDC(_handle);
        application::addPreComponentHook(tick_);
    }

  private:
    static inline std::string _name;
#ifdef WIN32
    static inline HWND _handle;
#endif
    static inline MSG _msg{};
    static inline HDC _hdc;
    static inline HGLRC _openglContext;

    static inline void tick_()
    {
        while (PeekMessage(&_msg, NULL, 0, 0, PM_REMOVE))
        {
            if (_msg.message == WM_QUIT)
                application::close();
            TranslateMessage(&_msg);
            DispatchMessageW(&_msg);
        }
        RedrawWindow(_handle, NULL, NULL, RDW_INVALIDATE | RDW_INTERNALPAINT);
    }

    static inline HWND createWindow_(const char *name, const vector2<size_t> center, const vector2<size_t> size, bool useMica, bool useAcrylic)
    {
#ifdef WIN32
        WNDCLASS wc{};
        wc.lpfnWndProc = WndProc;
        wc.hInstance = GetModuleHandle(NULL);
        wc.lpszClassName = name;
        assertWinError_(RegisterClass(&wc), "Failed to register window class");

        HWND hwnd = CreateWindowEx(
            0, name, name,
            WS_OVERLAPPEDWINDOW,
            center.x - size.x / 2,
            center.y - size.y / 2,
            size.x, size.y,
            NULL, NULL, NULL, NULL);
        assertWinError_(hwnd, "Failed to create window class");

        if (useMica || useAcrylic)
        {
            const auto backdrop = useMica ? DWMSBT_MAINWINDOW : DWMSBT_TRANSIENTWINDOW;
            HRESULT hr = DwmSetWindowAttribute(hwnd, DWMWA_SYSTEMBACKDROP_TYPE, &backdrop, sizeof(backdrop));
            assertWinError_(!FAILED(hr), "Failed to enable Mica backdrop");
        }

        // extend client area (avoid black areas)
        // MARGINS margins{-1};
        // DwmExtendFrameIntoClientArea(hwnd, &margins);

        ShowWindow(hwnd, SW_SHOWNORMAL);
        UpdateWindow(hwnd);
        return hwnd;
#endif
    }

    static inline void setupOpengl_()
    {
        // create a dummy window to be able to use wglChoosePixelFormatARB
        WNDCLASS dummyClass = {
            .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
            .lpfnWndProc = DefWindowProcA,
            .hInstance = GetModuleHandle(0),
            .lpszClassName = "dummyClass",
        };
        assertWinError_(RegisterClass(&dummyClass), "RegisterClass failed for opengl's dummy class");
        HWND dummyWindow = CreateWindowEx(
            0,
            dummyClass.lpszClassName,
            dummyClass.lpszClassName,
            0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            dummyClass.hInstance,
            0);
        assertWinError_(dummyWindow, "CreateWindowEx failed for opengl's dummy window");

        auto dummyDc = GetDC(dummyWindow);
        PIXELFORMATDESCRIPTOR pfd = {
            .nSize = sizeof(PIXELFORMATDESCRIPTOR),
            .nVersion = 1,
            .dwFlags = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,
            .iPixelType = PFD_TYPE_RGBA,
            .cColorBits = 32,
            .cDepthBits = 24,
            .cStencilBits = 8,
            .iLayerType = PFD_MAIN_PLANE,
        };

        int dummyPixelFormat = ChoosePixelFormat(dummyDc, &pfd);
        assert(dummyPixelFormat != 0);
        assert(SetPixelFormat(dummyDc, dummyPixelFormat, &pfd));

        // create dummy context
        auto dummyContext = wglCreateContext(dummyDc);
        wglMakeCurrent(dummyDc, dummyContext);

        // load GLAD's GL and WGL pointers
        assert(gladLoaderLoadGL() && "gladLoaderLoadGL failed");
        assert(gladLoaderLoadWGL(_hdc) && "gladLoadWGL failed"); // load WGL extensions with GLAD

        // test GLAD's GL pointers and print specs
        log::logInfo("opengl version: {}", (char *)glGetString(GL_VERSION));
        log::logInfo("renderer: {}", (char *)glGetString(GL_RENDERER));
        log::logInfo("vendor: {}", (char *)glGetString(GL_VENDOR));

        // remove dummies
        wglMakeCurrent(dummyDc, NULL);
        wglDeleteContext(dummyContext);
        ReleaseDC(dummyWindow, dummyDc);
        DestroyWindow(dummyWindow);

        // Now we can use wglChoosePixelFormatARB
        int pixelFormatAttribs[]{
            WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
            WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
            WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
            WGL_ACCELERATION_ARB, WGL_FULL_ACCELERATION_ARB,
            WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
            WGL_COLOR_BITS_ARB, 32,
            WGL_DEPTH_BITS_ARB, 24,
            WGL_STENCIL_BITS_ARB, 8,
            0 // end
        };
        int piFormat;
        UINT numFormats;
        wglChoosePixelFormatARB(_hdc, pixelFormatAttribs, NULL, 1, &piFormat, &numFormats);
        assert(numFormats && "wglChoosePixelFormatARB failed");

        PIXELFORMATDESCRIPTOR lppfd;
        assert(DescribePixelFormat(_hdc, piFormat, sizeof(lppfd), &lppfd) && "DescribePixelFormat failed");
        assert(SetPixelFormat(_hdc, piFormat, &lppfd) && "SetPixelFormat failed");

        // create context
        int attribList[]{
            WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
            WGL_CONTEXT_MINOR_VERSION_ARB, 6,
            // WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
            0 // end
        };
        assert((_openglContext = wglCreateContextAttribsARB(_hdc, NULL, attribList)) && "wglCreateContextAttribsARB failed");
        wglMakeCurrent(_hdc, _openglContext);

        glEnable(GL_DEPTH_TEST);
    }

    static inline void updateOpenglWindowSize_(int width, int height)
    {
        log::logInfo("resize {} {}", width, height);
        glViewport(0, 0, width, height);
    }

#ifdef WIN32
    static inline LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
    {
        switch (msg)
        {
        case WM_CREATE:
            _hdc = GetDC(hwnd);
            setupOpengl_();
            break;
        case WM_SIZE:
            updateOpenglWindowSize_(LOWORD(lParam), HIWORD(lParam));
            break;
        case WM_SETCURSOR:
            // handle cursor shape
            if (LOWORD(lParam) == HTCLIENT)
            {
                SetCursor(LoadCursor(nullptr, IDC_ARROW));
                return TRUE;
            }
            break;
        case WM_DESTROY:
            wglDeleteContext(_openglContext);
            PostQuitMessage(0);
            return TRUE;
        case WM_PAINT:
            renderOpengl_();
            break;
        }
        return DefWindowProc(hwnd, msg, wParam, lParam);
    }

    static inline void assertWinError_(const bool expression, const char *const error)
    {
        if (!expression)
            engine::log::logError("{}. GetLastError(): {}", error, GetLastError());
        assert(expression);
    }
#endif

    static inline void renderOpengl_()
    {
        log::logInfo("render");
        glClearColor(0.f, 1.f, 0.f, 0.f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        SwapBuffers(_hdc);
    }
};
} // namespace engine