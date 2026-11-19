#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>

namespace thor::physics {

// ROM entrypoints and boundaries (0x00E0BA..0x00E25C, 418 bytes).
inline constexpr std::uint32_t kProjectileSparkLifecycleStartAddr = 0x00E0BAU;
inline constexpr std::uint32_t kDispatchSparkEffectsAddr = 0x00E0BAU;
inline constexpr std::uint32_t kDispatchSparkEffectsEndAddr = 0x00E0F4U;
inline constexpr std::size_t kDispatchSparkEffectsSizeBytes = 58U;

inline constexpr std::uint32_t kDispatchProjectileEntitiesAddr = 0x00E0F4U;
inline constexpr std::uint32_t kDispatchProjectileEntitiesEndAddr = 0x00E140U;
inline constexpr std::size_t kDispatchProjectileEntitiesSizeBytes = 76U;

inline constexpr std::uint32_t kProjectileStateJumpTableAddr = 0x00E140U;
inline constexpr std::array<std::uint16_t, 2> kProjectileStateJumpTableWords = {0x0010U, 0x0050U};
inline constexpr std::uint32_t kDispatchSingleProjectileSlotAddr = 0x00E144U;

inline constexpr std::uint32_t kProjectileGravityLandingAddr = 0x00E150U;
inline constexpr std::uint32_t kProjectileGravityLandingEndAddr = 0x00E192U;
inline constexpr std::size_t kProjectileGravityLandingSizeBytes = 66U;

inline constexpr std::uint32_t kProjectileGravityBlinkDespawnAddr = 0x00E192U;
inline constexpr std::uint32_t kProjectileGravityBlinkDespawnEndAddr = 0x00E20AU;
inline constexpr std::size_t kProjectileGravityBlinkDespawnSizeBytes = 120U;

inline constexpr std::uint32_t kProjectileBouncePhaseInitAddr = 0x00E20AU;
inline constexpr std::uint32_t kProjectileBouncePhaseInitEndAddr = 0x00E25CU;
inline constexpr std::size_t kProjectileBouncePhaseInitSizeBytes = 82U;

inline constexpr std::uint32_t kProjectileSparkLifecycleEndAddr = 0x00E25CU;
inline constexpr std::size_t kProjectileSparkLifecycleSizeBytes = 418U;

// RAM pool layout & global timer/pause addresses.
inline constexpr std::uint32_t kMegaDriveWramBase = 0x00FF0000U;
inline constexpr std::uint32_t kSparkPoolBaseRamAddr = 0x00FF2D8CU;
inline constexpr std::size_t kSparkPoolBaseRamOffset = 0x2D8CU;
inline constexpr std::size_t kSparkPoolSlotCount = 6U;
inline constexpr std::size_t kLightSlotStrideBytes = 0x5AU; // 90 bytes
inline constexpr std::size_t kSparkPoolByteSize = kSparkPoolSlotCount * kLightSlotStrideBytes;

inline constexpr std::uint32_t kProjectilePoolBaseRamAddr = 0x00FF2ABCU;
inline constexpr std::size_t kProjectilePoolBaseRamOffset = 0x2ABCU;
inline constexpr std::size_t kProjectilePoolSlotCount = 8U;
inline constexpr std::size_t kProjectilePoolByteSize = kProjectilePoolSlotCount * kLightSlotStrideBytes;

inline constexpr std::uint32_t kProjectilePauseFlagsRamAddr = 0x00FF17B8U;
inline constexpr std::size_t kProjectilePauseFlagsRamOffset = 0x17B8U;
inline constexpr std::uint8_t kProjectilePauseBitMask = 0x02U; // bit 1 ($FF17B8)
inline constexpr std::uint32_t kProjectileGlobalTimerRamAddr = 0x00FF1856U;
inline constexpr std::size_t kProjectileGlobalTimerRamOffset = 0x1856U;

// Slot field byte offsets within 0x5A-byte (projectile/spark) and 0xBC-byte (full entity) records.
inline constexpr std::size_t kSlotTypeOffset = 0x00U;        // s16 (spark != 0, projectile > 0)
inline constexpr std::size_t kSlotStateOffset = 0x04U;       // s16 (0 or 2 jump table index)
inline constexpr std::size_t kSlotPosYWordOffset = 0x0CU;    // s16 Y integer word
inline constexpr std::size_t kSlotElevationFpOffset = 0x10U; // s32 16.16 Z (+$10.w high word)
inline constexpr std::size_t kSlotTimer16Offset = 0x16U;     // u16 cleared by 0x00E20A
inline constexpr std::size_t kSlotLifetimeOffset = 0x22U;    // s16 frame lifetime counter
inline constexpr std::size_t kSlotFrameDirOffset = 0x2AU;    // u16 (+= 2 at 0x00E22C)
inline constexpr std::size_t kSlotFlags36Offset = 0x36U;     // u8 bit 1: blink / parent latch
inline constexpr std::size_t kSlotTileAttrByteOffset = 0x3DU;// u8 (0x18 at 0x00E236)
inline constexpr std::size_t kSlotRadiusWordOffset = 0x46U;  // u16 (0x18 at 0x00E242)
inline constexpr std::size_t kSlotExtentXOffset = 0x48U;     // u8 (0x18 at 0x00E248)
inline constexpr std::size_t kSlotExtentYOffset = 0x49U;     // u8 (0x18 at 0x00E24E)
inline constexpr std::size_t kSlotSpriteIdOffset = 0x4AU;    // u16 (0x38 at 0x00E23C)
inline constexpr std::size_t kSlotVelocityZFpOffset = 0x56U; // s32 Z velocity / u32 spark fn ptr
inline constexpr std::size_t kSlotBouncePhaseOffset = 0x9EU; // u8 (188B entity phase at 0x00E20A)
inline constexpr std::size_t kSlotParentPtrOffset = 0xA6U;   // u32 (188B parent ptr at 0x00E17C)
inline constexpr std::uint8_t kSlotBlinkBitMask = 0x02U;     // bit 1 of +$36

// Physics, lifetime blink/despawn, and bounce-init constants.
inline constexpr std::int32_t kProjectileGravityStepFp = 0x0000C000;
inline constexpr std::int16_t kProjectileBlinkStartFrame = 240; // 0x00F0
inline constexpr std::int16_t kProjectileBlinkEndFrame = 300;   // 0x012C
inline constexpr std::int16_t kBouncePhaseInitElevationInt = 8;
inline constexpr std::uint8_t kBouncePhaseInitNext = 0x63U;
inline constexpr std::int16_t kBouncePhaseTriggerElevationInt = 12;
inline constexpr std::uint8_t kBouncePhaseTriggerTileAttr = 0x18U;
inline constexpr std::uint16_t kBouncePhaseTriggerSpriteId = 0x0038U;
inline constexpr std::uint16_t kBouncePhaseTriggerRadius = 0x0018U;
inline constexpr std::uint8_t kBouncePhaseTriggerExtent = 0x18U;
inline constexpr std::uint8_t kBouncePhaseTriggerNext = 0x02U;

/// Projection of a 90-byte spark effect slot in pool $FF2D8C (6 slots).
struct SparkEffectSlot {
    std::int16_t type{0};           // +$00.w (!= 0 active)
    std::uint32_t callback_addr{0}; // +$56.l (non-zero handler address)
    std::uint32_t ram_addr{kSparkPoolBaseRamAddr};

