#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::genesis {

struct TileAttributes {
    std::uint16_t tile_index{};
    std::uint8_t palette{};
    bool priority{};
    bool flip_h{};
    bool flip_v{};
};

[[nodiscard]] constexpr TileAttributes decode_tile_attributes(std::uint16_t value) noexcept {
    return {
        static_cast<std::uint16_t>(value & 0x07FFU),
        static_cast<std::uint8_t>((value >> 13U) & 0x03U),
        (value & 0x8000U) != 0,
        (value & 0x0800U) != 0,
        (value & 0x1000U) != 0,
    };
}

class Vdp {
public:
    static constexpr std::size_t kVramSize = 64 * 1024;
    static constexpr std::size_t kCramSize = 128;
    static constexpr std::size_t kVsramSize = 80;

    void write_control(std::uint32_t value) noexcept { control_ = value; }
    [[nodiscard]] std::uint32_t control() const noexcept { return control_; }

    void write_vram(std::size_t address, std::span<const std::uint8_t> data);
    void write_cram(std::size_t address, std::span<const std::uint8_t> data);
    void write_vsram(std::size_t address, std::span<const std::uint8_t> data);

    [[nodiscard]] std::uint16_t read_vram_word(std::size_t address) const;
    [[nodiscard]] std::uint16_t read_cram_word(std::size_t address) const;
    [[nodiscard]] std::uint16_t read_vsram_word(std::size_t address) const;

    [[nodiscard]] const std::array<std::uint8_t, kVramSize>& vram() const noexcept { return vram_; }
    [[nodiscard]] const std::array<std::uint8_t, kCramSize>& cram() const noexcept { return cram_; }
    [[nodiscard]] const std::array<std::uint8_t, kVsramSize>& vsram() const noexcept { return vsram_; }

private:
    std::uint32_t control_{};
    std::array<std::uint8_t, kVramSize> vram_{};
    std::array<std::uint8_t, kCramSize> cram_{};
    std::array<std::uint8_t, kVsramSize> vsram_{};
};

} // namespace oasis::genesis
