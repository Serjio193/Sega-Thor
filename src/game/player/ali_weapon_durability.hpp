#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <tuple>

namespace oasis::game::player {

struct CombatWeaponDurabilityConfig {
    static constexpr std::uint32_t kWeaponDurabilityDecrementAddr = 0x008530U;
    static constexpr std::uint32_t kThresholdResetGuardAddr = 0x0085C4U;
    static constexpr std::uint32_t kDpadVelocityResolverAddr = 0x0085E2U;
    static constexpr std::uint32_t kDpadJumpTableAddr = 0x0085FAU;

    static constexpr std::uint32_t kEquippedSlotRam = 0x00FF0DB0U;
    static constexpr std::uint32_t kEquippedWeaponIdRam = 0x00FF0DB1U;
    static constexpr std::uint32_t kWeaponRevertTimerRam = 0x00FF0D7EU;
    static constexpr std::uint32_t kWeaponInventoryBaseRam = 0x00FF0D80U;
    static constexpr std::uint32_t kPlayerSubStateRam = 0x00FF19EEU;
    static constexpr std::uint32_t kHudWeaponIdRam = 0x00FF1854U;
    static constexpr std::uint32_t kHudWeaponStateRam = 0x00FF1855U;
    static constexpr std::uint32_t kHudWeaponToastTimerRam = 0x00FF1856U;
    static constexpr std::uint32_t kWeaponBreakFlagRam = 0x00FF198FU;
    static constexpr std::uint32_t kThresholdValueRam = 0x00FF0E3CU;
    static constexpr std::uint32_t kThresholdLimitRam = 0x00FF1A70U;
    static constexpr std::uint32_t kThresholdTrippedFlagRam = 0x00FF0DBBU;
    static constexpr std::uint32_t kDpadInputRam = 0x00FF165EU;

    static constexpr std::uint8_t kMinConsumableWeaponId = 2U;
    static constexpr std::uint8_t kMaxConsumableWeaponId = 12U;
    static constexpr std::uint8_t kInfiniteDurability = 0xFFU;
    static constexpr std::uint8_t kDefaultDaggerId = 1U;
    static constexpr std::uint16_t kRevertTimerDuration = 0x000AU;
    static constexpr std::uint16_t kHudToastTimerDuration = 0x005AU;
    static constexpr std::uint32_t kBreakSfxId = 0x00C00269U;
    static constexpr std::uint8_t kTrippedFlagValue = 0xFFU;

