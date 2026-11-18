#include "game/hud/decimal_formatter.hpp"

namespace oasis::game::hud {

namespace {

[[nodiscard]] inline std::size_t to_ram_offset(std::uint32_t addr) noexcept {
    if (addr >= 0x00FF0000U && addr <= 0x00FFFFFFU) {
        return static_cast<std::size_t>(addr - 0x00FF0000U);
    }
    return static_cast<std::size_t>(addr & 0xFFFFU);
}

} // namespace

void format_dec_fixed5_raw(std::span<std::uint8_t> ram, std::uint16_t val) {
    const std::size_t off = to_ram_offset(DecimalFormatterConfig::kDecimalBufferRam);
    if (off + DecimalFormatterConfig::kBufferLen > ram.size()) {
        return;
    }

    const DecimalString5 formatted = format_dec_fixed5(val);
    for (std::size_t i = 0U; i < DecimalFormatterConfig::kBufferLen; ++i) {
        ram[off + i] = static_cast<std::uint8_t>(formatted[i]);
    }
}

void format_dec_spaces_raw(std::span<std::uint8_t> ram, std::uint16_t val) {
    const std::size_t off = to_ram_offset(DecimalFormatterConfig::kDecimalBufferRam);
    if (off + DecimalFormatterConfig::kBufferLen > ram.size()) {
        return;
    }

    const DecimalString5 formatted = format_dec_spaces(val);
    for (std::size_t i = 0U; i < DecimalFormatterConfig::kBufferLen; ++i) {
        ram[off + i] = static_cast<std::uint8_t>(formatted[i]);
    }
}

DecimalString5 read_decimal_buffer_raw(std::span<const std::uint8_t> ram) {
    const std::size_t off = to_ram_offset(DecimalFormatterConfig::kDecimalBufferRam);
    DecimalString5 out{};
    if (off + DecimalFormatterConfig::kBufferLen > ram.size()) {
        out.fill(' ');
        return out;
    }

    for (std::size_t i = 0U; i < DecimalFormatterConfig::kBufferLen; ++i) {
        out[i] = static_cast<char>(ram[off + i]);
    }
    return out;
}

} // namespace oasis::game::hud
