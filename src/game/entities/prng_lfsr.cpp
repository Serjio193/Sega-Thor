#include "game/entities/prng_lfsr.hpp"

namespace oasis::game::entities {

std::uint16_t PrngLfsr16::next_rng16() noexcept {
    std::uint32_t d0 = state_;
    for (int i = 0; i < 4; ++i) {
        const std::uint32_t rol16 = ((d0 << 2U) | (d0 >> 14U)) & 0xFFFFU;
        const std::uint32_t tap = (rol16 ^ d0) & 1U;
        d0 = ((d0 << 1U) | tap) & 0xFFFFU;
    }
    state_ = static_cast<std::uint16_t>(d0);
    return state_;
}

std::uint16_t PrngLfsr16::rand_u16(std::uint16_t bound) noexcept {
    if (bound == 0U) {
        return 0U;
    }
    const std::uint32_t r = next_rng16();
    return static_cast<std::uint16_t>((r * static_cast<std::uint32_t>(bound)) >> 16U);
}

std::uint8_t PrngLfsr16::rand_u8(std::uint8_t bound) noexcept {
    return static_cast<std::uint8_t>(rand_u16(static_cast<std::uint16_t>(bound & 0xFFU)));
}

} // namespace oasis::game::entities
