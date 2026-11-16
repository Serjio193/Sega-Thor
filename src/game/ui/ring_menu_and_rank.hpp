#pragma once

#include "game/save/sram_save_system.hpp"

#include <array>
#include <cstddef>
#include <cstdint>

namespace oasis::game::ui {

// ROM 0x0042E8..0x0043AC & 0x00155D76: Pause Ring Menu configuration.
struct RingMenuConfig {
    static constexpr std::uint32_t kWeaponRingRendererRom = 0x000042E8U;
    static constexpr std::uint32_t kItemRingRendererRom = 0x00004388U;
    static constexpr std::uint32_t kIconTilesetRomAddr = 0x00155D76U;
    static constexpr std::uint8_t kTilesPerIcon = 9U; // 3x3
    static constexpr std::size_t kWeaponSlotCount = 8U;
    static constexpr std::size_t kItemSlotCount = 16U;
    static constexpr int kSlotSpacingPx = 24;
};

struct RingMenuState {
    bool open{false};
    std::uint8_t weapon_cursor{0U}; // $FF0E50 (0..7)
    std::uint8_t item_cursor{0U};   // $FF0E51 (0..15)
    std::array<std::uint16_t, RingMenuConfig::kWeaponSlotCount> weapon_slots{}; // $FF0D80
    std::array<std::uint16_t, RingMenuConfig::kItemSlotCount> item_slots{};     // $FF0D90

    void step_weapon_cursor(int delta) noexcept;
    void step_item_cursor(int delta) noexcept;
    [[nodiscard]] static constexpr int slot_x_offset_px(std::size_t slot_index) noexcept {
        return static_cast<int>(slot_index) * RingMenuConfig::kSlotSpacingPx;
    }

    void sync_from_wram(const save::PersistentWramBlock& wram) noexcept;
};

// ROM 0x007BF6..0x007C1C: Player Rank, MaxHP scaling, SP pickup & Gem counters.
struct PlayerRankStats {
    static constexpr std::uint16_t kMaxRank = 200U;   // 0xC8 ($FF1A7E cap)
    static constexpr std::int16_t kBaseMaxHp = 198;   // 0xC6
    static constexpr std::int16_t kSpPickupStep = 5;  // 0x007BCA

    std::uint16_t rank{0U};    // $FF1A7E
    std::int16_t max_hp{198};  // $FF1A70
    std::int16_t hp{198};      // $FF1A72
    std::int16_t max_sp{100};  // $FF1A74
    std::int16_t sp{100};      // $FF1A76
    std::array<std::uint8_t, 4> gem_counters{}; // $FF0DB2 + (item_id - 0x0D)

    // ROM 0x007BF6: MaxHP = 198 + 2 * min(rank, 200)
    [[nodiscard]] static constexpr std::int16_t max_hp_for_rank(std::uint16_t r) noexcept {
        const std::uint16_t clamped = (r > kMaxRank) ? kMaxRank : r;
        return static_cast<std::int16_t>(kBaseMaxHp + 2 * static_cast<int>(clamped));
    }

    // Increments rank (up to 200), recomputes max_hp, and adds +2 to current hp (clamped).
    bool apply_rank_up() noexcept;

    // ROM 0x007BCA: adds +5 SP clamped to max_sp; returns actual delta applied.
    std::int16_t apply_sp_pickup() noexcept;

    // ROM 0x007BD4: increments gem counter $FF0DB2 + (item_id - 0x0D) for item_id in [0x0D, 0x10].
    bool increment_gem_counter(std::uint8_t item_id) noexcept;
};

} // namespace oasis::game::ui
