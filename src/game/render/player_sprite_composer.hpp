#pragma once

#include "game/render/sat_pipeline_contract.hpp"
#include <cstddef>
#include <cstdint>

namespace oasis::game::render {

// Dispatches authentic Beyond Oasis player Ali sprite pieces into the SAT pipeline
// for each of the 8 walk animation poses reconstructed from ROM routines 0x00A342..0x00A608.
class PlayerSpriteComposer {
public:
    static void emit_player_pieces(
        SatPipelineContract& sat,
        int screen_x,
        int screen_y,
        std::size_t pose_index,
        bool facing_left = false
    );
};

} // namespace oasis::game::render
