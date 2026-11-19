#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>
#include <vector>

namespace thor::enemy {

// ROM entrypoints and boundaries (0x00EE94..0x00F054, 448 bytes, 120 instructions).
inline constexpr std::uint32_t kBossChargeState4Addr = 0x00EE94U;
inline constexpr std::uint32_t kBossChargeState2Addr = 0x00EF1EU;
inline constexpr std::uint32_t kBossChargeState0Addr = 0x00EF38U;
inline constexpr std::uint32_t kBossChargeDampingAddr = 0x00EF5EU;
inline constexpr std::uint32_t kBossChargeTrajectoryAddr = 0x00EFA2U;
inline constexpr std::uint32_t kBossChargeDespawnNotifyAddr = 0x00F038U;
inline constexpr std::uint32_t kBossChargeEngineEndAddr = 0x00F054U;
inline constexpr std::size_t kBossChargeEngineSizeBytes = 448U;

// Helper callees invoked by 0x00EE94..0x00F054 (ROM calls 0x00E7E8, not 0x00E78E).
inline constexpr std::uint32_t kBossChargeWaterProbeAddr = 0x00E36EU;
inline constexpr std::uint32_t kBossChargeViewportCullAddr = 0x00E610U;
inline constexpr std::uint32_t kBossChargeSpatialSfxAddr = 0x00E7E8U;

// Physics, culling, and notification constants.
inline constexpr std::int32_t kBossChargeGravityAccel = 0x0000C000;
inline constexpr std::int32_t kBossChargeBounceRestThreshold = 0x00004000;
inline constexpr std::int16_t kBossChargeViewportMarginPx = 48; // 0x30
inline constexpr std::uint16_t kBossChargeScreenShakeFrames = 4U;
inline constexpr std::uint16_t kBossChargeSpawnerNotifyMsg = 0x0010U;

// Audio SFX command IDs.
inline constexpr std::uint32_t kBossChargeSfxSwing = 0x00B00224U;
inline constexpr std::uint32_t kBossChargeSfxRoar = 0x00B00225U;
inline constexpr std::uint32_t kBossChargeSfxQuake = 0x00600382U;
inline constexpr std::uint32_t kBossChargeSfxBounce = 0x00400376U;

// Work RAM addresses and entity slot offsets (188 bytes / 0xBC stride).
inline constexpr std::uint32_t kBossChargePlayerAliRamAddr = 0x00FF19E8U;
inline constexpr std::uint32_t kBossChargeScrollModeRamOffset = 0x16F0U;
inline constexpr std::uint32_t kBossChargePlaneACamRamOffset = 0x1716U;
inline constexpr std::uint32_t kBossChargePlaneBCamRamOffset = 0x173EU;
inline constexpr std::uint32_t kBossChargeScreenShakeRamOffset = 0x17C4U;
inline constexpr std::size_t kBossChargeEntitySlotBytes = 0xBCU;

// Entity flags & render masks.
inline constexpr std::uint8_t kBossChargeFlagWaterBit = 0x08U;         // +$36 bit 3
inline constexpr std::uint8_t kBossChargeFlagAnimDoneBit = 0x02U;      // +$37 bit 1
inline constexpr std::uint8_t kBossChargeFlag38Bit3 = 0x08U;           // +$38 bit 3
inline constexpr std::uint8_t kBossChargeFlag3cBit1 = 0x02U;           // +$3C bit 1
inline constexpr std::uint16_t kBossChargeRenderHFlipBit = 0x0800U;   // +$18 bit 11
inline constexpr std::uint16_t kBossChargeRenderState4Mask = 0x97FFU;  // ~$0800 & ~$6000

/// Entity fields read and mutated by 0x00EE94..0x00F054 (188-byte entity slot layout).
struct BossChargeEntityState {
    std::int16_t type_00{0};             // +$00: Entity type ID (0 on despawn)
    std::uint16_t timer_02{0U};          // +$02: State 0 countdown timer
    std::uint16_t state_04{0U};          // +$04: Current state (0, 2, 4, 6, 8)
    std::int16_t world_x_08{0};          // +$08: Integer world X (read by 0x00E610 / 0x00E36E)
    std::int16_t world_y_0c{0};          // +$0C: Integer world Y (read by 0x00E610 / 0x00E36E)
    std::int32_t elev_z_10{0};           // +$10: 16.16 Z elevation
    std::uint16_t render_attr_18{0U};    // +$18: SAT tile/attr bits (bit 11 = h-flip)
    std::uint16_t anim_2a{0U};           // +$2A: Animation ID
    std::uint8_t flags_36{0U};           // +$36: Bit 3 = water/hazard despawn flag
    std::uint8_t flags_37{0U};           // +$37: Bit 1 = action frame completed
    std::uint8_t flags_38{0U};           // +$38: Bit 3 set on State 4 completion
    std::uint8_t flags_3c{0U};           // +$3C: Bit 1 set on State 4 completion
    std::int32_t vel_x_4e{0};            // +$4E: 16.16 X velocity
    std::int32_t vel_y_52{0};            // +$52: 16.16 Y velocity
    std::int32_t vel_z_56{0};            // +$56: 16.16 Z velocity
    std::int32_t accum_x_72{0};          // +$72: 16.16 integrated X accumulator
    std::int32_t accum_y_76{0};          // +$76: 16.16 integrated Y accumulator
    std::uint16_t subtype_a6{0U};        // +$A6: 0 / 1 / >=2 selector in State 4
    std::uint32_t spawner_ptr_aa{0U};    // +$AA: Parent spawner entity RAM address
    std::uint32_t self_ram_addr{0U};     // Slot's own RAM address for a0 == a6 check