    [[nodiscard]] constexpr bool is_active() const noexcept {
        return type != 0 && callback_addr != 0U;
    }

    [[nodiscard]] constexpr bool operator==(const SparkEffectSlot&) const noexcept = default;
};

using SparkCallback = std::function<void(std::size_t slot_index, SparkEffectSlot& slot)>;
using SparkRamCallback = std::function<void(
    std::size_t slot_index,
    std::uint32_t slot_ram_addr,
    std::uint32_t callback_addr,
    std::span<std::uint8_t> ram)>;

/// Minimal parent actor fields mutated when a < $FF2ABC slot lands on ground.
struct ParentEntityTarget {
    std::int16_t pos_y{0};    // +$0C.w
    std::uint8_t flags_36{0}; // +$36.b (bit 1 cleared on unlatch)

    [[nodiscard]] constexpr bool operator==(const ParentEntityTarget&) const noexcept = default;
};

/// Projection of a projectile or pre-$FF2ABC entity slot handled by 0x00E0F4..0x00E208.
struct ProjectileLifecycleSlot {
    std::int16_t type{0};               // +$00.w (> 0 active in $FF2ABC)
    std::int16_t state{0};              // +$04.w (0 = landing, 2 = blink/despawn)
    std::int16_t pos_y{0};              // +$0C.w
    std::int32_t elevation_fp{0};       // +$10.l (16.16 fixed-point Z)
    std::int16_t lifetime{0};           // +$22.w (signed 16-bit frame counter)
    std::uint8_t flags_36{0};           // +$36.b (bit 1 blink/visible latch)
    std::int32_t velocity_z_fp{0};      // +$56.l (16.16 Z velocity)
    std::uint32_t slot_ram_addr{kProjectilePoolBaseRamAddr};
    std::uint32_t parent_ram_addr{0};   // +$A6.l (valid when slot_ram_addr < $FF2ABC)
    ParentEntityTarget* parent{nullptr};

