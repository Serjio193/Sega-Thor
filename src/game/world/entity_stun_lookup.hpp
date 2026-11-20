#pragma once

#include <cstddef>
#include <cstdint>
#include <functional>
#include <span>
#include <vector>

namespace thor::world {

// ROM entrypoints and boundaries (0x00F1B2..0x00F258, 166 bytes, 44 instructions).
inline constexpr std::uint32_t kEntityStunLookupStartAddr = 0x00F1B2U;
inline constexpr std::uint32_t kPool2KnockbackStunBroadcastAddr = 0x00F1B2U;
inline constexpr std::uint32_t kPool2KnockbackStunBroadcastEndAddr = 0x00F234U;
inline constexpr std::size_t kPool2KnockbackStunBroadcastSizeBytes = 130U;

inline constexpr std::uint32_t kEntityFindByTypeIdGlobalAddr = 0x00F234U;
inline constexpr std::uint32_t kEntityFindByTypeIdGlobalEndAddr = 0x00F258U;
inline constexpr std::size_t kEntityFindByTypeIdGlobalSizeBytes = 36U;

inline constexpr std::uint32_t kEntityStunLookupEndAddr = 0x00F258U;
inline constexpr std::size_t kEntityStunLookupSizeBytes = 166U;
inline constexpr std::uint32_t kKnockbackWaveLaunchHelperAddr = 0x00E4D0U;

// Work RAM pools, stride, and cutscene script bytecode trigger ($FF17B2).
// Note: Pool 2 ($FF1CD8, 17 slots) overlaps slots 4..20 of Pool 1 ($FF19E8, 21 slots).
inline constexpr std::uint32_t kStunBroadcastPool2RamAddr = 0x00FF1CD8U;
inline constexpr std::size_t kStunBroadcastPool2RamOffset = 0x1CD8U;
inline constexpr std::size_t kStunBroadcastPool2SlotCount = 17U; // moveq #$10, d1 -> 17 slots

inline constexpr std::uint32_t kGlobalTypeLookupPool1RamAddr = 0x00FF19E8U;
inline constexpr std::size_t kGlobalTypeLookupPool1RamOffset = 0x19E8U;
inline constexpr std::size_t kGlobalTypeLookupPool1SlotCount = 21U; // moveq #$14, d1 -> 21 slots

inline constexpr std::size_t kStunEntitySlotStrideBytes = 0xBCU; // 188 bytes
inline constexpr std::size_t kStunPoolMemoryEndOffset =
    kGlobalTypeLookupPool1RamOffset +
    (kGlobalTypeLookupPool1SlotCount * kStunEntitySlotStrideBytes); // 0x2954

inline constexpr std::uint32_t kStunScriptEventPtrRamAddr = 0x00FF17B2U;
inline constexpr std::size_t kStunScriptEventPtrRamOffset = 0x17B2U;
inline constexpr std::uint32_t kStunScriptEventPtrValue = 0x0004382EU;

// Entity slot field offsets within each 188-byte record.
inline constexpr std::size_t kStunEntityTypeOffset = 0x00U;      // s16 type/status
inline constexpr std::size_t kStunEntityFacingOffset = 0x16U;    // u16 cardinal facing
inline constexpr std::size_t kStunEntityFlags38Offset = 0x38U;   // u8 bit 3: intangible
inline constexpr std::size_t kStunEntityFlags3AOffset = 0x3AU;   // u8 bit 0: invulnerable
inline constexpr std::size_t kStunEntityVelXFpOffset = 0x4EU;    // s32 16.16 X velocity
inline constexpr std::size_t kStunEntityVelYFpOffset = 0x52U;    // s32 16.16 Y velocity
inline constexpr std::size_t kStunEntityVelZFpOffset = 0x56U;    // s32 16.16 Z velocity
inline constexpr std::size_t kStunEntityHitstunOffset = 0x82U;   // u8 active hitstun
inline constexpr std::size_t kStunEntityLaunchDirOffset = 0x84U; // u16 launch/knockback direction
inline constexpr std::size_t kStunEntityDamageOffset = 0x86U;    // u16 pending damage word
inline constexpr std::size_t kStunEntityHitTimerOffset = 0x8CU;  // u16 hit flash/lock timer

// Eligibility bitmasks, damage/timer constants, and 0x00E4D0 launch speeds.
inline constexpr std::uint8_t kStunIntangibleBit3Mask = 0x08U;
inline constexpr std::uint8_t kStunInvulnerableBit0Mask = 0x01U;
inline constexpr std::uint16_t kStunReverseFacingBit1Mask = 0x0002U;
inline constexpr std::uint16_t kStunKnockbackDamageWord = 0xCFFFU;
inline constexpr std::uint16_t kStunHitTimerLatchWord = 0xFFFFU;
inline constexpr std::uint16_t kStunLaunchBaseSpeedX = 0x0300U;
inline constexpr std::uint16_t kStunLaunchBaseSpeedY = 0x02E0U;
inline constexpr std::int32_t kStunLaunchUpwardVelZFp = 0x00060000;

/// Projection of a 188-byte entity slot accessed by 0x00F1B2..0x00F258.
struct StunBroadcastEntity {
    std::int16_t type{0};            // +$00.w (<= 0 skipped in broadcast; == d0 in lookup)
    std::uint16_t facing_16{0U};     // +$16.w (read-only; bit 1 toggled into +$84)
    std::uint8_t flags_38{0U};       // +$38.b (bit 3 intangible -> skip)
    std::uint8_t flags_3a{0U};       // +$3A.b (bit 0 invulnerable -> skip)
    std::int32_t vel_x_fp{0};        // +$4E.l (16.16 X velocity from 0x00E4D0)
    std::int32_t vel_y_fp{0};        // +$52.l (16.16 Y velocity from 0x00E4D0)
    std::int32_t vel_z_fp{0};        // +$56.l (16.16 Z velocity from 0x00E4D0)
    std::uint8_t hitstun_82{0U};     // +$82.b (!= 0 skipped)
    std::uint16_t launch_dir_84{0U}; // +$84.w (facing_16 ^ 0x0002)
    std::uint16_t damage_86{0U};     // +$86.w (0xCFFF)
    std::uint16_t hit_timer_8c{0U};  // +$8C.w (set to 0xFFFF iff previously 0)

