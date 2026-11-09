#pragma once

#include "game/render/sat_pipeline_contract.hpp"
#include <cstddef>
#include <cstdint>

namespace oasis::game::render {

// Dispatches authentic Beyond Oasis player Ali sprite pieces into the SAT pipeline
// for each of the 8 walk animation poses reconstructed from ROM routines 0x00A342..0x00A608,
// and attack weapon piece attachments from ROM routine 0x005B3C..0x005BF0.
class PlayerSpriteComposer {
public:
    static void emit_player_pieces(
        SatPipelineContract& sat,
        int screen_x,
        int screen_y,
        std::size_t pose_index,
        bool facing_left = false,
        bool alt_step = false
    );

    static void emit_attack_pieces(
        SatPipelineContract& sat,
        int screen_x,
        int screen_y,
        std::uint16_t direction_code
    );
};

} // namespace oasis::game::render
