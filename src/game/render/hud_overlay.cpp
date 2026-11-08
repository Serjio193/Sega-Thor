#include "game/render/hud_overlay.hpp"

namespace oasis::game::render {

bool HudOverlay::emit_hud(SatPipelineContract& sat,
                          int hp, int max_hp,
                          int sp, int max_sp) noexcept {
    (void)hp;
    (void)max_hp;
    (void)sp;
    (void)max_sp;

    // Routine 0x003FA0 authentic emission sequence:
    // Slot 0: Health gauge base frame: (8, 8), 3x2 tiles (24x16 px), tile 0x0781, palette 0, priority true
    const bool s0 = sat.emit_piece(8, 8, 3, 2, kHealthFrameTile, 0, true, false, false);

    // Slot 1: Health bar upper/middle: (16, 24), 1x4 tiles (8x32 px), tile 0x0787, palette 0, priority true
    const bool s1 = sat.emit_piece(16, 24, 1, 4, kHealthBarPart1Tile, 0, true, false, false);

    // Slot 2: Health bar lower/tail: (16, 56), 1x3 tiles (8x24 px), tile 0x078B, palette 0, priority true
    const bool s2 = sat.emit_piece(16, 56, 1, 3, kHealthBarPart2Tile, 0, true, false, false);

    // Slot 3: Magic gauge base frame: (8, 80), 3x2 tiles (24x16 px), tile 0x078E, palette 0, priority true
    const bool s3 = sat.emit_piece(8, 80, 3, 2, kMagicFrameTile, 0, true, false, false);

    // Slot 4: Magic bar upper/middle: (16, 96), 1x4 tiles (8x32 px), tile 0x0794, palette 0, priority true
    const bool s4 = sat.emit_piece(16, 96, 1, 4, kMagicBarPart1Tile, 0, true, false, false);

    // Slot 5: Magic bar lower/tail: (16, 128), 1x3 tiles (8x24 px), tile 0x0798, palette 0, priority true
    const bool s5 = sat.emit_piece(16, 128, 1, 3, kMagicBarPart2Tile, 0, true, false, false);

    return s0 && s1 && s2 && s3 && s4 && s5;
}

} // namespace oasis::game::render
