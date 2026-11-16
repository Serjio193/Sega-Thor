#include "game/interaction/interaction_dispatcher.hpp"

#include <cassert>

int main() {
    using oasis::game::interaction::dispatch_interaction_by_type;
    using oasis::game::interaction::evaluate_interaction_proximity_and_facing;
    using oasis::game::interaction::InteractionDispatcherConfig;
    using oasis::game::interaction::InteractionRoute;
    using oasis::game::interaction::resolve_story_npc_post_dialogue_hook;
    using oasis::game::player::Direction;

    // 1. Proximity (16 px default, 0x00B852) and facing (0x00E338) gate.
    assert(evaluate_interaction_proximity_and_facing(100, 100, Direction::up, 100, 84));
    assert(!evaluate_interaction_proximity_and_facing(100, 100, Direction::up, 100, 83));
    assert(!evaluate_interaction_proximity_and_facing(100, 100, Direction::down, 100, 84));
    assert(evaluate_interaction_proximity_and_facing(100, 100, Direction::down, 108, 116));
    assert(evaluate_interaction_proximity_and_facing(100, 100, Direction::left, 84, 100));
    assert(!evaluate_interaction_proximity_and_facing(100, 100, Direction::left, 105, 100));
    assert(evaluate_interaction_proximity_and_facing(100, 100, Direction::right, 116, 100));
    assert(!evaluate_interaction_proximity_and_facing(100, 100, Direction::none, 100, 100));

    // 2. Entity-type routing (0x008330).
    const auto r_town = dispatch_interaction_by_type(0x0032U);
    assert(r_town.route == InteractionRoute::TownNpc0032);
    assert(r_town.handler_pc == 0x00018BBCU);

    const auto r_spirit = dispatch_interaction_by_type(0x0020U);
    assert(r_spirit.route == InteractionRoute::SpiritAwakening0020);
    assert(r_spirit.handler_pc == 0x00018CAAU);

    for (const std::uint16_t story_type : {std::uint16_t{0x0094U}, std::uint16_t{0x0096U}, std::uint16_t{0x00BEU}}) {
        const auto r_story = dispatch_interaction_by_type(story_type);
        assert(r_story.route == InteractionRoute::StoryNpc018EBE);
        assert(r_story.handler_pc == 0x00018EBEU);
        assert(r_story.reads_message_from_a0_96);
    }

    const auto r_sign = dispatch_interaction_by_type(0x00B0U);
    assert(r_sign.route == InteractionRoute::Signpost00B0);
    assert(r_sign.handler_pc == 0x0000839EU);
    assert(r_sign.sfx_cmd == 0x00A00264U);
    assert(r_sign.player_freeze_state == 0x003FU);
    assert(r_sign.dialogue_handler_pc == 0x0000C2ECU);

    const auto r_none = dispatch_interaction_by_type(0x0048U);
    assert(r_none.route == InteractionRoute::None);
    assert(r_none.handler_pc == 0U);

    // 3. Story NPC post-dialogue hooks (0x018EBE).
    assert(resolve_story_npc_post_dialogue_hook(0x0024U) == InteractionDispatcherConfig::kHookMsg24Rom);
    assert(resolve_story_npc_post_dialogue_hook(0x002FU) == InteractionDispatcherConfig::kHookMsg2FRom);
    assert(resolve_story_npc_post_dialogue_hook(0x0030U) == InteractionDispatcherConfig::kHookMsg30Rom);
    assert(resolve_story_npc_post_dialogue_hook(0x0039U) == InteractionDispatcherConfig::kHookMsg39Rom);
    assert(resolve_story_npc_post_dialogue_hook(0x0005U) == 0U);

    return 0;
}