    static constexpr std::int32_t kSpeedCardinalX = 0x00036000;
    static constexpr std::int32_t kSpeedCardinalY = 0x00030000;
    static constexpr std::int32_t kSpeedDiagonalX = 0x0002A000;
    static constexpr std::int32_t kSpeedDiagonalY = 0x00025800;
};

struct WeaponSlot {
    std::uint8_t id{0U};
    std::uint8_t durability{0U};
};

struct InventoryState {
    std::array<WeaponSlot, 8> slots{};
    std::uint8_t equipped_slot{0U};    // $FF0DB0
    std::uint8_t equipped_id{1U};      // $FF0DB1
    std::uint16_t revert_timer{0U};    // $FF0D7E
    std::uint16_t sub_state{0U};       // $FF19EE
    std::uint8_t hud_weapon_id{1U};    // $FF1854
    std::uint8_t hud_weapon_state{0U}; // $FF1855
    std::uint16_t hud_toast_timer{0U}; // $FF1856
    std::uint8_t break_flag{0U};       // $FF198F
};

struct WeaponDurabilityResult {
    bool decremented{false};
    bool broke{false};
    std::uint8_t prev_durability{0U};
    std::uint8_t new_durability{0U};
    std::uint32_t sfx_id{0U};
};

struct DpadMotionResult {
    bool moving{false};
    std::uint16_t facing{0U};
    std::int32_t vx{0};
    std::int32_t vy{0};
};

/// ROM 0x008530..0x0085C2:
/// Decrements durability of equipped consumable weapon (IDs 2..12, finite durability != 0xFF).
/// When durability reaches 0, clears the inventory slot and reverts to Dagger (ID 1) with toast and SFX.
[[nodiscard]] WeaponDurabilityResult step_weapon_durability(InventoryState& inv) noexcept;

/// Direct functional test mirror for weapon durability state transition.
[[nodiscard]] std::tuple<bool, std::uint8_t, std::uint8_t> step_weapon_durability_direct(
    std::uint8_t weapon_id,
    std::uint8_t durability) noexcept;

/// ROM 0x0085C4..0x0085E0:
/// Unsigned comparison of threshold counter against limit: resets counter to 0 and trips flag to 0xFF when greater.
[[nodiscard]] bool check_threshold_guard(
    std::uint16_t& value,
    std::uint16_t limit,
    std::uint8_t& flag) noexcept;

/// ROM 0x0085E2..0x0086B2:
/// Resolves 16-way D-pad directional input nibble to 16.16 fixed-point velocities, cardinal facing, and moving flag.
[[nodiscard]] constexpr DpadMotionResult resolve_dpad_motion(
    std::uint8_t dpad_nibble,
    std::uint16_t current_facing) noexcept {
    switch (dpad_nibble & 0x0FU) {
    case 0x00U: // Neutral
    case 0x03U: // Up + Down
    case 0x0CU: // Left + Right
    case 0x0FU: // All four
        return DpadMotionResult{
            .moving = false,
            .facing = current_facing,
            .vx = 0,
            .vy = 0,
        };
    case 0x01U: // Up
    case 0x0DU: // Up + Left + Right
        return DpadMotionResult{
            .moving = true,
            .facing = 0U,
            .vx = 0,
            .vy = -CombatWeaponDurabilityConfig::kSpeedCardinalY,
        };
    case 0x02U: // Down
    case 0x0EU: // Down + Left + Right
        return DpadMotionResult{
            .moving = true,
            .facing = 2U,
            .vx = 0,
            .vy = CombatWeaponDurabilityConfig::kSpeedCardinalY,
        };
    case 0x04U: // Left
    case 0x07U: // Left + Up + Down
        return DpadMotionResult{
            .moving = true,
            .facing = 3U,
            .vx = -CombatWeaponDurabilityConfig::kSpeedCardinalX,
            .vy = 0,
        };
    case 0x08U: // Right
    case 0x0BU: // Right + Up + Down
        return DpadMotionResult{
            .moving = true,
            .facing = 1U,
            .vx = CombatWeaponDurabilityConfig::kSpeedCardinalX,
            .vy = 0,
        };
    case 0x05U: // Up-Left
        return DpadMotionResult{
            .moving = true,
            .facing = 0U,
            .vx = -CombatWeaponDurabilityConfig::kSpeedDiagonalX,
            .vy = -CombatWeaponDurabilityConfig::kSpeedDiagonalY,
        };
    case 0x06U: // Down-Left
        return DpadMotionResult{
            .moving = true,
            .facing = 2U,
            .vx = -CombatWeaponDurabilityConfig::kSpeedDiagonalX,
            .vy = CombatWeaponDurabilityConfig::kSpeedDiagonalY,
        };
    case 0x09U: // Up-Right
        return DpadMotionResult{
            .moving = true,
            .facing = 0U,
            .vx = CombatWeaponDurabilityConfig::kSpeedDiagonalX,
            .vy = -CombatWeaponDurabilityConfig::kSpeedDiagonalY,
        };
    case 0x0AU: // Down-Right
        return DpadMotionResult{
            .moving = true,
            .facing = 2U,
            .vx = CombatWeaponDurabilityConfig::kSpeedDiagonalX,
            .vy = CombatWeaponDurabilityConfig::kSpeedDiagonalY,
        };
    default:
        return DpadMotionResult{
            .moving = false,
            .facing = current_facing,
            .vx = 0,
            .vy = 0,
        };
    }
}

} // namespace oasis::game::player
