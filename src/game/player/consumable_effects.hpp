#pragma once

#include "game/items/item_effects.hpp"

#include <cstdint>

namespace oasis::game::player {

inline constexpr std::uint32_t kCanUseConsumableAddr = 0x005236U;
inline constexpr std::uint32_t kApplyConsumableAddr = 0x00530CU;
inline constexpr std::uint32_t kConsumableJumpTableAddr = 0x00532CU;
inline constexpr std::uint32_t kSummonSpiritRoutineAddr = 0x007E9EU;

inline constexpr std::uint16_t kConsumableBlockedAreaId = 0x0010U;
inline constexpr std::uint8_t kConsumableItemMin = 0x1AU;
inline constexpr std::uint8_t kConsumableItemMax = 0x2AU;
inline constexpr std::uint32_t kConsumableHealSfxCmd = 0x00A00234U;

using ConsumableContext = items::ItemUseContext;
using ConsumableResult = items::ItemEffectResult;

/// Models 0x005236..0x00530A: validates Area 0x10 block, Spirit Gems
/// (0x1A..0x1D), HP Foods (0x1E..0x21), SP Foods (0x22..0x25), and Elixirs (0x26..0x2A).
[[nodiscard]] bool can_use_consumable(
    std::uint8_t item_id,
    const ConsumableContext& ctx) noexcept;

/// Models 0x00530C..0x005472: applies 25%/50%/75%/100% HP/SP restore tiers
/// clamped to Max, Spirit Gem SP refill + summon trigger, and heal SFX 0x00A00234.
ConsumableResult apply_consumable_effect(
    std::uint8_t item_id,
    ConsumableContext& ctx) noexcept;

} // namespace oasis::game::player
