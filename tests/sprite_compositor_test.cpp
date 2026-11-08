#include "game/render/sprite_compositor.hpp"

#include <cassert>
#include <iostream>
#include <vector>

namespace {

using oasis::game::render::SpriteCompositor;
using oasis::game::render::SpritePiece;
using oasis::game::render::SATRecord;

void test_mode0_composition() {
    SpriteCompositor compositor;
    compositor.set_camera(100, 50);

    const std::vector<SpritePiece> pieces = {
        {.offset_x = 0, .offset_y = 0, .width_tiles = 3, .height_tiles = 4, .tile_index = 0x0100, .palette = 0, .priority = true},
    };

    assert(compositor.compose_mode0(200, 150, pieces, false, false));
    assert(compositor.active_count() == 1);

    const auto summary = compositor.finalize_frame();
    assert(summary.active_count == 1);
    assert(summary.buffer_bytes == 8);

    const auto records = compositor.records();
    assert(records[0].screen_x() == 100); // 200 - 100
    assert(records[0].screen_y() == 100); // 150 - 50
    assert(records[0].width_tiles() == 3);
    assert(records[0].height_tiles() == 4);
    assert(records[0].link_target() == 0);

    std::cout << "[PASS] test_mode0_composition\n";
}

void test_mode1_absolute_and_relative() {
    SpriteCompositor compositor;
    compositor.set_camera(100, 50);

    const std::vector<SpritePiece> pieces = {
        {.offset_x = 10, .offset_y = 20, .width_tiles = 2, .height_tiles = 2, .tile_index = 0x0200, .palette = 1, .priority = true},
    };

    // Mode 1 absolute coordinates (e.g. HUD / overlay)
    assert(compositor.compose_mode1(50, 40, pieces, false, false, true));
    assert(compositor.active_count() == 1);
    assert(compositor.records()[0].screen_x() == 60);
    assert(compositor.records()[0].screen_y() == 60);

    // Mode 1 relative coordinates (e.g. attached weapon)
    assert(compositor.compose_mode1(200, 150, pieces, false, false, false));
    assert(compositor.active_count() == 2);
    assert(compositor.records()[1].screen_x() == 110); // 200 - 100 + 10
    assert(compositor.records()[1].screen_y() == 120); // 150 - 50 + 20

    compositor.finalize_frame();
    assert(compositor.records()[0].link_target() == 1);
    assert(compositor.records()[1].link_target() == 0);

    std::cout << "[PASS] test_mode1_absolute_and_relative\n";
}

void test_clipping_and_shadow() {
    SpriteCompositor compositor;
    compositor.set_camera(0, 0);

    // Completely off-screen sprite
    const std::vector<SpritePiece> offscreen = {
        {.offset_x = 0, .offset_y = 0, .width_tiles = 2, .height_tiles = 2, .tile_index = 0x0010},
    };
    assert(!compositor.compose_mode0(-100, -100, offscreen, false, false));
    assert(compositor.active_count() == 0);

    // Shadow piece
    assert(compositor.emit_shadow(120, 80, 0x0050, 0));
    assert(compositor.active_count() == 1);
    assert(!compositor.records()[0].priority());
    assert(compositor.records()[0].width_tiles() == 2);
    assert(compositor.records()[0].height_tiles() == 1);

    std::cout << "[PASS] test_clipping_and_shadow\n";
}

} // namespace

int main() {
    test_mode0_composition();
    test_mode1_absolute_and_relative();
    test_clipping_and_shadow();
    std::cout << "All SpriteCompositor tests passed successfully!\n";
    return 0;
}
