#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <span>

namespace oasis::game::entities {

inline constexpr std::uint32_t kSolidSweepXPosAddr = 0x009DD6U;
inline constexpr std::uint32_t kEntityTableBaseRam = 0xFF19E8U;
inline constexpr std::size_t kSolidSweepSlotCount = 21U;
inline constexpr std::size_t kSolidSweepSlotStrideBytes = 188U; // 0xBC
inline constexpr std::uint8_t kPushContactTimerFrames = 3U;   // $6C = 3
inline constexpr std::uint8_t kSolidCollisionFlagBit7 = 0x80U; // $40 bit 7
inline constexpr std::uint8_t kSkipTraitBit3A = 0x02U;         // $3A bit 1
inline constexpr std::uint8_t kSkipRenderBit38 = 0x04U;        // $38 bit 2

/// Entity AABB & solidity descriptor for 0x009DD6 sweep.
struct SolidSweepBlocker {
    std::int32_t x{0};
    std::int32_t y{0};
    std::int32_t z{0};
    std::int32_t floor_z{0};
    std::int32_t half_w{0};   // +0x42
    std::int32_t half_h{0};   // +0x44
    std::int32_t height_z{0}; // +0x4A
    bool solid{true};
    bool pushable{false};
};

/// Result of +X solid entity sweep across up to 21 slots.
struct SolidSweepResult {
    std::int32_t clamped_dx{0};
    std::optional<std::size_t> blocker_index{}; // +0x68 blocker slot
    std::uint8_t push_timer{0};                 // +0x6C = 3 on contact
    bool nudge_blocker{false};                  // +1px when clearance == 0 && pushable
    bool col_flag_bit7{false};                  // +0x40 bit 7
};

/// Helper matching 0x009DD6 skip condition ($3A bit 1 or $38 bit 2).
[[nodiscard]] constexpr bool is_solid_sweep_eligible(
    std::uint8_t trait_flags_3a,
    std::uint8_t render_flags_38) noexcept {
    return (trait_flags_3a & kSkipTraitBit3A) == 0U &&
           (render_flags_38 & kSkipRenderBit38) == 0U;
}

/// Models 0x009DD6: sweeps along +X across up to 21 slots, testing 3D AABB
/// overlap in (x +/- $42, y + floor_z +/- $44, z + floor_z .. +$4A), clamping
/// dx to clearance, setting $40 bit 7, push_timer = 3, and nudge_blocker when
/// clearance == 0 and the blocker is pushable.
[[nodiscard]] SolidSweepResult sweep_solid_x_pos(
    const SolidSweepBlocker& mover,
    std::size_t mover_index,
    std::int32_t dx,
    std::span<const SolidSweepBlocker> slots) noexcept;

} // namespace oasis::game::entities
