#pragma once

#include "genesis/vdp_types.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::genesis {

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

    void write_vram_word(std::size_t address, std::uint16_t value);
    void write_cram_word(std::size_t address, std::uint16_t value);
    void write_vsram_word(std::size_t address, std::uint16_t value);

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
