#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace oasis::game::ui {

inline constexpr std::uint32_t kFormatU16DecZeroPaddedAddr = 0x00C0BEU;
inline constexpr std::uint32_t kFormatU16DecSpacePaddedAddr = 0x00C114U;
inline constexpr std::uint32_t kDecimalBufRamAddr = 0x00FF192AU;
inline constexpr std::size_t kDecimalDigitCount = 5U;

/// Models 0x00C0BE..0x00C112: formats val into 5 zero-padded ASCII digits
/// ("00000".."65535") matching $FF192A.
[[nodiscard]] std::array<char, kDecimalDigitCount> format_u16_dec_zero_padded(
    std::uint16_t val) noexcept;

/// Models 0x00C114..0x00C178: formats val into 5 space-padded ASCII chars
/// ("    0".."65535") matching $FF192A.
[[nodiscard]] std::array<char, kDecimalDigitCount> format_u16_dec_space_padded(
    std::uint16_t val) noexcept;

} // namespace oasis::game::ui
