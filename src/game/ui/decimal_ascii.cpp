#include "game/ui/decimal_ascii.hpp"

namespace oasis::game::ui {

namespace {

constexpr std::array<std::uint16_t, kDecimalDigitCount> kDivisors = {
    10000U, 1000U, 100U, 10U, 1U
};

[[nodiscard]] std::array<char, kDecimalDigitCount> format_u16_dec(
    std::uint16_t val,
    bool space_pad) noexcept {
    std::array<char, kDecimalDigitCount> out{};
    std::uint16_t rem = val;
    bool nonzero_seen = false;

    for (std::size_t i = 0U; i < kDecimalDigitCount; ++i) {
        const std::uint16_t div = kDivisors[i];
        const std::uint16_t digit = static_cast<std::uint16_t>(rem / div);
        rem = static_cast<std::uint16_t>(rem % div);

        if (space_pad && !nonzero_seen && digit == 0U && (i + 1U) < kDecimalDigitCount) {
            out[i] = ' ';
        } else {
            nonzero_seen = true;
            out[i] = static_cast<char>('0' + digit);
        }
    }
    return out;
}

} // namespace

std::array<char, kDecimalDigitCount> format_u16_dec_zero_padded(
    std::uint16_t val) noexcept {
    return format_u16_dec(val, false);
}

std::array<char, kDecimalDigitCount> format_u16_dec_space_padded(
    std::uint16_t val) noexcept {
    return format_u16_dec(val, true);
}

} // namespace oasis::game::ui
