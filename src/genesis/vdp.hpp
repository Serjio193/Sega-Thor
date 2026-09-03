#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::genesis {

class Vdp {
public:
    static constexpr std::size_t kVramSize = 64 * 1024;

    void write_control(std::uint32_t value) noexcept { control_ = value; }
    [[nodiscard]] std::uint32_t control() const noexcept { return control_; }

    void write_vram(std::size_t address, std::span<const std::uint8_t> data);
    [[nodiscard]] const std::array<std::uint8_t, kVramSize>& vram() const noexcept { return vram_; }

private:
    std::uint32_t control_{};
    std::array<std::uint8_t, kVramSize> vram_{};
};

} // namespace oasis::genesis
