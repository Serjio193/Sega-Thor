#pragma once

#include "game/player/secondary_weapon.hpp"

#include <cstdint>

namespace oasis::game::player {

inline constexpr std::uint32_t kStartSpiritSummonAddr = 0x007E9EU;
inline constexpr std::uint32_t kQuickEquipWeaponSlotAddr = 0x007EC4U;

inline constexpr std::uint16_t kSpiritSummonPlayerState = 20U; // 0x14
inline constexpr std::uint16_t kEquipToastFrames = 60U;        // 0x3C

inline constexpr std::uint8_t kWeaponClassDagger = 0x0AU;
inline constexpr std::uint8_t kWeaponClassSword = 0x0CU;
inline constexpr std::uint8_t kWeaponClassBow = 0x0EU;
inline constexpr std::uint8_t kWeaponClassBomb = 0x10U;

inline constexpr std::uint32_t kSpiritSummonSfx = 0x00900359U;
inline constexpr std::uint32_t kWeaponEquipSfx = 0x00C0025DU;
inline constexpr std::uint32_t kWeaponEquipErrorSfx = 0x00A00253U;

struct SpiritSummonTriggerResult {
    bool summon_lock_ff1990{true};
    std::uint16_t player_state{kSpiritSummonPlayerState};
    std::uint16_t anim_state_2a{kSpiritSummonPlayerState};
    std::uint32_t sound_cmd{kSpiritSummonSfx};
};

struct WeaponQuickEquipState {
    std::uint8_t equipped_slot{0U};      // $FF0DB0
    std::uint8_t equipped_item_id{0U};   // $FF0DB1
    std::uint16_t weapon_class{0U};      // $FF0D7E
    std::uint16_t anim_counter{0U};      // $FF19EE
    std::uint8_t toast_item_id{0U};      // $FF1854
    std::uint8_t toast_flag{0U};         // $FF1855
    std::uint16_t toast_timer{0U};       // $FF1856
    bool hud_refresh_ff198f{false};      // $FF198F
};

struct WeaponQuickEquipResult {
    bool equipped{false};
    WeaponQuickEquipState next_state{};
    std::uint32_t sound_cmd{0U};
};

/// Models 0x007E9E..0x007EC2: triggers Ali's spirit summon state (0x14) and SFX.
[[nodiscard]] SpiritSummonTriggerResult start_spirit_summon() noexcept;

/// Models 0x007F10..0x007F80: maps weapon item ID to weapon class (0x0A/0x0C/0x0E/0x10).
[[nodiscard]] std::uint8_t map_weapon_item_to_class(std::uint8_t item_id) noexcept;

/// Models 0x007EC4..0x007F80: quick-equips target_slot (toggling to 0 if already equipped).
[[nodiscard]] WeaponQuickEquipResult quick_equip_weapon_slot(
    const WeaponInventory& inventory,
    WeaponQuickEquipState state,
    std::uint8_t target_slot) noexcept;

} // namespace oasis::game::player
