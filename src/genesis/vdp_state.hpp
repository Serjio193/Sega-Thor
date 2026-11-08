#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>
#include <string_view>

namespace oasis::genesis {

struct VdpState {
    std::array<std::uint8_t, 65536> vram{};
    std::array<std::uint8_t, 128> cram{};
    std::array<std::uint8_t, 80> vsram{};
    std::array<std::uint8_t, 32> registers{};

    [[nodiscard]] bool is_display_enabled() const noexcept;
    [[nodiscard]] bool is_h40() const noexcept;
    [[nodiscard]] bool is_v30() const noexcept;

    [[nodiscard]] int display_width() const noexcept;
    [[nodiscard]] int display_height() const noexcept;

    [[nodiscard]] std::uint32_t plane_a_base() const noexcept;
    [[nodiscard]] std::uint32_t plane_b_base() const noexcept;
    [[nodiscard]] std::uint32_t window_base() const noexcept;
    [[nodiscard]] std::uint32_t sat_base() const noexcept;
    [[nodiscard]] std::uint32_t hscroll_base() const noexcept;

    [[nodiscard]] int plane_width_cells() const noexcept;
    [[nodiscard]] int plane_height_cells() const noexcept;

    [[nodiscard]] std::uint8_t backdrop_cram_index() const noexcept;
    [[nodiscard]] int hscroll_mode() const noexcept;
    [[nodiscard]] int vscroll_mode() const noexcept;

    [[nodiscard]] bool load_from_json(std::string_view json_text);
    [[nodiscard]] bool load_from_file(const char* filepath);
};

} // namespace oasis::genesis
