#pragma once

#include <array>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <span>

namespace oasis::game::player {

struct AliJumpPickupConfig {
    static constexpr std::uint32_t kJumpTriggerAddr = 0x00822CU;
    static constexpr std::uint32_t kPickupProbeAddr = 0x0082AEU;
    static constexpr std::uint32_t kInteractProbeAddr = 0x0082F8U;

    static constexpr std::uint32_t kCutsceneLockRam = 0x00FF1985U;
    static constexpr std::uint32_t kRunningJumpFlagRam = 0x00FF197EU;
    static constexpr std::uint32_t kStandingJumpFlagRam = 0x00FF1980U;
    static constexpr std::uint32_t kInputRingCountRam = 0x00FF16EAU;
    static constexpr std::uint32_t kCollectedItemRam = 0x00FF1976U;
    static constexpr std::uint32_t kCollectedSlotRam = 0x00FF1978U;
    static constexpr std::uint32_t kCollectedPayloadRam = 0x00FF197AU;

    static constexpr std::int32_t kJumpZVel16_16 = 0x00070000; // +7.0 px/frame
    static constexpr std::uint16_t kRunningJumpState = 22U;     // 0x16
    static constexpr std::uint16_t kStandingJumpState = 16U;    // 0x10
    static constexpr std::uint16_t kRunningSubAction = 4U;      // Ali running sub_action

    static constexpr std::int16_t kPickupProbeRadiusPx = 6;    // ±6px
    static constexpr std::uint16_t kPickupEntityType = 8U;     // Entity type 8
};

struct InteractBox {
    std::int16_t x1{0};
    std::int16_t y1{0};
    std::int16_t x2{0};
    std::int16_t y2{0};

    [[nodiscard]] constexpr bool contains_point(std::int16_t rel_x, std::int16_t rel_y) const noexcept {
        return rel_x >= x1 && rel_x <= x2 && rel_y >= y1 && rel_y <= y2;
    }
};

inline constexpr std::array<InteractBox, 4> kInteractBoxes = {{
    {-8, -30, 8, -4},  // Facing 0: Up
    {4, -8, 32, 8},    // Facing 1: Right
    {-8, 4, 8, 30},    // Facing 2: Down
    {-32, -8, -4, 8},  // Facing 3: Left
}};

[[nodiscard]] constexpr InteractBox get_interact_box(std::uint16_t facing) noexcept {
    return kInteractBoxes[facing & 0x03U];
}

[[nodiscard]] constexpr bool is_in_interact_box(
    std::uint16_t facing,
    std::int16_t rel_x,
    std::int16_t rel_y) noexcept {
    return get_interact_box(facing).contains_point(rel_x, rel_y);
}

enum class AliJumpType : std::uint8_t {
    None = 0,
    RunningJump,
    StandingJump,
};

struct AliJumpTriggerContext {
    bool cutscene_locked{false};         // $FF1985 != 0
    bool dpad_held{false};               // D-pad movement active
    std::uint8_t running_jump_flag{0U};  // $FF197E
    std::uint16_t standing_jump_flag{0U};// $FF1980
    std::uint16_t input_ring_count{0U};  // $FF16EA
};

struct AliJumpEntityState {
    std::uint16_t sub_action{0U};        // Ali +0x04
    std::uint16_t state{0U};             // Ali +0x2A
    std::int32_t vel_x_16_16{0};         // Ali +0x4E
    std::int32_t vel_y_16_16{0};         // Ali +0x52
    std::int32_t vel_z_16_16{0};         // Ali +0x56
    std::uint16_t prev_sub_action{0U};   // Ali +0x9E
};

struct AliJumpResult {
    AliJumpType jump_type{AliJumpType::None};
    bool triggered{false};
};

/// ROM 0x00822C..0x0082AC:
/// Evaluates and triggers Ali's jump:
/// If cutscene_locked ($FF1985 != 0), returns None.
/// If dpad_held && sub_action == 4 (running), triggers Running Jump (state 22, preserves XY velocity).
/// Otherwise triggers Standing Jump (state 16, zeros XY velocity).
[[nodiscard]] AliJumpResult trigger_ali_jump(
    AliJumpTriggerContext& ctx,
    AliJumpEntityState& ali) noexcept;

struct PickupCandidateEntity {
    std::uint16_t entity_type{0U};  // +0x00 (0x08 = Pickup)
    std::uint16_t sub_action{0U};   // +0x04
    std::int32_t pos_x_16_16{0};    // +0x08
    std::int32_t pos_y_16_16{0};    // +0x0C
    std::uint16_t item_id{0U};      // +0x32
    std::int32_t world_payload{0};  // +0x4E
    std::uint8_t quantity{0U};      // +0x52
};

struct PickupProbeResult {
    bool collected{false};
    std::size_t entity_index{0U};
    std::uint16_t collected_item_word{0U}; // (item_id << 8) | quantity -> $FF1976
    std::uint16_t collected_sub_action{0U};// $FF1978
    std::int32_t collected_payload{0};     // $FF197A
};

/// ROM 0x0082AE..0x0082F6:
/// Scans candidate entities within a ±6px box of Ali.
/// If entity_type == 8 (Pickup), despawns the entity (type = 0) and records item data.
[[nodiscard]] PickupProbeResult probe_pickup_entities(
    std::int32_t ali_x_16_16,
    std::int32_t ali_y_16_16,
    std::span<PickupCandidateEntity> entities) noexcept;

} // namespace oasis::game::player
