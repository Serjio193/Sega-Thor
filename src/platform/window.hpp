#pragma once

#include "core/runtime.hpp"

#include <cstdint>
#include <span>
#include <string_view>

namespace oasis::platform {

class NativeWindow {
public:
    NativeWindow(std::uint32_t logical_width, std::uint32_t logical_height,
                 std::uint32_t scale = 3) noexcept;
    ~NativeWindow();

    NativeWindow(const NativeWindow&) = delete;
    NativeWindow& operator=(const NativeWindow&) = delete;

    [[nodiscard]] bool open(std::string_view title) noexcept;
    [[nodiscard]] bool process_events() noexcept;
    void present(std::span<const std::uint32_t> pixels) noexcept;

    [[nodiscard]] bool is_open() const noexcept { return open_; }
    [[nodiscard]] bool backend_available() const noexcept;
    [[nodiscard]] core::ControllerState poll_controller() const noexcept;

private:
    std::uint32_t logical_width_{};
    std::uint32_t logical_height_{};
    std::uint32_t scale_{};
    bool open_{};
    core::ControllerState controller_{};
    void* window_handle_{};
};

} // namespace oasis::platform