    [[nodiscard]] constexpr bool operator==(const BossChargeEntityState&) const noexcept = default;
};

/// Active camera viewport bounds used by 0x00F02C -> 0x00E610.
struct BossChargeViewport {
    std::int16_t cam_x{0};
    std::int16_t cam_y{0};
    std::int16_t margin_px{kBossChargeViewportMarginPx};

    [[nodiscard]] constexpr bool operator==(const BossChargeViewport&) const noexcept = default;
};

/// Observable effects produced by one tick of 0x00EE94..0x00F054.
struct BossChargeStepResult {
    bool bounced{false};
    bool velocity_stopped{false};
    bool offscreen_culled{false};
    bool despawned{false};
    bool spawner_notified{false};
    std::uint32_t notified_spawner_addr{0U};
    std::uint16_t spawner_notify_msg{0U};
    bool state_transitioned{false};
    std::uint16_t screen_shake_frames{0U};
    std::vector<std::uint32_t> sfx_emitted{};
};

/// 0x00EF5E..0x00EF94: Computes 0.75x velocity via `(v >> 1) + ((v >> 1) >> 1)` (`asr.l #1`).
[[nodiscard]] constexpr std::int32_t damp_velocity_75_percent(std::int32_t vel) noexcept {
    const std::int32_t half = vel >> 1;
    return half + (half >> 1);
}

/// 0x00EF5E..0x00EFA0: Applies 0.75x damping to X/Y/Z velocities, integrates 0.75x X/Y
/// into +$72/+$76 and 0.5x Z (`vel_z >> 1`) into +$10, clamping +$10 >= 0.
void boss_charge_damping_step(BossChargeEntityState& ent) noexcept;

/// 0x00F038..0x00F052: Notifies parent spawner (+$06 = 0x0010) unless self or player Ali
/// ($FF19E8), and clears +$00 = 0.
BossChargeStepResult boss_charge_despawn_and_notify(BossChargeEntityState& ent) noexcept;

/// 0x00EFA2..0x00F036: Integrates +$72/+$76, applies -0xC000 gravity, handles ground/floor
/// collision, 50% bounce recoil, h-flip toggle, rest threshold (<0x4000), and 48px culling.
BossChargeStepResult boss_charge_trajectory_step(
    BossChargeEntityState& ent,
    const BossChargeViewport& viewport = {},
    std::optional<bool> water_immersed = std::nullopt);

/// 0x00EF38..0x00EF5C: Runs trajectory step, decrements +$02, and transitions to state 2 on zero.
BossChargeStepResult boss_charge_state0_step(
    BossChargeEntityState& ent,
    const BossChargeViewport& viewport = {},
    std::optional<bool> water_immersed = std::nullopt);

/// 0x00EF1E..0x00EF36: Runs trajectory step and transitions to state 4 if +$37 bit 1 is set.
BossChargeStepResult boss_charge_state2_step(
    BossChargeEntityState& ent,
    const BossChargeViewport& viewport = {},
    std::optional<bool> water_immersed = std::nullopt);

/// 0x00EE94..0x00EF1C: Runs trajectory step; if +$37 bit 1 set, masks +$18, sets flags,
/// transitions to state 6/8 per subtype +$A6, queues SFX via 0x00E7E8, and shakes screen ($FF17C4).
BossChargeStepResult boss_charge_state4_step(
    BossChargeEntityState& ent,
    const BossChargeViewport& viewport = {},
    std::optional<bool> water_immersed = std::nullopt);

/// Loads BossChargeEntityState from a 64KB Work RAM window ($FF0000..$FFFFFF or 16-bit offset).
[[nodiscard]] BossChargeEntityState load_boss_charge_entity_from_ram(
    std::span<const std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr) noexcept;

/// Stores BossChargeEntityState fields back into a 64KB Work RAM window.
void store_boss_charge_entity_to_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr,
    const BossChargeEntityState& ent) noexcept;

/// 64KB RAM window helpers for 0x00EF5E, 0x00EFA2, 0x00EF38, 0x00EF1E, 0x00EE94, 0x00F038.
BossChargeStepResult boss_charge_damping_step_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr) noexcept;

BossChargeStepResult boss_charge_trajectory_step_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr);

BossChargeStepResult boss_charge_state0_step_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr);

BossChargeStepResult boss_charge_state2_step_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr);

BossChargeStepResult boss_charge_state4_step_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr);

BossChargeStepResult boss_charge_despawn_and_notify_from_ram(
    std::span<std::uint8_t, 0x10000> ram,
    std::uint32_t entity_ram_addr) noexcept;

} // namespace thor::enemy

namespace oasis::game::enemy {
using namespace thor::enemy;
} // namespace oasis::game::enemy
