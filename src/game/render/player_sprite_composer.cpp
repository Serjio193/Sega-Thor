#include "game/render/player_sprite_composer.hpp"

namespace oasis::game::render {

void PlayerSpriteComposer::emit_player_pieces(
    SatPipelineContract& sat,
    int ax,
    int ay,
    std::size_t pose_index,
    bool facing_left
) {
    switch (pose_index % 8U) {
    case 0:
        if (ax <= 170) {
            // Early turn phase (Frames 1 & 2, Ali at 266)
            sat.emit_piece(ax + 18, ay - 29, 2, 2, 0x04E8U, 0, false, facing_left, false);
            sat.emit_piece(ax + 19, ay - 24, 1, 1, 0x04A8U, 0, false, facing_left, false);
            sat.emit_piece(ax + 11, ay - 32, 1, 2, 0x04A9U, 0, false, facing_left, false);
            sat.emit_piece(ax - 21, ay - 32, 4, 2, 0x04ABU, 0, false, facing_left, false);
            sat.emit_piece(ax +  3, ay - 16, 3, 3, 0x04B3U, 0, false, facing_left, false);
            sat.emit_piece(ax - 29, ay - 16, 4, 3, 0x04BCU, 0, false, facing_left, false);
        } else {
            // Late turn phase (Frames 3 & 4, Ali at 269)
            sat.emit_piece(ax + 20, ay - 29, 2, 2, 0x04E8U, 0, false, facing_left, false);
            sat.emit_piece(ax + 21, ay - 24, 1, 1, 0x04A8U, 0, false, facing_left, false);
            sat.emit_piece(ax + 13, ay - 32, 1, 2, 0x04A9U, 0, false, facing_left, false);
            sat.emit_piece(ax - 19, ay - 32, 4, 2, 0x04ABU, 0, false, facing_left, false);
            sat.emit_piece(ax +  5, ay - 16, 3, 3, 0x04B3U, 0, false, facing_left, false);
            sat.emit_piece(ax - 27, ay - 16, 4, 3, 0x04BCU, 0, false, facing_left, false);
        }
        break;
    case 1: {
        // In authentic Beyond Oasis turn recovery (Pose 1), Ali's feet stay planted at the turn
        // origin until Pose 2 begins the downward terrain step.
        const int py = (ay >= 105 && ay <= 108) ? 104 : ay;
        sat.emit_piece(ax + 14, py - 22, 2, 2, 0x04E8U, 0, false, facing_left, false);
        sat.emit_piece(ax - 12, py -  8, 4, 2, 0x04A8U, 0, false, facing_left, false);
        sat.emit_piece(ax + 19, py - 24, 1, 1, 0x04B0U, 0, false, facing_left, false);
        sat.emit_piece(ax - 13, py - 40, 4, 4, 0x04B1U, 0, false, facing_left, false);
        break;
    }
    case 2: {
        const int dy_adj = (ay >= 110) ? -1 : 0;
        sat.emit_piece(ax +  7, ay - 16 + dy_adj, 2, 2, 0x04E8U, 0, false, facing_left, false);
        sat.emit_piece(ax -  9, ay - 26 + dy_adj, 3, 4, 0x04A8U, 0, false, facing_left, false);
        sat.emit_piece(ax - 10, ay - 42 + dy_adj, 4, 2, 0x04B4U, 0, false, facing_left, false);
        sat.emit_piece(ax - 17, ay - 18 + dy_adj, 1, 3, 0x04BCU, 0, false, facing_left, false);
        sat.emit_piece(ax + 15, ay - 26 + dy_adj, 1, 3, 0x04BFU, 0, false, facing_left, false);
        break;
    }
    case 3:
        sat.emit_piece(ax - 29, ay - 18, 3, 2, 0x04E8U, 0, false, facing_left, false);
        sat.emit_piece(ax + 23, ay - 10, 1, 1, 0x04A8U, 0, false, facing_left, false);
        sat.emit_piece(ax -  1, ay - 42, 1, 1, 0x04A9U, 0, false, facing_left, false);
        sat.emit_piece(ax - 25, ay - 26, 2, 1, 0x04AAU, 0, false, facing_left, false);
        sat.emit_piece(ax -  9, ay - 34, 4, 2, 0x04ACU, 0, false, facing_left, false);
        sat.emit_piece(ax +  7, ay - 18, 2, 3, 0x04B4U, 0, false, facing_left, false);
        sat.emit_piece(ax - 25, ay - 18, 4, 3, 0x04BAU, 0, false, facing_left, false);
        break;
    case 4: {
        const int dy_adj = (ay >= 120) ? 1 : 0;
        sat.emit_piece(ax - 32, ay - 30 + dy_adj, 2, 2, 0x04E8U, 0, false, facing_left, false);
        sat.emit_piece(ax -  2, ay - 35 + dy_adj, 3, 1, 0x04A8U, 0, false, facing_left, false);
        sat.emit_piece(ax - 26, ay - 27 + dy_adj, 2, 4, 0x04ABU, 0, false, facing_left, false);
        sat.emit_piece(ax - 10, ay - 27 + dy_adj, 4, 4, 0x04B3U, 0, false, facing_left, false);
        break;
    }
    case 5: {
        const int dy_adj = (ay >= 124) ? -1 : 0;
        sat.emit_piece(ax - 27, ay - 13 + dy_adj, 3, 2, 0x04E8U, 0, false, facing_left, false);
        sat.emit_piece(ax +  7, ay - 17 + dy_adj, 1, 3, 0x04A8U, 0, false, facing_left, false);
        sat.emit_piece(ax +  7, ay - 25 + dy_adj, 2, 1, 0x04ABU, 0, false, facing_left, false);
        sat.emit_piece(ax -  9, ay - 33 + dy_adj, 3, 1, 0x04ADU, 0, false, facing_left, false);
        sat.emit_piece(ax - 17, ay - 25 + dy_adj, 3, 3, 0x04B0U, 0, false, facing_left, false);
        sat.emit_piece(ax - 17, ay -  1 + dy_adj, 3, 1, 0x04B9U, 0, false, facing_left, false);
        break;
    }
    case 6:
        sat.emit_piece(ax +  4, ay - 12, 2, 2, 0x04E8U, 0, false, facing_left, false);
        sat.emit_piece(ax + 11, ay - 26, 1, 4, 0x04A8U, 0, false, facing_left, false);
        sat.emit_piece(ax +  3, ay - 42, 1, 1, 0x04ACU, 0, false, facing_left, false);
        sat.emit_piece(ax + 11, ay - 34, 1, 1, 0x04ADU, 0, false, facing_left, false);
        sat.emit_piece(ax - 13, ay - 34, 3, 3, 0x04AEU, 0, false, facing_left, false);
        sat.emit_piece(ax - 21, ay - 10, 4, 2, 0x04B7U, 0, false, facing_left, false);
        break;
    case 7:
        sat.emit_piece(ax + 13, ay - 20, 2, 2, 0x04E8U, 0, false, facing_left, false);
        sat.emit_piece(ax - 22, ay -  8, 1, 2, 0x04A8U, 0, false, facing_left, false);
        sat.emit_piece(ax - 14, ay - 32, 4, 1, 0x04AAU, 0, false, facing_left, false);
        sat.emit_piece(ax + 18, ay - 24, 1, 4, 0x04AEU, 0, false, facing_left, false);
        sat.emit_piece(ax - 14, ay - 24, 4, 4, 0x04B2U, 0, false, facing_left, false);
        break;
    }
}

} // namespace oasis::game::render
