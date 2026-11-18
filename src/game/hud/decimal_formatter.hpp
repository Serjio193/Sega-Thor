#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::hud {

/// Configuration and constants for ROM Fixed-Width & Space-Padded Decimal String Formatters
/// (0x00C0BE..0x00C17A, 188 bytes).
struct DecimalFormatterConfig {
    static constexpr std::uint32_t kFormatDecFixed5Addr = 0x00C0BEU;
    static constexpr std::uint32_t kFormatDecSpacesAddr = 0x00C114U;
    static constexpr std::uint32_t kExitRtsFixed5Addr = 0x00C112U;
    static constexpr std::uint32_t kExitRtsSpacesAddr = 0x00C178U;
    static constexpr std::uint32_t kRangeEndAddr = 0x00C17AU;
    static constexpr std::size_t kRoutineSizeBytes = 188U;

    // RAM Buffer ($FF192A..$FF192E)
    static constexpr std::uint32_t kDecimalBufferRam = 0x00FF192AU;
    static constexpr std::size_t kBufferLen = 5U;

    // Divisors
    static constexpr std::uint16_t kDivisor10000 = 10000U;
    static constexpr std::uint16_t kDivisor1000 = 1000U;
    static constexpr std::uint16_t kDivisor100 = 100U;
    static constexpr std::uint16_t kDivisor10 = 10U;

    // Characters
    static constexpr char kZeroChar = '0';  // 0x30
    static constexpr char kSpaceChar = ' '; // 0x20
};

using DecimalString5 = std::array<char, DecimalFormatterConfig::kBufferLen>;

/// Formats 16-bit unsigned integer into 5 zero-padded ASCII digits ("00000".."65535") (0x00C0BE).
[[nodiscard]] constexpr DecimalString5 format_dec_fixed5(std::uint16_t val) noexcept {
    DecimalString5 out{};
    std::uint32_t d0 = val;

    // 10000s stage (0x00C0CE)
    const std::uint16_t q10k = static_cast<std::uint16_t>(d0 / 10000U);
    const std::uint16_t rem10k = static_cast<std::uint16_t>(d0 % 10000U);
    out[0] = static_cast<char>('0' + (q10k & 0xFFU));

    // 1000s stage (0x00C0DC)
    d0 = rem10k;
    const std::uint16_t q1k = static_cast<std::uint16_t>(d0 / 1000U);
    const std::uint16_t rem1k = static_cast<std::uint16_t>(d0 % 1000U);
    out[1] = static_cast<char>('0' + (q1k & 0xFFU));

    // 100s stage (0x00C0EA)
    d0 = rem1k;
    const std::uint16_t q100 = static_cast<std::uint16_t>(d0 / 100U);
    const std::uint16_t rem100 = static_cast<std::uint16_t>(d0 % 100U);
    out[2] = static_cast<char>('0' + (q100 & 0xFFU));

    // 10s stage (0x00C0F8)
    d0 = rem100;
    const std::uint16_t q10 = static_cast<std::uint16_t>(d0 / 10U);
    const std::uint16_t rem10 = static_cast<std::uint16_t>(d0 % 10U);
    out[3] = static_cast<char>('0' + (q10 & 0xFFU));

    // Units stage (0x00C102..0x00C108)
    out[4] = static_cast<char>('0' + (rem10 & 0xFFU));

    return out;
}

/// Formats 16-bit unsigned integer into 5 space-padded ASCII chars ("    0".."65535") (0x00C114).
[[nodiscard]] constexpr DecimalString5 format_dec_spaces(std::uint16_t val) noexcept {
    DecimalString5 out{};
    std::uint32_t d0 = val;

    // 10000s stage (0x00C124..0x00C12E)
    const std::uint16_t q10k = static_cast<std::uint16_t>(d0 / 10000U);
    const std::uint16_t rem10k = static_cast<std::uint16_t>(d0 % 10000U);
    if ((q10k & 0xFFU) != 0U) {
        out[0] = static_cast<char>('0' + (q10k & 0xFFU));
        d0 = rem10k;
        out[1] = static_cast<char>('0' + ((d0 / 1000U) & 0xFFU));
        d0 = d0 % 1000U;
        out[2] = static_cast<char>('0' + ((d0 / 100U) & 0xFFU));
        d0 = d0 % 100U;
        out[3] = static_cast<char>('0' + ((d0 / 10U) & 0xFFU));
        out[4] = static_cast<char>('0' + ((d0 % 10U) & 0xFFU));
        return out;
    }
    out[0] = ' ';

    // 1000s stage (0x00C136..0x00C140)
    d0 = rem10k;
    const std::uint16_t q1k = static_cast<std::uint16_t>(d0 / 1000U);
    const std::uint16_t rem1k = static_cast<std::uint16_t>(d0 % 1000U);
    if ((q1k & 0xFFU) != 0U) {
        out[1] = static_cast<char>('0' + (q1k & 0xFFU));
        d0 = rem1k;
        out[2] = static_cast<char>('0' + ((d0 / 100U) & 0xFFU));
        d0 = d0 % 100U;
        out[3] = static_cast<char>('0' + ((d0 / 10U) & 0xFFU));
        out[4] = static_cast<char>('0' + ((d0 % 10U) & 0xFFU));
        return out;
    }
    out[1] = ' ';

    // 100s stage (0x00C148..0x00C152)
    d0 = rem1k;
    const std::uint16_t q100 = static_cast<std::uint16_t>(d0 / 100U);
    const std::uint16_t rem100 = static_cast<std::uint16_t>(d0 % 100U);
    if ((q100 & 0xFFU) != 0U) {
        out[2] = static_cast<char>('0' + (q100 & 0xFFU));
        d0 = rem100;
        out[3] = static_cast<char>('0' + ((d0 / 10U) & 0xFFU));
        out[4] = static_cast<char>('0' + ((d0 % 10U) & 0xFFU));
        return out;
    }
    out[2] = ' ';

    // 10s stage (0x00C15A..0x00C164)
    d0 = rem100;
    const std::uint16_t q10 = static_cast<std::uint16_t>(d0 / 10U);
    const std::uint16_t rem10 = static_cast<std::uint16_t>(d0 % 10U);
    if ((q10 & 0xFFU) != 0U) {
        out[3] = static_cast<char>('0' + (q10 & 0xFFU));
        out[4] = static_cast<char>('0' + (rem10 & 0xFFU));
        return out;
    }
    out[3] = ' ';

    // Units stage: always outputs decimal digit unconditionally (0x00C168..0x00C16E)
    out[4] = static_cast<char>('0' + (rem10 & 0xFFU));
    return out;
}

/// Static utility wrapper.
class DecimalFormatter {
public:
    static constexpr std::size_t kBufferLen = DecimalFormatterConfig::kBufferLen;

    [[nodiscard]] static constexpr DecimalString5 format_fixed5(std::uint16_t val) noexcept {
        return format_dec_fixed5(val);
    }

    [[nodiscard]] static constexpr DecimalString5 format_spaces(std::uint16_t val) noexcept {
        return format_dec_spaces(val);
    }
};

// ============================================================================
// Raw 64KB RAM Window Integration Helpers ($FF192A..$FF192E)
// ============================================================================

void format_dec_fixed5_raw(std::span<std::uint8_t> ram, std::uint16_t val);
void format_dec_spaces_raw(std::span<std::uint8_t> ram, std::uint16_t val);
[[nodiscard]] DecimalString5 read_decimal_buffer_raw(std::span<const std::uint8_t> ram);

} // namespace oasis::game::hud