    [[nodiscard]] constexpr bool operator==(const ProjectileLifecycleSlot&) const noexcept = default;
};

/// Projection of a 188-byte entity slot fields updated by 0x00E20A..0x00E25C.
struct BouncePhaseEntitySlot {
    std::int32_t elevation_fp{0};    // +$10.l (only high integer word +$10.w updated)
    std::uint16_t field_16{0};       // +$16.w (cleared in phases 0 and 1)
    std::uint16_t field_2a{0};       // +$2A.w (+= 2 in phase 1)
    std::uint8_t tile_attr_3d{0};    // +$3D.b (0x18 in phase 1)
    std::uint16_t radius_46{0};      // +$46.w (0x18 in phase 1)
    std::uint8_t extent_x_48{0};     // +$48.b (0x18 in phase 1)
    std::uint8_t extent_y_49{0};     // +$49.b (0x18 in phase 1)
    std::uint16_t sprite_id_4a{0};   // +$4A.w (0x38 in phase 1)
    std::uint8_t bounce_phase_9e{0}; // +$9E.b (0->0x63, 1->2, else no-op)

    [[nodiscard]] constexpr bool operator==(const BouncePhaseEntitySlot&) const noexcept = default;
};

/// Resolves PC-relative jump table target at 0x00E140 for state_offset (0 or 2).
/// Returns 0x00E150, 0x00E192, or 0 if invalid.
[[nodiscard]] constexpr std::uint32_t resolve_projectile_jump_target(
    std::int16_t state_offset) noexcept {
    if (state_offset == 0) {
        return kProjectileGravityLandingAddr;
    }
    if (state_offset == 2) {
        return kProjectileGravityBlinkDespawnAddr;
    }
    return 0U;
}

/// 0x00E0BA..0x00E0F2: Checks pause bit 1 of pause_flags_ff17b8; if clear, iterates up to 6
/// spark slots ($FF2D8C) and invokes callback for each slot with type != 0 and callback_addr != 0.
std::size_t dispatch_spark_effects(
    std::span<SparkEffectSlot> sparks,
    std::uint8_t pause_flags_ff17b8,
    const SparkCallback& callback = {});

/// 0x00E150..0x00E190 (State 0): Applies 16.16 gravity (-0xC000) when elevation_fp != 0,
/// clamps < 0 to 0, and if slot_ram_addr < $FF2ABC and elevation_fp == 0, unlatches parent bit 1,
/// copies pos_y to parent, and clears slot.type = 0. Returns true if slot was freed.
bool step_projectile_gravity_landing(ProjectileLifecycleSlot& slot) noexcept;

/// 0x00E192..0x00E208 (State 2): Applies gravity and optional < $FF2ABC landing unlatch
/// (falling through without early return), increments lifetime counter, blinks +$36 bit 1 on
/// frames 240..300 (odd=set, even=clear), and clears slot.type = 0 when d0 > 300 (without writing
/// 301 to lifetime). Returns true if slot.type == 0 after step.
bool step_projectile_gravity_blink_despawn(ProjectileLifecycleSlot& slot) noexcept;

/// 0x00E144..0x00E14E: Dispatches a single slot via state_offset (+$04) through jump table 0x00E140.
bool dispatch_projectile_slot_state(ProjectileLifecycleSlot& slot) noexcept;

/// 0x00E0F4..0x00E13E: Decrements global_timer_ff1856 if non-zero (before pause check), checks
/// pause bit 1 of pause_flags_ff17b8, and dispatches up to 8 slots with type > 0.
std::size_t dispatch_projectile_entities(
    std::span<ProjectileLifecycleSlot> projectiles,
    std::uint16_t& global_timer_ff1856,
    std::uint8_t pause_flags_ff17b8) noexcept;

/// 0x00E20A..0x00E25A: Phase state machine on +$9E(a6) (0 -> init elev 8 & phase 0x63;
/// 1 -> elev 12, +$2A += 2, sprite 0x38, radius/extents 0x18, phase 2; else no-op).
bool step_projectile_bounce_phase(BouncePhaseEntitySlot& entity) noexcept;

// ============================================================================
// 64KB Work RAM Window Helpers ($FF0000..$FFFFFF)
// ============================================================================

[[nodiscard]] SparkEffectSlot load_spark_slot_from_ram(
    std::span<const std::uint8_t> ram,
    std::size_t slot_index) noexcept;

void store_spark_slot_to_ram(
    std::span<std::uint8_t> ram,
    std::size_t slot_index,
    const SparkEffectSlot& slot) noexcept;

[[nodiscard]] ProjectileLifecycleSlot load_projectile_slot_from_ram(
    std::span<const std::uint8_t> ram,
    std::size_t slot_index) noexcept;

void store_projectile_slot_to_ram(
    std::span<std::uint8_t> ram,
    std::size_t slot_index,
    const ProjectileLifecycleSlot& slot) noexcept;

std::size_t dispatch_spark_effects_from_ram(
    std::span<std::uint8_t> ram,
    const SparkRamCallback& callback = {});

bool step_projectile_gravity_landing_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t slot_ram_addr) noexcept;

bool step_projectile_gravity_blink_despawn_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t slot_ram_addr) noexcept;

std::size_t dispatch_projectile_entities_from_ram(
    std::span<std::uint8_t> ram) noexcept;

bool step_projectile_bounce_phase_from_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept;

} // namespace thor::physics

namespace oasis::game::physics {
using namespace thor::physics;
} // namespace oasis::game::physics
