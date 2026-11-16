#pragma once

#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace oasis::game::player {

// ROM 0x0059BA..0x005A2F: Complete 59-state Ali player action table (0x00..0x74 even IDs).
enum class PlayerActionState : std::uint16_t {
    IdleStanding = 0x00,
    Walking = 0x02,
    RunningOrDash = 0x04,
    JumpingAirborne = 0x06,
    KnockbackHurt = 0x08,
    Climbing = 0x0A,
    Swimming = 0x0C,
    FallingLanding = 0x0E,
    Crouching = 0x10,
    CrouchRecovery = 0x12,
    SpiritSummonCast = 0x14,
    JumpAttack = 0x16,
    StunRecovery = 0x18,
    PrimaryAttackStart = 0x1A,
    PrimaryAttackHold = 0x1C,
    AttackFollowThrough = 0x1E,
    SecondaryAttackStart = 0x20,
    SecondaryAttackHold = 0x22,
    AttackRecovery = 0x24,
    AerialSpecial = 0x26,
    LadderClimb28 = 0x28,
    LadderClimb2A = 0x2A,
    LadderClimb2C = 0x2C,
    LadderClimb2E = 0x2E,
    Action30 = 0x30,
    PushPull32 = 0x32,
    PushPull34 = 0x34,
    NpcDialogueLock = 0x36,
    Action38 = 0x38,
    Action3A = 0x3A,
    Action3C = 0x3C,
    PitFall = 0x3E,
    Action40 = 0x40,
    Action42 = 0x42,
    PickupFanfare = 0x44,
    WeaponCombo46 = 0x46,
    WeaponCombo48 = 0x48,
    WeaponCombo4A = 0x4A,
    WeaponCombo4C = 0x4C,
    WeaponCombo4E = 0x4E,
    WeaponCombo50 = 0x50,
    WeaponCombo52 = 0x52,
    WeaponCombo54 = 0x54,
    WeaponCombo56 = 0x56,
    WeaponCombo58 = 0x58,
    WeaponCombo5A = 0x5A,
    WeaponCombo5C = 0x5C,
    Death = 0x5E,
    GameOver = 0x60,
    ItemPickup62 = 0x62,
    ItemPickup64 = 0x64,
    ItemPickup66 = 0x66,
    ItemPickup68 = 0x68,
    Action6A = 0x6A,
    AutoRevive = 0x6C,
    Action6E = 0x6E,
    Action70 = 0x70,
    Action72 = 0x72,
    Action74 = 0x74,
};

struct PlayerActionMachineConfig {
    static constexpr std::uint32_t kTickEntryAddr = 0x00005580U;
    static constexpr std::uint32_t kStateTableAddr = 0x000059BAU;
    static constexpr std::uint16_t kStateCount = 59U;
    static constexpr std::uint16_t kMaxStateId = 0x0074U;

    static constexpr std::uint16_t kLowHpThreshold = 0x0040U; // 64
    static constexpr std::uint32_t kLowHpSfx = 0x00A0038CU;
    static constexpr std::uint16_t kLowSpThreshold = 0x0018U; // 24
    static constexpr std::uint32_t kLowSpSfx = 0x00A004DDU;

    static constexpr std::uint32_t kReviveSfx = 0x00D00355U;
    static constexpr std::uint16_t kReviveItemId = 0x002AU;
    static constexpr std::uint16_t kReviveStepPerFrame = 3U;
    static constexpr std::uint8_t kGameOverDelayFrames = 40U; // 0x28
};

// ROM 0x0059BA..0x005A2F: Resolves `entry_addr + s16_be(entry_addr)` where
// `entry_addr = 0x0059BA + state_id` for even `state_id <= 0x74` (59 states).
[[nodiscard]] std::optional<std::uint32_t> lookup_player_state_handler_addr(
    std::span<const std::uint8_t> rom,
    std::uint16_t state_id) noexcept;

// Models Ali's per-frame pre-dispatch tick (0x005580..0x0057AE) and death/revive (0x005F92, 0x00611E).
struct PlayerPreTickState {
    std::uint16_t state{0x0000U}; // +4(a6)
    std::uint16_t hp{100U};
    std::uint16_t max_hp{100U};
    std::uint16_t sp{100U};
    std::uint16_t max_sp{100U};
    std::uint16_t active_spirit{0U}; // $FF1AA4

    bool airborne{false};             // +0x6F & 0x80
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t safe_respawn_x{0};   // $FF1972
    std::int32_t safe_respawn_y{0};

    bool hazard_ring{false};          // $FF0DBA & 1
    bool sp_ring{false};              // $FF0DBA & 4
    bool regen_speed_boost{false};    // $FF0DBA & 8
    bool tile_hazard_or_water{false}; // $FF16F0 & 0x20

    std::uint8_t low_hp_timer{0U};    // $FF1993
    std::uint8_t low_sp_timer{0U};    // $FF1994
    std::uint8_t hazard_timer{0U};
    std::uint8_t sp_regen_timer{0U};
    std::uint32_t frame_counter{0U};  // $FF1971

    bool death_anim_finished{false};  // $37 & 2
    bool auto_revive_slot_present{false}; // 0x0061CC
    std::uint16_t active_item_id{0U};
    std::uint8_t game_over_fade_timer{0U}; // $FF1992

    std::vector<std::uint32_t> sound_commands{};

    void step_pre_dispatch_tick();
    void step_death_and_revive();
};

} // namespace oasis::game::player
