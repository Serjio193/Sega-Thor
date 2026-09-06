#include "platform/window.hpp"

#ifdef _WIN32

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include <algorithm>
#include <string>

namespace oasis::platform {
namespace {

constexpr wchar_t kWindowClass[] = L"OasisNativeControlledScreen";

NativeWindow* self(HWND window) noexcept {
    return reinterpret_cast<NativeWindow*>(GetWindowLongPtrW(window, GWLP_USERDATA));
}

LRESULT CALLBACK window_proc(HWND window, UINT message, WPARAM wparam, LPARAM lparam) {
    auto* owner = self(window);
    if (message == WM_NCCREATE) {
        const auto* create = reinterpret_cast<const CREATESTRUCTW*>(lparam);
        owner = static_cast<NativeWindow*>(create->lpCreateParams);
        SetWindowLongPtrW(window, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(owner));
    }
    if (owner != nullptr) {
        switch (message) {
        case WM_CLOSE:
            DestroyWindow(window);
            return 0;
        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;
        default:
            break;
        }
    }
    return DefWindowProcW(window, message, wparam, lparam);
}

} // namespace

NativeWindow::NativeWindow(std::uint32_t logical_width,
                           std::uint32_t logical_height,
                           std::uint32_t scale) noexcept
    : logical_width_(logical_width), logical_height_(logical_height),
      scale_(std::max(1U, scale)) {}

NativeWindow::~NativeWindow() {
    if (window_handle_ != nullptr) DestroyWindow(static_cast<HWND>(window_handle_));
}

bool NativeWindow::open(std::string_view title) noexcept {
    const auto instance = GetModuleHandleW(nullptr);
    WNDCLASSW window_class{};
    window_class.hInstance = instance;
    window_class.lpfnWndProc = window_proc;
    window_class.lpszClassName = kWindowClass;
    window_class.hCursor = LoadCursorW(nullptr, MAKEINTRESOURCEW(32512));
    if (RegisterClassW(&window_class) == 0 && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    std::wstring wide_title(title.begin(), title.end());
    RECT client{0, 0, static_cast<LONG>(logical_width_ * scale_),
                static_cast<LONG>(logical_height_ * scale_)};
    AdjustWindowRect(&client, WS_OVERLAPPEDWINDOW, FALSE);
    auto* window = CreateWindowExW(
        0, kWindowClass, wide_title.c_str(), WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, client.right - client.left,
        client.bottom - client.top, nullptr, nullptr, instance, this);
    if (window == nullptr) return false;
    window_handle_ = window;
    open_ = true;
    ShowWindow(window, SW_SHOW);
    UpdateWindow(window);
    return true;
}

bool NativeWindow::process_events() noexcept {
    MSG message{};
    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE) != 0) {
        if (message.message == WM_QUIT) {
            open_ = false;
            controller_ = {};
            return false;
        }
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }
    return open_;
}

void NativeWindow::present(std::span<const std::uint32_t> pixels) noexcept {
    if (!open_ || pixels.size() < static_cast<std::size_t>(logical_width_) * logical_height_) return;
    const auto window = static_cast<HWND>(window_handle_);

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = static_cast<LONG>(logical_width_);
    info.bmiHeader.biHeight = -static_cast<LONG>(logical_height_);
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    const auto client_dc = GetDC(window);
    if (client_dc != nullptr) {
        RECT client{};
        GetClientRect(window, &client);
        StretchDIBits(client_dc, 0, 0, client.right, client.bottom, 0, 0,
                      static_cast<int>(logical_width_), static_cast<int>(logical_height_),
                      pixels.data(), &info, DIB_RGB_COLORS, SRCCOPY);
        ReleaseDC(window, client_dc);
    }
}

bool NativeWindow::backend_available() const noexcept { return true; }

core::ControllerState NativeWindow::poll_controller() const noexcept {
    core::ControllerState result{};
    if (GetForegroundWindow() != static_cast<HWND>(window_handle_)) return result;
    if ((GetAsyncKeyState(VK_UP) & 0x8000) != 0) result.set(core::Button::Up);
    if ((GetAsyncKeyState(VK_DOWN) & 0x8000) != 0) result.set(core::Button::Down);
    if ((GetAsyncKeyState(VK_LEFT) & 0x8000) != 0) result.set(core::Button::Left);
    if ((GetAsyncKeyState(VK_RIGHT) & 0x8000) != 0) result.set(core::Button::Right);
    return result;
}

} // namespace oasis::platform

#else

namespace oasis::platform {

NativeWindow::NativeWindow(std::uint32_t logical_width,
                           std::uint32_t logical_height,
                           std::uint32_t scale) noexcept
    : logical_width_(logical_width), logical_height_(logical_height), scale_(scale) {}

NativeWindow::~NativeWindow() = default;

bool NativeWindow::open(std::string_view) noexcept { return false; }
bool NativeWindow::process_events() noexcept { return false; }
void NativeWindow::present(std::span<const std::uint32_t>) noexcept {}
bool NativeWindow::backend_available() const noexcept { return false; }
core::ControllerState NativeWindow::poll_controller() const noexcept { return {}; }

} // namespace oasis::platform

#endif
