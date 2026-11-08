#include "game/render/sat_pipeline_contract.hpp"

#include <cassert>
#include <iostream>

namespace {

using oasis::game::render::SatPipelineContract;

// Case 1: Normal single-piece player sprite (3x4 tiles, high priority, palette 0)
void test_case1_normal_player_sprite() {
    SatPipelineContract pipeline;
    const bool ok = pipeline.emit_piece(148, 96, 3, 4, 0x0280, 0, true, false, false);
    assert(ok);
    assert(pipeline.active_count() == 1);

    const auto summary = pipeline.finalize_frame();
    assert(summary.active_count == 1);
    assert(summary.buffer_bytes == 8);

    const auto entries = pipeline.entries();
    assert(entries.size() == 1);
    assert(entries[0].y_pos == 96 + 128); // 224
    assert(entries[0].size == (((3 - 1) << 2) | (4 - 1))); // (2 << 2) | 3 = 11
    assert(entries[0].link == 0); // terminated
    assert(entries[0].attr == (0x0280 | 0x8000)); // prio 1, pal 0
    assert(entries[0].x_pos == 148 + 128); // 276

    std::cout << "[PASS] test_case1_normal_player_sprite\n";
}

// Case 2: Player sprite with attached weapon sprite in consecutive SAT slots with link chaining
void test_case2_weapon_attachment() {
    SatPipelineContract pipeline;
    // Player body
    assert(pipeline.emit_piece(148, 96, 3, 4, 0x0280, 0, true, false, false));
    // Attached weapon
    assert(pipeline.emit_piece(168, 104, 2, 2, 0x0310, 1, true, false, false));
    assert(pipeline.active_count() == 2);

    pipeline.finalize_frame();
    const auto entries = pipeline.entries();
    assert(entries.size() == 2);

    // Entry 0 (player) links to entry 1 (weapon)
    assert(entries[0].link == 1);
    assert(entries[0].attr == (0x0280 | 0x8000));

    // Entry 1 (weapon) terminates chain
    assert(entries[1].y_pos == 104 + 128);
    assert(entries[1].size == (((2 - 1) << 2) | (2 - 1))); // 5
    assert(entries[1].link == 0);
    assert(entries[1].attr == (0x0310 | (1 << 13) | 0x8000)); // pal 1, prio 1
    assert(entries[1].x_pos == 168 + 128);

    std::cout << "[PASS] test_case2_weapon_attachment\n";
}

// Case 3: Horizontal flip validation for west-facing player sprite
void test_case3_horizontal_flip() {
    SatPipelineContract pipeline;
    assert(pipeline.emit_piece(130, 96, 3, 4, 0x0280, 0, true, true, false));
    pipeline.finalize_frame();

    const auto entries = pipeline.entries();
    assert(entries.size() == 1);
    // H-flip bit 11 (0x0800), priority bit 15 (0x8000)
    assert(entries[0].attr == (0x0280 | 0x0800 | 0x8000));
    assert(entries[0].x_pos == 130 + 128);

    std::cout << "[PASS] test_case3_horizontal_flip\n";
}

// Case 4: Multi-piece composite entity descriptor emitting 4 linked SAT entries
void test_case4_multi_piece_descriptors() {
    SatPipelineContract pipeline;
    assert(pipeline.emit_piece(100, 50, 4, 4, 0x0400, 2, false, false, false));
    assert(pipeline.emit_piece(132, 50, 4, 4, 0x0410, 2, false, false, false));
    assert(pipeline.emit_piece(100, 82, 4, 4, 0x0420, 2, false, false, false));
    assert(pipeline.emit_piece(132, 82, 4, 4, 0x0430, 2, false, false, false));
    assert(pipeline.active_count() == 4);

    pipeline.finalize_frame();
    const auto entries = pipeline.entries();
    assert(entries.size() == 4);
    assert(entries[0].link == 1);
    assert(entries[1].link == 2);
    assert(entries[2].link == 3);
    assert(entries[3].link == 0);

    for (std::size_t i = 0; i < 4; ++i) {
        assert(entries[i].size == (((4 - 1) << 2) | (4 - 1))); // 15
        assert((entries[i].attr >> 13) == 2); // palette 2
    }

    std::cout << "[PASS] test_case4_multi_piece_descriptors\n";
}

// Case 5: Authentic shadow entry generated using ROM table 0x00ACFE (Type 1: 2x1 tiles, offset 8, 3)
void test_case5_shadow_table_00acfe() {
    SatPipelineContract pipeline;
    // Emitted by 0x00AD76 at screen center (160, 120), Type 1 offset is x=8, y=3
    const bool ok = pipeline.emit_shadow(160 - 8, 120 - 3, 0x0549, 3);
    assert(ok);
    pipeline.finalize_frame();

    const auto entries = pipeline.entries();
    assert(entries.size() == 1);
    assert(entries[0].y_pos == (120 - 3) + 128); // 245
    assert(entries[0].size == 4); // 2x1 tiles
    assert(entries[0].link == 0);
    assert(entries[0].attr == 0x6549); // palette 3, tile 0x0549
    assert(entries[0].x_pos == (160 - 8) + 128); // 280

    std::cout << "[PASS] test_case5_shadow_table_00acfe\n";
}

// Case 6: Strict 80-sprite capacity limit with link-terminator and clipping boundary rejection
void test_case6_capacity_clipping_boundary() {
    SatPipelineContract pipeline;
    for (int i = 0; i < 80; ++i) {
        const int x = (i % 20) * 16;
        const int y = (i / 20) * 32;
        const bool ok = pipeline.emit_piece(x, y, 2, 2, static_cast<std::uint16_t>(0x0100 + i), 0, true, false, false);
        assert(ok);
    }
    assert(pipeline.active_count() == 80);

    // 81st piece must be rejected due to capacity limit
    assert(!pipeline.emit_piece(50, 50, 1, 1, 0x0200, 0, false, false, false));

    // Culled pieces outside viewport
    assert(!pipeline.emit_piece(-32, 50, 2, 2, 0x0999, 0, true, false, false));
    assert(!pipeline.emit_piece(320, 50, 1, 1, 0x0999, 0, true, false, false));
    assert(!pipeline.emit_piece(50, -32, 2, 2, 0x0999, 0, true, false, false));
    assert(!pipeline.emit_piece(50, 224, 1, 1, 0x0999, 0, true, false, false));

    const auto summary = pipeline.finalize_frame();
    assert(summary.active_count == 80);
    assert(summary.buffer_bytes == 640);
    assert(pipeline.entries()[79].link == 0);

    std::cout << "[PASS] test_case6_capacity_clipping_boundary\n";
}

} // namespace

int main() {
    test_case1_normal_player_sprite();
    test_case2_weapon_attachment();
    test_case3_horizontal_flip();
    test_case4_multi_piece_descriptors();
    test_case5_shadow_table_00acfe();
    test_case6_capacity_clipping_boundary();

    std::cout << "\nALL 6 SAT ORACLE FIXTURE TESTS PASSED!\n";
    return 0;
}
