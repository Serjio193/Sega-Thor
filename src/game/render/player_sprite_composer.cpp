#include "game/render/player_sprite_composer.hpp"

namespace oasis::game::render {

namespace {
inline void emit_piece_dir(SatPipelineContract& sat, int ax, int ay, int dx, int dy,
                           std::uint8_t w, std::uint8_t h, std::uint16_t tile, std::uint8_t pal,
                           bool priority, bool facing_left) {
    const int px = facing_left ? (ax - dx - static_cast<int>(w) * 8) : (ax + dx);
    sat.emit_piece(px, ay + dy, w, h, tile, pal, priority, facing_left, false);
}
} // namespace

void PlayerSpriteComposer::emit_player_pieces(
    SatPipelineContract& sat,
    int ax,
    int ay,
    std::size_t pose_index,
    bool facing_left,
    bool alt_step
) {
    switch (pose_index % 8U) {
    case 0:
        if (ax <= 170) {
            // Early turn phase (Frames 1 & 2, Ali at 266)
            emit_piece_dir(sat, ax, ay,  18, -29, 2, 2, 0x04E8U, 0, false, facing_left);
            emit_piece_dir(sat, ax, ay,  19, -24, 1, 1, 0x04A8U, 0, false, facing_left);
            emit_piece_dir(sat, ax, ay,  11, -32, 1, 2, 0x04A9U, 0, false, facing_left);
            emit_piece_dir(sat, ax, ay, -21, -32, 4, 2, 0x04ABU, 0, false, facing_left);
            emit_piece_dir(sat, ax, ay,   3, -16, 3, 3, 0x04B3U, 0, false, facing_left);
            emit_piece_dir(sat, ax, ay, -29, -16, 4, 3, 0x04BCU, 0, false, facing_left);
        } else {
            // Late turn phase (Frames 3 & 4, Ali at 269)
            emit_piece_dir(sat, ax, ay,  20, -29, 2, 2, 0x04E8U, 0, false, facing_left);
            emit_piece_dir(sat, ax, ay,  21, -24, 1, 1, 0x04A8U, 0, false, facing_left);
            emit_piece_dir(sat, ax, ay,  13, -32, 1, 2, 0x04A9U, 0, false, facing_left);
            emit_piece_dir(sat, ax, ay, -19, -32, 4, 2, 0x04ABU, 0, false, facing_left);
            emit_piece_dir(sat, ax, ay,   5, -16, 3, 3, 0x04B3U, 0, false, facing_left);
            emit_piece_dir(sat, ax, ay, -27, -16, 4, 3, 0x04BCU, 0, false, facing_left);
        }
        break;
    case 1: {
        const int py = (ay >= 105 && ay <= 108) ? 104 : ay;
        emit_piece_dir(sat, ax, py,  14, -22, 2, 2, 0x04E8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, py, -12,  -8, 4, 2, 0x04A8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, py,  19, -24, 1, 1, 0x04B0U, 0, false, facing_left);
        emit_piece_dir(sat, ax, py, -13, -40, 4, 4, 0x04B1U, 0, false, facing_left);
        break;
    }
    case 2: {
        const int dy_adj = (ay >= 110) ? -1 : 0;
        emit_piece_dir(sat, ax, ay,   7, -16 + dy_adj, 2, 2, 0x04E8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,  -9, -26 + dy_adj, 3, 4, 0x04A8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -10, -42 + dy_adj, 4, 2, 0x04B4U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -17, -18 + dy_adj, 1, 3, 0x04BCU, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,  15, -26 + dy_adj, 1, 3, 0x04BFU, 0, false, facing_left);
        break;
    }
    case 3:
        emit_piece_dir(sat, ax, ay, -29, -18, 3, 2, 0x04E8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,  23, -10, 1, 1, 0x04A8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,  -1, -42, 1, 1, 0x04A9U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -25, -26, 2, 1, 0x04AAU, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,  -9, -34, 4, 2, 0x04ACU, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,   7, -18, 2, 3, 0x04B4U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -25, -18, 4, 3, 0x04BAU, 0, false, facing_left);
        break;
    case 4: {
        const int dy_adj = alt_step ? 1 : 0;
        emit_piece_dir(sat, ax, ay, -32, -30 + dy_adj, 2, 2, 0x04E8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,  -2, -35 + dy_adj, 3, 1, 0x04A8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -26, -27 + dy_adj, 2, 4, 0x04ABU, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -10, -27 + dy_adj, 4, 4, 0x04B3U, 0, false, facing_left);
        break;
    }
    case 5: {
        const int dy_adj = alt_step ? -1 : 0;
        emit_piece_dir(sat, ax, ay, -27, -13 + dy_adj, 3, 2, 0x04E8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,   7, -17 + dy_adj, 1, 3, 0x04A8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,   7, -25 + dy_adj, 2, 1, 0x04ABU, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,  -9, -33 + dy_adj, 3, 1, 0x04ADU, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -17, -25 + dy_adj, 3, 3, 0x04B0U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -17,  -1 + dy_adj, 3, 1, 0x04B9U, 0, false, facing_left);
        break;
    }
    case 6:
        emit_piece_dir(sat, ax, ay,   4, -12, 2, 2, 0x04E8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,  11, -26, 1, 4, 0x04A8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,   3, -42, 1, 1, 0x04ACU, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,  11, -34, 1, 1, 0x04ADU, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -13, -34, 3, 3, 0x04AEU, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -21, -10, 4, 2, 0x04B7U, 0, false, facing_left);
        break;
    case 7:
        emit_piece_dir(sat, ax, ay,  13, -20, 2, 2, 0x04E8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -22,  -8, 1, 2, 0x04A8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -14, -32, 4, 1, 0x04AAU, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay,  18, -24, 1, 4, 0x04AEU, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, -14, -24, 4, 4, 0x04B2U, 0, false, facing_left);
        break;
    }
}

void PlayerSpriteComposer::emit_attack_pieces(
    SatPipelineContract& sat,
    int ax,
    int ay,
    std::uint16_t direction_code
) {
    const bool facing_left = (direction_code == 3U);
    if (direction_code == 0U) {
        // Attack Up
        emit_piece_dir(sat, ax, ay, -8, -28, 3, 4, 0x04A8U, 0, false, false);
        emit_piece_dir(sat, ax, ay,  0, -36, 2, 2, 0x04F0U, 0, false, false);
    } else if (direction_code == 2U) {
        // Attack Down
        emit_piece_dir(sat, ax, ay, -8, -28, 3, 4, 0x04A8U, 0, false, false);
        emit_piece_dir(sat, ax, ay,  0,  -4, 2, 2, 0x04F0U, 0, false, false);
    } else {
        // Attack Right or Left (mirrored)
        emit_piece_dir(sat, ax, ay, -8, -28, 3, 4, 0x04A8U, 0, false, facing_left);
        emit_piece_dir(sat, ax, ay, 18, -12, 2, 2, 0x04F0U, 0, false, facing_left);
    }
}

} // namespace oasis::game::render
