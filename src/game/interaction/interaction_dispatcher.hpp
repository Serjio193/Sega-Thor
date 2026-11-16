#pragma once

#include "game/player/player.hpp"

#include <cstdint>

namespace oasis::game::interaction {

// ROM 0x008330..0x0083D2: Universal Interaction Dispatcher routes by entity type ($0(A0)).
enum class InteractionRoute : std::uint8_t {
    None = 0,
    TownNpc0032,        // 0x0032 -> 0x018BBC (Town NPCs / Village Elder)
    SpiritAwakening0020,// 0x0020 -> 0x018CAA (Spirit Awakening Dispatcher)
    StoryNpc018EBE,     // 0x0094, 0x0096, 0x00BE -> 0x018EBE (Story NPCs, msg $96(A0))
    Signpost00B0,       // 0x00B0 -> 0x00839E (Signpost / Tablet)
};

struct InteractionDispatchResult {
    InteractionRoute route{InteractionRoute::None};
    std::uint32_t handler_pc{0U};
    std::uint32_t sfx_cmd{0U};
    std::uint16_t player_freeze_state{0U};
    std::uint32_t dialogue_handler_pc{0U};
    bool reads_message_from_a0_96{false};
};

struct InteractionDispatcherConfig {
    static constexpr std::uint32_t kDispatcherRom = 0x00008330U;
    static constexpr std::uint32_t kProximityHelperRom = 0x0000B852U;
    static constexpr std::uint32_t kFacingHelperRom = 0x0000E338U;
    static constexpr std::int32_t kDefaultProximityRadiusPx = 16;

    // Route handler PCs
    static constexpr std::uint32_t kTownNpcHandlerRom = 0x00018BBCU;
    static constexpr std::uint32_t kSpiritAwakeningHandlerRom = 0x00018CAAU;
    static constexpr std::uint32_t kStoryNpcHandlerRom = 0x00018EBEU;
    static constexpr std::uint32_t kSignpostHandlerRom = 0x0000839EU;

    // Signpost 0x00B0 contract (0x00839E)
    static constexpr std::uint32_t kSignpostSfx = 0x00A00264U;
    static constexpr std::uint16_t kSignpostFreezeState = 0x003FU;
    static constexpr std::uint32_t kSignpostDialogueRom = 0x0000C2ECU;

    // Story NPC post-dialogue hooks (0x018EBE)
    static constexpr std::uint32_t kHookMsg24Rom = 0x00018FAAU;
    static constexpr std::uint32_t kHookMsg2FRom = 0x00018FF0U;
    static constexpr std::uint32_t kHookMsg30Rom = 0x0001903CU;
    static constexpr std::uint32_t kHookMsg39Rom = 0x00019048U;
};

// ROM 0x008330: Evaluates 16 px proximity gate (0x00B852) and player facing check (0x00E338).
// Returns true iff |target_x - px| <= radius && |target_y - py| <= radius and `facing` points
// toward (or onto) the target.
[[nodiscard]] bool evaluate_interaction_proximity_and_facing(
    std::int32_t px,
    std::int32_t py,
    player::Direction facing,
    std::int32_t target_x,
    std::int32_t target_y,
    std::int32_t radius = InteractionDispatcherConfig::kDefaultProximityRadiusPx) noexcept;

// ROM 0x008330..0x0083D2: Routes entity type `$0(A0)` to its interaction handler.
[[nodiscard]] InteractionDispatchResult dispatch_interaction_by_type(
    std::uint16_t entity_type) noexcept;

// ROM 0x018EBE: Returns the post-dialogue hook PC for Story NPCs (`$96(A0)` message ID),
// or 0 if `message_id` has no special post-dialogue hook.
[[nodiscard]] std::uint32_t resolve_story_npc_post_dialogue_hook(
    std::uint16_t message_id) noexcept;

} // namespace oasis::game::interaction
