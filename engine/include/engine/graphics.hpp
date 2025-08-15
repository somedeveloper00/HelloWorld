#pragma once

#include "app.hpp"
#include "benchmark.hpp"
#include "data.hpp"
#include "log.hpp"
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <glad/gl.h>
#include <glad/wgl.h>
#include <glm/glm.hpp>
#include <string>
#include <winbase.h>
#include <windef.h>
#include <winnls.h>
#include <winnt.h>

#ifdef WIN32
#include <dwmapi.h>
#include <errhandlingapi.h>
#include <minwindef.h>
#include <winerror.h>
#include <wingdi.h>
#include <winuser.h>
#pragma comment(lib, "dwmapi.lib")
#endif

namespace
{
#ifdef WIN32
static inline void assertWinError_(const bool expression, const char *const error)
{
    if (!expression)
    {
        auto errorId = GetLastError();
        std::string errorMessage = "";
        if (errorId != 0)
        {
            LPSTR buff = nullptr;
            auto size = FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                                      NULL, errorId, GetSystemDefaultUILanguage(), (LPSTR)&buff, 0, NULL);
            errorMessage = std::string((char *)buff, size);
            LocalFree(buff);
        }
        engine::log::logError("{} | GetLastError(): {}", error, errorMessage);
    }
    assert(expression);
}
#endif
} // namespace

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
        if (_initialized)
        {
            engine::log::logError("window already initialized");
            return;
        }
        _initialized = true;
        _name = name;
        _renderer = renderer;
        win32::initialize_(name, center, size, useMica, useAcrylic);
    }

    static inline renderer getRenderer() noexcept
    {
        return _renderer;
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
        static inline void initialize_(HDC dc)
        {
            // create a dummy window to be able to use wglChoosePixelFormatARB
            WNDCLASS dummyClass = {
                .lpfnWndProc = DefWindowProcA,
                .hInstance = GetModuleHandle(NULL),
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
            assert(gladLoaderLoadWGL(dc) && "gladLoadWGL failed"); // load WGL extensions with GLAD

            // test GLAD's GL pointers and print specs
            log::logInfo("opengl version: {}", (char *)glGetString(GL_VERSION));
            log::logInfo("renderer: {}", (char *)glGetString(GL_RENDERER));
            log::logInfo("vendor: {}", (char *)glGetString(GL_VENDOR));

            // remove dummies
            wglMakeCurrent(NULL, NULL);
            wglDeleteContext(dummyContext);
            ReleaseDC(dummyWindow, dummyDc);
            DestroyWindow(dummyWindow);
            UnregisterClass(dummyClass.lpszClassName, dummyClass.hInstance);

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
            wglChoosePixelFormatARB(dc, pixelFormatAttribs, NULL, 1, &piFormat, &numFormats);
            assert(numFormats && "wglChoosePixelFormatARB failed");

            PIXELFORMATDESCRIPTOR lppfd;
            assert(DescribePixelFormat(dc, piFormat, sizeof(lppfd), &lppfd) && "DescribePixelFormat failed");
            assert(SetPixelFormat(dc, piFormat, &lppfd) && "SetPixelFormat failed");

            // create context
            int attribList[]{
                WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
                WGL_CONTEXT_MINOR_VERSION_ARB, 6,
                WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
                0 // end
            };
            assert((win32::_openglContext = wglCreateContextAttribsARB(dc, NULL, attribList)) && "wglCreateContextAttribsARB failed");
            wglMakeCurrent(dc, win32::_openglContext);

            glEnable(GL_DEPTH_TEST);
            application::hooksMutex.lock();
            application::postComponentHooks.push_back(tick_);
            application::onExitHooks.push_back(onExit_);
            application::hooksMutex.unlock();
        }

        static inline void updateWindowSize_(glm::vec2 size)
        {
            glViewport(0, 0, size.x, size.y);
        }

        static inline void tick_()
        {
#if WIN32
            if (!win32::_hwnd)
                return;
            assert(win32::_openglContext && "where is opengl context?");

#endif
            glClearColor(graphics::clearColor.r, graphics::clearColor.g, graphics::clearColor.b, graphics::clearColor.a);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

            // execute a copy of onRenders
            onRendersMutex.lock();
            auto onRendersCopy = onRenders;
            onRendersMutex.unlock();
            for (auto &row : onRendersCopy)
                for (auto &render : row)
                    render();

#if WIN32
            auto dc = GetDC(win32::_hwnd);
            assertWinError_(dc != 0, "GetDC failed");
            SwapBuffers(dc);
            ReleaseDC(win32::_hwnd, dc);
#endif
        }

        static inline void onExit_()
        {
            if (win32::_openglContext)
            {
                wglMakeCurrent(NULL, NULL);
                wglDeleteContext(win32::_openglContext);
                win32::_openglContext = NULL;
            }
        }
    };

#if WIN32
    struct win32 final
    {
        friend graphics;
        friend struct opengl;

      private:
        static inline HWND _hwnd{0};
        static inline HGLRC _openglContext{0};

        static inline void initialize_(const std::string name, const glm::vec2 center, const glm::vec2 size, bool useMica, bool useAcrylic)
        {
            _hwnd = createWindow_(name.c_str(), center, size, useMica, useAcrylic);
            application::hooksMutex.lock();
            application::preComponentHooks.push_back([]() {
                MSG msg{};
                while (PeekMessage(&msg, _hwnd, 0, 0, PM_REMOVE))
                {
                    if (msg.message == WM_QUIT)
                        application::close();
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }
            });
            application::hooksMutex.unlock();
        }

        static inline LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
        {
            switch (msg)
            {
            case WM_CREATE: {
                auto dc = GetDC(hwnd);
                if (_renderer == renderer::opengl)
                    opengl::initialize_(dc);
                ReleaseDC(hwnd, dc);
                break;
            }
            case WM_SIZE:
                opengl::updateWindowSize_({LOWORD(lParam), HIWORD(lParam)});
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
                PostQuitMessage(0);
                return FALSE;
            case WM_PAINT:
                opengl::tick_();
                break;
            }
            return DefWindowProc(hwnd, msg, wParam, lParam);
        }

        static inline HWND createWindow_(const char *name, const glm::vec2 center, const glm::vec2 size, bool useMica, bool useAcrylic)
        {
            WNDCLASS wc{
                .style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC,
                .lpfnWndProc = WndProc,
                .hInstance = GetModuleHandle(NULL),
                .lpszClassName = name};
            assertWinError_(RegisterClass(&wc), "Failed to register window class");

            HWND hwnd = CreateWindowEx(
                0, name, NULL,
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
        }
    };
#endif

  private:
    static inline renderer _renderer;
    static inline std::string _name;
};
} // namespace engine