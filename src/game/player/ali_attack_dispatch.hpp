#pragma once

#include <cstdint>
#include <optional>
#include <utility>

namespace oasis::game::player {

inline constexpr std::uint32_t kAliAttackDispatchAddr = 0x008002U;
inline constexpr std::uint32_t kAliAirborneAttackDispatchAddr = 0x0080D8U;
inline constexpr std::uint32_t kAliAttackDispatchEndAddr = 0x00814CU;

inline constexpr std::uint16_t kWeaponClassDagger = 0x000AU;
inline constexpr std::uint16_t kWeaponClassSword = 0x000CU;
inline constexpr std::uint16_t kWeaponClassBow = 0x000EU;
inline constexpr std::uint16_t kWeaponClassBomb = 0x0010U;

inline constexpr std::uint16_t kAnimDaggerGround = 0x003AU;
inline constexpr std::uint16_t kAnimDaggerAirKick = 0x0040U;
inline constexpr std::uint16_t kAnimSwordSlash = 0x004EU;
inline constexpr std::uint16_t kAnimSwordThrust = 0x0060U;
inline constexpr std::uint16_t kAnimSwordSpin = 0x0066U;
inline constexpr std::uint16_t kAnimBowFire = 0x0054U;
inline constexpr std::uint16_t kAnimBowHyperFire = 0x005CU;
inline constexpr std::uint16_t kAnimBombThrow = 0x0058U;

inline constexpr std::uint16_t kStateDaggerGround = 0x000AU;
inline constexpr std::uint16_t kStateDaggerAirKick = 0x0038U;
inline constexpr std::uint16_t kStateSwordSlash = 0x0046U;
inline constexpr std::uint16_t kStateSwordThrust = 0x0054U;
inline constexpr std::uint16_t kStateSwordSpin = 0x005CU;
inline constexpr std::uint16_t kStateBowFire = 0x004CU;
inline constexpr std::uint16_t kStateBombThrow = 0x0050U;

inline constexpr std::uint8_t kAirKickStunFrames = 0x15U; // 21 frames
inline constexpr std::uint32_t kBowFireSfx = 0x00600377U;
inline constexpr std::uint8_t kAttackAnimRate = 0x04U;
inline constexpr std::uint8_t kHyperBowItemId = 8U;

struct AliAttackInput {
    std::uint16_t weapon_class{kWeaponClassDagger}; // $FF0D7E
    std::uint8_t equipped_item_id{0U};              // $FF0DB1
    bool is_airborne{false};                         // true if player state == 4 or mid-air
    bool dpad_held{false};                          // $FF165E & 0x0F != 0
    bool spin_cmd{false};                           // special move spin flag
    bool active_projectile{false};                  // $FF1B60 != 0
};

struct AliAttackDispatchResult {
    bool dispatched{false};
    std::uint16_t anim{0U};
    std::uint16_t state{0U};
    std::uint8_t anim_rate{0U};
    bool clear_input_ring{false};
    std::uint8_t set_flags_39{0U};
    std::uint8_t air_stun_9a{0U};
    std::uint32_t sound_cmd{0U};
};

/// Models 0x008002..0x00814C: dispatches Ali's attack action and target state
/// based on weapon class, equipped item, D-pad, spin command, and air state.
[[nodiscard]] AliAttackDispatchResult dispatch_ali_attack(
    const AliAttackInput& input) noexcept;

/// Reference helper matching rom_ali_attack_dispatch_test.py: returns (anim, state) or nullopt.
[[nodiscard]] std::optional<std::pair<std::uint16_t, std::uint16_t>> dispatch_ground_attack(
    std::uint16_t weapon_class,
    std::uint8_t equipped_item_id,
    bool dpad_held,
    bool spin_cmd,
    bool active_projectile) noexcept;

} // namespace oasis::game::player
