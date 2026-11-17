#pragma once

#include <cstdint>

namespace oasis::game::video {

inline constexpr std::uint32_t kSpriteDmaThrottleEntryAddr = 0x00A9A0U;
inline constexpr std::uint32_t kSpriteDmaBudgetCheckAddr = 0x00A9AEU;
inline constexpr std::uint32_t kShadowVisibilityCheckAddr = 0x00AA78U;

inline constexpr std::uint32_t kDmaBudgetRamAddr = 0x00FF1652U;
inline constexpr std::uint32_t kDmaBacklogFlagRamAddr = 0x00FF1651U;
inline constexpr std::uint8_t kDmaBacklogActiveFlag = 0xFFU;
inline constexpr std::uint8_t kShadowDisableBit36 = 0x08U; // bit 3
inline constexpr std::uint8_t kShadowGroundBit36 = 0x04U;  // bit 2

struct SpriteDmaFrameSelection {
    std::int32_t selected_frame{0};
    bool hflip{false}; // true when facing == 3
    bool dma_queued{false};
    bool throttled{false};
    std::int32_t next_cached_frame{0};
    std::int32_t remaining_budget{0};
    std::uint8_t backlog_flag{0U}; // 0xFF when throttled ($FF1651)
};

/// Models 0x00A9AE..0x00A9F8: maps facing 3 -> 1 (+hflip), checks target_frame
/// vs cached_frame, and throttles back to cached_frame ($FF1651 = 0xFF) when
/// cached_frame >= 0 && dma_cost > dma_budget.
[[nodiscard]] SpriteDmaFrameSelection select_entity_frame_with_dma_budget(
    std::uint8_t facing,
    std::int32_t anim_frame,
    std::int32_t cached_frame,
    std::int32_t dma_cost,
    std::int32_t dma_budget) noexcept;

/// Models 0x00AA78..0x00AAA5: returns true when !(flags_36 & 8) and
/// (z > 0 || (z == 0 && (flags_36 & 4))).
[[nodiscard]] bool should_draw_entity_shadow(
    std::int32_t z,
    std::uint8_t flags_36) noexcept;

} // namespace oasis::game::video
