#pragma once

#include <cstdint>

namespace oasis::game::entities {

inline constexpr std::uint32_t kPrngSeedRamAddr = 0xFF1928U;
inline constexpr std::uint16_t kPrngDefaultSeed = 0x7F28U;
inline constexpr std::uint32_t kNextRng16Addr = 0x00E268U;
inline constexpr std::uint32_t kRandU16Addr = 0x00E2A2U;
inline constexpr std::uint32_t kRandU8Addr = 0x00E2BAU;

/// Deterministic 16-bit LFSR PRNG ($FF1928, initial seed 0x7F28).
class PrngLfsr16 {
public:
    explicit constexpr PrngLfsr16(std::uint16_t seed = kPrngDefaultSeed) noexcept
        : state_(seed) {}

    [[nodiscard]] constexpr std::uint16_t state() const noexcept { return state_; }
    constexpr void reset(std::uint16_t seed = kPrngDefaultSeed) noexcept { state_ = seed; }

    /// Models 0x00E268: runs 4 LFSR shift-tap iterations on state_ and returns it.
    [[nodiscard]] std::uint16_t next_rng16() noexcept;

    /// Models 0x00E2A2: returns 0 if bound == 0, else (next_rng16() * bound) >> 16.
    [[nodiscard]] std::uint16_t rand_u16(std::uint16_t bound) noexcept;

    /// Models 0x00E2BA: same as rand_u16 with bound & 0xFF.
    [[nodiscard]] std::uint8_t rand_u8(std::uint8_t bound) noexcept;

private:
    std::uint16_t state_{kPrngDefaultSeed};
};

} // namespace oasis::game::entities
