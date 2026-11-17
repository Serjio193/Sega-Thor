#pragma once

#include <cstddef>
#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::video {

inline constexpr std::uint32_t kYsortSatPipelineAddr = 0x00A196U;

inline constexpr std::uint32_t kHighPriorityFxRam = 0x00FF2954U;
inline constexpr std::uint32_t kHitSparksRam = 0x00FF19C8U;
inline constexpr std::uint32_t kYSortEntitiesRam = 0x00FF19E8U;
inline constexpr std::uint32_t kGroundShadowFxRam = 0x00FF2ABCU;
inline constexpr std::uint32_t kFloorFxRam = 0x00FF2D8CU;
inline constexpr std::uint32_t kEntityDoneMaskRam = 0x00FF188EU;

inline constexpr std::size_t kHighPriorityFxCount = 4U;
inline constexpr std::size_t kHitSparksCount = 4U;
inline constexpr std::size_t kYSortEntityCount = 21U;
inline constexpr std::size_t kGroundShadowFxCount = 8U;
inline constexpr std::size_t kFloorFxCount = 6U;
inline constexpr std::int32_t kInitialMaxYSentinel = -1;

enum class SatRenderPassKind : std::uint8_t {
    HighPriorityFx = 0, // $FF2954, 4 slots, stride 0x5A
    HitSparks,          // $FF19C8, 4 slots, stride 0x08
    YSortedEntity,      // $FF19E8, 21 slots, stride 0xBC (descending Y > -1)
    GroundShadowFx,     // $FF2ABC, 8 slots, stride 0x5A
    FloorFx,            // $FF2D8C, 6 slots, stride 0x5A
};

struct SatRenderSlotInput {
    bool active{false};
    std::int32_t y{0}; // 0x0C(a6), must be > -1 to render
};

struct SatRenderOrderEntry {
    SatRenderPassKind pass{SatRenderPassKind::HighPriorityFx};
    std::uint8_t slot_index{0U};
};

/// Models 0x00A196..0x00A27E: schedules the 5 front-to-back SAT render passes
/// (4 high-prio FX, 4 hit-sparks, 21 entities sorted by descending Y > -1,
/// 8 ground/shadow FX, 6 floor FX).
[[nodiscard]] std::vector<SatRenderOrderEntry> build_ysort_sat_render_order(
    std::span<const bool> high_priority_fx_active,
    std::span<const bool> hit_sparks_active,
    std::span<const SatRenderSlotInput> entities,
    std::span<const bool> ground_shadow_fx_active,
    std::span<const bool> floor_fx_active);

} // namespace oasis::game::video
