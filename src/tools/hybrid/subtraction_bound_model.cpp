#include "tools/hybrid/subtraction_bound_model.hpp"

namespace oasis::hybrid {
namespace {
constexpr std::uint16_t x_flag = 0x10U;

std::uint16_t arithmetic_flags(std::uint16_t lhs, std::uint16_t rhs,
                               std::uint16_t result, bool subtract,
                               std::uint16_t preserved_x,
                               bool update_x = false) noexcept {
    const bool negative = (result & 0x8000U) != 0U;
    const bool zero = result == 0U;
    const bool carry = subtract ? lhs < rhs : false;
    const bool overflow = subtract && ((lhs ^ rhs) & (lhs ^ result) & 0x8000U) != 0U;
    const auto x = update_x ? (carry ? x_flag : 0U) : preserved_x;
    return static_cast<std::uint16_t>(x |
        (negative ? 0x08U : 0U) | (zero ? 0x04U : 0U) |
        (overflow ? 0x02U : 0U) | (carry ? 0x01U : 0U));
}

} // namespace

SubtractionBoundResult apply_subtraction_bound(
    const SubtractionBoundInput& input) noexcept {
    auto d0 = input.d0;
    auto d3 = (input.d3 & 0xFFFF0000U) |
              static_cast<std::uint16_t>(input.d1 - input.limit);
    const auto d3_low = static_cast<std::uint16_t>(d3);
    auto x = static_cast<std::uint16_t>(input.sr & x_flag);
    auto sr = arithmetic_flags(static_cast<std::uint16_t>(input.d1), input.limit,
                               d3_low, true, x, true);
    x = static_cast<std::uint16_t>(sr & x_flag);
    auto marker = input.marker;
    bool wrote = false;

    if ((sr & 0x08U) != 0U) {
        d0 &= 0xFFFF0000U;
        sr = static_cast<std::uint16_t>(x | 0x04U); // CLR.W D0
        marker = static_cast<std::uint8_t>(marker | 0x48U);
        sr = arithmetic_flags(marker, 0U, marker, false, x); // ORI.B
        wrote = true;
    } else {
        const auto d0_low = static_cast<std::uint16_t>(d0);
        const auto compare = static_cast<std::uint16_t>(d0_low - d3_low);
        sr = arithmetic_flags(d0_low, d3_low, compare, true, x); // CMP.W D3,D0
        const bool below_or_equal = (sr & 0x03U) != 0U;
        if (!below_or_equal) {
            d0 = (d0 & 0xFFFF0000U) | d3_low;
            sr = arithmetic_flags(d3_low, 0U, d3_low, false, x); // MOVE.W D3,D0
            if (d3_low == 0U) {
                marker = static_cast<std::uint8_t>(marker | 0x48U);
                sr = arithmetic_flags(marker, 0U, marker, false, x);
                wrote = true;
            }
        }
    }
    return {d0, d3, marker, static_cast<std::uint16_t>((input.sr & 0xFFE0U) | sr), wrote};
}

} // namespace oasis::hybrid
