#include "game/interaction/interaction_dispatcher.hpp"

#include <cstdlib>

namespace oasis::game::interaction {

bool evaluate_interaction_proximity_and_facing(
    std::int32_t px,
    std::int32_t py,
    player::Direction facing,
    std::int32_t target_x,
    std::int32_t target_y,
    std::int32_t radius) noexcept {
    if (radius < 0) {
        return false;
    }

    const std::int32_t dx = target_x - px;
    const std::int32_t dy = target_y - py;

    // ROM 0x00B852: 16 px axis-aligned proximity gate
    if (std::abs(dx) > radius || std::abs(dy) > radius) {
        return false;
    }

    // ROM 0x00E338: Facing direction check toward target
    switch (facing) {
        case player::Direction::up:
            return dy <= 0;
        case player::Direction::down:
            return dy >= 0;
        case player::Direction::left:
            return dx <= 0;
        case player::Direction::right:
            return dx >= 0;
        case player::Direction::up_left:
            return dy <= 0 && dx <= 0;
        case player::Direction::up_right:
            return dy <= 0 && dx >= 0;
        case player::Direction::down_left:
            return dy >= 0 && dx <= 0;
        case player::Direction::down_right:
            return dy >= 0 && dx >= 0;
        case player::Direction::none:
        default:
            return false;
    }
}

InteractionDispatchResult dispatch_interaction_by_type(std::uint16_t entity_type) noexcept {
    switch (entity_type) {
        case 0x0032U:
            return InteractionDispatchResult{
                InteractionRoute::TownNpc0032,
                InteractionDispatcherConfig::kTownNpcHandlerRom,
                0U,
                0U,
                0U,
                false,
            };
        case 0x0020U:
            return InteractionDispatchResult{
                InteractionRoute::SpiritAwakening0020,
                InteractionDispatcherConfig::kSpiritAwakeningHandlerRom,
                0U,
                0U,
                0U,
                false,
            };
        case 0x0094U:
        case 0x0096U:
        case 0x00BEU:
            return InteractionDispatchResult{
                InteractionRoute::StoryNpc018EBE,
                InteractionDispatcherConfig::kStoryNpcHandlerRom,
                0U,
                0U,
                0U,
                true,
            };
        case 0x00B0U:
            return InteractionDispatchResult{
                InteractionRoute::Signpost00B0,
                InteractionDispatcherConfig::kSignpostHandlerRom,
                InteractionDispatcherConfig::kSignpostSfx,
                InteractionDispatcherConfig::kSignpostFreezeState,
                InteractionDispatcherConfig::kSignpostDialogueRom,
                false,
            };
        default:
            return InteractionDispatchResult{};
    }
}

std::uint32_t resolve_story_npc_post_dialogue_hook(std::uint16_t message_id) noexcept {
    switch (message_id) {
        case 0x0024U:
            return InteractionDispatcherConfig::kHookMsg24Rom;
        case 0x002FU:
            return InteractionDispatcherConfig::kHookMsg2FRom;
        case 0x0030U:
            return InteractionDispatcherConfig::kHookMsg30Rom;
        case 0x0039U:
            return InteractionDispatcherConfig::kHookMsg39Rom;
        default:
            return 0U;
    }
}

} // namespace oasis::game::interaction