    [[nodiscard]] constexpr bool operator==(const StunBroadcastEntity&) const noexcept = default;
};

/// Optional callback modeling `clr.w d7; bsr.w $e4d0` inside 0x00F1B2.
using StunLaunchCallback = std::function<void(
    std::size_t slot_index,
    StunBroadcastEntity& entity,
    std::uint16_t status_mask_d7)>;

/// Result summary from 0x00F1B2..0x00F234 (`Entity_Pool2_KnockbackStunWaveBroadcast`).
struct StunBroadcastResult {
    bool carry_set{false};                     // CCR Carry: false if OOB or caller +$00 != 0
    std::uint32_t script_event_ptr_ff17b2{0U}; // 0x0004382E when carry_set == true
    std::vector<std::size_t> affected_slots{}; // Pool 2 indices [0..16] stunned

    [[nodiscard]] constexpr bool operator==(const StunBroadcastResult&) const noexcept = default;
};

/// Result from 0x00F234..0x00F258 (`Entity_FindByTypeIdGlobal`).
struct GlobalTypeLookupResult {
    bool found{false};               // CCR Carry: true on first match across 21 slots
    std::size_t slot_index{0U};      // 0..20 when found
    std::uint32_t slot_ram_addr{0U}; // $FF19E8 + slot_index * 0xBC when found

    [[nodiscard]] constexpr bool operator==(const GlobalTypeLookupResult&) const noexcept = default;
};

/// 0x00F1CC..0x00F1EC: Checks eligibility (type > 0, +$38 bit 3 clear, +$3A bit 0 clear, +$82 == 0).
[[nodiscard]] constexpr bool is_stun_broadcast_eligible(
    const StunBroadcastEntity& entity) noexcept
{
    return entity.type > 0 &&
           (entity.flags_38 & kStunIntangibleBit3Mask) == 0U &&
           (entity.flags_3a & kStunInvulnerableBit0Mask) == 0U &&
           entity.hitstun_82 == 0U;
}

/// 0x00E4D0 fallback: if (status_mask_d7 & 0xC000) == 0, projects (0x0300, 0x02E0) along
/// entity.launch_dir_84 & 7 into vel_x_fp/vel_y_fp and sets vel_z_fp = 0x00060000.
void apply_default_stun_launch_physics(
    StunBroadcastEntity& entity,
    std::uint16_t status_mask_d7 = 0U) noexcept;

/// 0x00F1B2..0x00F234: Verifies caller slot is free (type == 0), sweeps up to 17 Pool 2 slots,
/// writes facing_16 ^ 0x0002 to +$84 (preserving +$16), invokes 0x00E4D0 (or launch_cb) with d7=0,
/// sets +$86 = 0xCFFF and +$8C = 0xFFFF (if 0), writes $FF17B2 = 0x0004382E, returns Carry set.
StunBroadcastResult broadcast_pool2_knockback_stun(
    std::span<StunBroadcastEntity> pool2_slots,
    std::uint16_t caller_slot_index,
    std::uint32_t& script_event_ptr_ff17b2,
    const StunLaunchCallback& launch_cb = {});

/// 0x00F234..0x00F258: Scans up to 21 type words ($FF19E8[0..20]) for exact match == target_type.
[[nodiscard]] GlobalTypeLookupResult find_entity_by_type_id_global(
    std::span<const std::int16_t> pool1_types,
    std::int16_t target_type) noexcept;

/// Overload scanning StunBroadcastEntity records across up to 21 slots.
[[nodiscard]] GlobalTypeLookupResult find_entity_by_type_id_global(
    std::span<const StunBroadcastEntity> pool1_slots,
    std::int16_t target_type) noexcept;

// ============================================================================
// 64KB Work RAM Window Helpers ($FF0000..$FFFFFF)
// ============================================================================

/// Loads StunBroadcastEntity fields from Work RAM at entity_ram_addr ($FFxxxx or 16-bit offset).
[[nodiscard]] StunBroadcastEntity load_stun_entity_from_ram(
    std::span<const std::uint8_t> ram,
    std::uint32_t entity_ram_addr) noexcept;

/// Stores StunBroadcastEntity fields into Work RAM at entity_ram_addr.
void store_stun_entity_to_ram(
    std::span<std::uint8_t> ram,
    std::uint32_t entity_ram_addr,
    const StunBroadcastEntity& entity) noexcept;

/// 0x00F1B2 against Work RAM ($FF1CD8[0..16], $FF17B2): mutates only affected slots'
/// +$4E, +$52, +$56, +$84, +$86, +$8C and $FF17B2 on Carry set.
StunBroadcastResult broadcast_pool2_knockback_stun_from_ram(
    std::span<std::uint8_t> ram,
    std::uint16_t caller_slot_index,
    const StunLaunchCallback& launch_cb = {});

/// 0x00F234 against Work RAM ($FF19E8[0..20]): scans 21 slots for +$00.w == target_type.
[[nodiscard]] GlobalTypeLookupResult find_entity_by_type_id_global_from_ram(
    std::span<const std::uint8_t> ram,
    std::int16_t target_type) noexcept;

} // namespace thor::world

namespace oasis::game::world {
using namespace thor::world;
} // namespace oasis::game::world
