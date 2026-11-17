#include "game/video/sprite_dma_throttle.hpp"

namespace oasis::game::video {

SpriteDmaFrameSelection select_entity_frame_with_dma_budget(
    std::uint8_t facing,
    std::int32_t anim_frame,
    std::int32_t cached_frame,
    std::int32_t dma_cost,
    std::int32_t dma_budget) noexcept {
    const bool hflip = (facing == 3U);
    const std::uint8_t effective_facing = hflip ? 1U : facing;
    const std::int32_t target_frame =
        static_cast<std::int32_t>(effective_facing) + anim_frame;

    if (target_frame == cached_frame) {
        return SpriteDmaFrameSelection{
            target_frame, hflip, false, false, cached_frame, dma_budget, 0U,
        };
    }

    if (cached_frame >= 0 && dma_cost > dma_budget) {
        return SpriteDmaFrameSelection{
            cached_frame, hflip, false, true, cached_frame, dma_budget,
            kDmaBacklogActiveFlag,
        };
    }

    return SpriteDmaFrameSelection{
        target_frame, hflip, true, false, target_frame, dma_budget - dma_cost, 0U,
    };
}

bool should_draw_entity_shadow(
    std::int32_t z,
    std::uint8_t flags_36) noexcept {
    if ((flags_36 & kShadowDisableBit36) != 0U) {
        return false;
    }
    return z > 0 || (z == 0 && (flags_36 & kShadowGroundBit36) != 0U);
}

} // namespace oasis::game::video
