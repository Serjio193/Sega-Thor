#pragma once

#include "game/save/sram_save_system.hpp"
#include "game/spirits/all_spirits_registry.hpp"
#include "game/spirits/dytto_controller.hpp"
#include "genesis/vdp_state.hpp"

#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::items {

// ROM 0x005236..0x005470: Consumable item IDs (0x1A..0x2A) & jump table 0x00532C.
struct ItemConfig {
    static constexpr std::uint32_t kEligibilityRoutineRom = 0x00005236U;
    static constexpr std::uint32_t kDispatcherRoutineRom = 0x0000530CU;
    static constexpr std::uint32_t kJumpTableRom = 0x0000532CU;
    static constexpr std::uint32_t kAttributeTableRom = 0x0005CE16U;
    static constexpr std::uint16_t kBlockedAreaId = 0x0010U;
    static constexpr std::uint32_t kSfxHeal = 0x00A00234U;
    static constexpr std::uint32_t kSfxConsume = 0x00A00256U;
    static constexpr std::uint8_t kSummonTimer = 0x14U;
};

struct ItemUseContext {
    std::uint16_t area_id{0};           // $FF0E40 (area 0x10 blocks item usage)
    bool summon_locked{false};          // $FF16F1:0 != 0
    bool spirit_active{false};          // $FF1AA4 != 0
    std::uint8_t unlocked_spirit_mask{0}; // $FF0D7A bits 0..3 (Dytto, Efreet, Shade, Bow)
    std::int16_t hp{0};                 // $FF1A72
    std::int16_t max_hp{100};           // $FF1A70
    std::int16_t sp{0};                 // $FF1A76
    std::int16_t max_sp{100};           // $FF1A74
};

struct ItemEffectResult {
    bool applied{false};
    bool consumed{false};
    bool summon_requested{false};
    std::uint8_t summon_spirit_index{0}; // 0=Dytto, 1=Efreet, 2=Shade, 3=Bow
    std::uint8_t summon_timer{0};        // 0x14 for spirit rings
    std::uint32_t sound_cmd{0};          // 0xA00234 on heal
    std::int16_t hp_delta{0};
    std::int16_t sp_delta{0};
};

// ROM 0x005236: evaluates whether `item_id` (0x1A..0x2A) can be used in `ctx`.
[[nodiscard]] bool can_use_item(std::uint8_t item_id, const ItemUseContext& ctx) noexcept;

// ROM 0x00530C / 0x00532C: applies item effect for `item_id` (0x1A..0x2A) to `ctx`.
ItemEffectResult apply_item_effect(std::uint8_t item_id, ItemUseContext& ctx) noexcept;

// Evaluates eligibility, applies effect, consumes non-ring consumables (0x1E..0x2A)
// in `wram` slot `slot_index`, and summons any of the 4 spirits (0x1A..0x1D) via `spirit_ctrl`.
ItemEffectResult use_inventory_slot(
    save::PersistentWramBlock& wram,
    std::size_t slot_index,
    ItemUseContext& ctx,
    spirits::ActiveSpiritController& spirit_ctrl,
    std::int32_t spawn_x = 0,
    std::int32_t spawn_y = 0,
    std::span<const std::uint8_t> rom = {},
    genesis::VdpState* vdp = nullptr) noexcept;

// Evaluates eligibility, applies effect, consumes non-ring consumables (0x1E..0x2A)
// in `wram` slot `slot_index`, and spawns Dytto if Dytto Ring (0x1A) is used.
ItemEffectResult use_inventory_slot(
    save::PersistentWramBlock& wram,
    std::size_t slot_index,
    ItemUseContext& ctx,
    spirits::DyttoSpiritController* dytto = nullptr,
    std::int32_t spawn_x = 0,
    std::int32_t spawn_y = 0) noexcept;

} // namespace oasis::game::items
