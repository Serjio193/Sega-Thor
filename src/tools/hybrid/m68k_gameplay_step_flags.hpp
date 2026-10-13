#pragma once

#include <cstdint>

namespace oasis::hybrid::step_flags {

inline std::uint16_t logic_flags(std::uint16_t value, std::uint16_t old) noexcept {
    return static_cast<std::uint16_t>((old & 0x10U) | (value == 0 ? 4U : 0U) |
        ((value & 0x8000U) ? 8U : 0U));
}

inline std::uint16_t byte_logic_flags(std::uint8_t value, std::uint16_t old) noexcept {
    return static_cast<std::uint16_t>((old & 0x10U) | (value == 0 ? 4U : 0U) |
        ((value & 0x80U) ? 8U : 0U));
}

inline std::uint16_t sub_flags(std::uint16_t lhs, std::uint16_t rhs,
                               std::uint16_t result, std::uint16_t old,
                               bool affect_x) noexcept {
    const bool carry = lhs < rhs;
    const bool overflow = ((lhs ^ rhs) & (lhs ^ result) & 0x8000U) != 0;
    return static_cast<std::uint16_t>((affect_x ? (carry ? 0x10U : 0U) : old & 0x10U) |
        (result == 0 ? 4U : 0U) | ((result & 0x8000U) ? 8U : 0U) |
        (overflow ? 2U : 0U) | (carry ? 1U : 0U));
}

inline std::uint16_t add_flags(std::uint16_t lhs, std::uint16_t rhs,
                               std::uint16_t result, std::uint16_t old,
                               bool affect_x) noexcept {
    const bool carry = static_cast<std::uint32_t>(lhs) + rhs > 0xFFFFU;
    const bool overflow = ((~(lhs ^ rhs)) & (lhs ^ result) & 0x8000U) != 0;
    return static_cast<std::uint16_t>((affect_x ? (carry ? 0x10U : 0U) : old & 0x10U) |
        (result == 0 ? 4U : 0U) | ((result & 0x8000U) ? 8U : 0U) |
        (overflow ? 2U : 0U) | (carry ? 1U : 0U));
}

inline bool condition(unsigned cc, std::uint16_t sr) noexcept {
    const bool n = (sr & 8U) != 0, z = (sr & 4U) != 0;
    const bool v = (sr & 2U) != 0, c = (sr & 1U) != 0;
    switch (cc) {
    case 2: return !c && !z;
    case 3: return c || z;
    case 4: return !c;
    case 5: return c;
    case 6: return !z;
    case 7: return z;
    case 8: return !v;
    case 9: return v;
    case 10: return !n;
    case 11: return n;
    case 12: return n == v;
    case 13: return n != v;
    case 14: return !z && n == v;
    case 15: return z || n != v;
    default: return false;
    }
}

} // namespace oasis::hybrid::step_flags
