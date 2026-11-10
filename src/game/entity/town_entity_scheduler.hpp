#pragma once

#include "game/combat/sound_command_queue.hpp"
#include "game/entity/native_entity.hpp"
#include "game/entity/town_entity_animation.hpp"

#include <cstdint>
#include <span>
#include <vector>

namespace oasis::game::entity {

struct TownSchedulerContext {
    std::uint16_t prng_seed{0x22BCU}; // $FF1928
    std::int32_t player_x{160};
    std::int32_t player_y{488};
    std::int16_t player_z{0};
    std::uint16_t player_room{0};
    std::uint16_t player_state{0};
    std::uint16_t player_facing{0};
    std::int16_t player_hp{208};
    bool player_invisible{false};       // bit 3 of $FF1A20
    std::int16_t pending_player_damage{0};
    int frame_60hz{0};
    bool skip_post_move_repulsion{false};
};

// Executes one full 30Hz ROM 0x00DEEC scheduler pass across all Town entities (Slots 4..19),
// updating physics, 0x002C AI/combat/throw/rooftop-drop, 0x002E projectiles,
// water current/splash (0x02E8C8), solid-body repulsion, and spatial sound dispatch.
void step_town_scheduler(std::vector<NativeEntity>& entities,
                         std::span<const std::uint8_t> rom,
                         std::span<const std::uint8_t> terrain_collision,
                         TownSchedulerContext& ctx,
                         combat::SoundCommandQueue& sound_queue,
                         bool advance_logic_step = true) noexcept;

} // namespace oasis::game::entity