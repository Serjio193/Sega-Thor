#pragma once

#include "game/entity/native_entity.hpp"

#include <cstdint>
#include <span>

namespace oasis::game::entity {

inline constexpr std::uint32_t kAnimTable002C = 0x042E04U;
inline constexpr std::uint32_t kAnimTable0060 = 0x040E70U;
inline constexpr std::uint32_t kAnimTable0002 = 0x03FF7AU;

// Exact ROM 0x00E268 4-step 16-bit LFSR PRNG on $FF1928.
[[nodiscard]] std::uint16_t step_prng(std::uint16_t& seed) noexcept;

// Exact ROM 0x00E2A2 bounded PRNG: ((step_prng(seed) * bound) >> 16).
[[nodiscard]] std::uint16_t prng_mod(std::uint16_t& seed, std::uint16_t bound) noexcept;

// ROM 0x008EF8 animation reset.
void init_entity_animation(NativeEntity& ent, std::uint16_t anim_id) noexcept;

// ROM 0x008FA4..0x00903E authentic ROM animation table stepper.
// Reads (word frame_id, word flags_duration) pairs directly from `rom` at `anim_table_addr`.
void step_entity_animation(NativeEntity& ent,
                           std::span<const std::uint8_t> rom,
                           std::uint32_t anim_table_addr) noexcept;

} // namespace oasis::game::entity