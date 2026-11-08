#include "game/render/sat_pipeline_contract.hpp"

#include <cassert>
#include <iostream>

namespace {

void test_initial_state() {
    oasis::game::render::SatPipelineContract pipeline;
    assert(pipeline.active_count() == 0);
    const auto summary = pipeline.finalize_frame();
    assert(summary.active_count == 0);
    assert(summary.buffer_bytes == 0);
    assert(!summary.is_dirty);
    std::cout << "[PASS] test_initial_state\n";
}

void test_single_piece_packing() {
    oasis::game::render::SatPipelineContract pipeline;
    // Emit piece at screen (100, 50), 2x3 tiles, tile index 0x120, palette 2, high priority, h_flip
    const bool ok = pipeline.emit_piece(100, 50, 2, 3, 0x120, 2, true, true, false);
    assert(ok);
    assert(pipeline.active_count() == 1);

    const auto summary = pipeline.finalize_frame();
    assert(summary.active_count == 1);
    assert(summary.buffer_bytes == 8);
    assert(summary.is_dirty);

    const auto entries = pipeline.entries();
    assert(entries.size() == 1);
    const auto& e0 = entries[0];

    // VDP Y = screen Y + 128 = 50 + 128 = 178
    assert(e0.y_pos == 178);
    // Size = ((2 - 1) << 2) | (3 - 1) = (1 << 2) | 2 = 6
    assert(e0.size == 6);
    // Link field should be terminated to 0 by finalize_frame()
    assert(e0.link == 0);
    // Attr: tile=0x120, h_flip (bit 11: 0x0800), palette=2 (bits 14..13: 2 << 13 = 0x4000), priority (bit 15: 0x8000)
    // 0x120 | 0x0800 | 0x4000 | 0x8000 = 0xC920
    assert(e0.attr == 0xC920);
    // VDP X = screen X + 128 = 100 + 128 = 228
    assert(e0.x_pos == 228);

    std::cout << "[PASS] test_single_piece_packing\n";
}

void test_link_chain_traversal() {
    oasis::game::render::SatPipelineContract pipeline;
    // Emit 3 pieces
    assert(pipeline.emit_piece(10, 10, 1, 1, 0x10, 0, false, false, false));
    assert(pipeline.emit_piece(20, 20, 2, 2, 0x20, 1, false, false, false));
    assert(pipeline.emit_piece(30, 30, 3, 3, 0x30, 2, false, false, false));
    assert(pipeline.active_count() == 3);

    pipeline.finalize_frame();
    const auto entries = pipeline.entries();
    assert(entries.size() == 3);

    // Entry 0 links to entry 1
    assert(entries[0].link == 1);
    // Entry 1 links to entry 2
    assert(entries[1].link == 2);
    // Entry 2 is the tail, link must be 0
    assert(entries[2].link == 0);

    std::cout << "[PASS] test_link_chain_traversal\n";
}

void test_clipping_culling() {
    oasis::game::render::SatPipelineContract pipeline;
    // Completely off-screen left: screen X = -40, width = 2 tiles (16px) -> -40 + 16 = -24 <= 0
    assert(!pipeline.emit_piece(-40, 50, 2, 2, 0x10, 0, false, false, false));
    // Completely off-screen top: screen Y = -30, height = 2 tiles (16px) -> -30 + 16 = -14 <= 0
    assert(!pipeline.emit_piece(50, -30, 2, 2, 0x10, 0, false, false, false));
    // Completely off-screen right: screen X = 320 >= 320
    assert(!pipeline.emit_piece(320, 50, 1, 1, 0x10, 0, false, false, false));
    // Completely off-screen bottom: screen Y = 224 >= 224
    assert(!pipeline.emit_piece(50, 224, 1, 1, 0x10, 0, false, false, false));

    // Partially visible piece: screen X = -8, width = 2 tiles (16px) -> -8 + 16 = +8 > 0 -> should be accepted!
    assert(pipeline.emit_piece(-8, 50, 2, 2, 0x10, 0, false, false, false));
    assert(pipeline.active_count() == 1);

    std::cout << "[PASS] test_clipping_culling\n";
}

void test_capacity_limit_80() {
    oasis::game::render::SatPipelineContract pipeline;
    for (int i = 0; i < 80; ++i) {
        const bool ok = pipeline.emit_piece(i * 2, i * 2, 1, 1, static_cast<std::uint16_t>(i), 0, false, false, false);
        assert(ok);
    }
    assert(pipeline.active_count() == 80);

    // 81st piece must be rejected
    const bool overflow = pipeline.emit_piece(100, 100, 1, 1, 0x99, 0, false, false, false);
    assert(!overflow);
    assert(pipeline.active_count() == 80);

    const auto summary = pipeline.finalize_frame();
    assert(summary.active_count == 80);
    assert(summary.buffer_bytes == 640);
    assert(pipeline.entries()[79].link == 0);

    std::cout << "[PASS] test_capacity_limit_80\n";
}

void test_shadow_emission() {
    oasis::game::render::SatPipelineContract pipeline;
    const bool ok = pipeline.emit_shadow(120, 150, 0x050, 3);
    assert(ok);
    assert(pipeline.active_count() == 1);

    const auto entries = pipeline.entries();
    const auto& sh = entries[0];
    // Shadow is 2x1 tiles: size = ((2-1)<<2) | (1-1) = 4
    assert(sh.size == 4);
    // Palette 3, low priority, no flip
    assert((sh.attr >> 13) == 3);
    assert((sh.attr & 0x8000) == 0);

    std::cout << "[PASS] test_shadow_emission\n";
}

} // namespace

int main() {
    test_initial_state();
    test_single_piece_packing();
    test_link_chain_traversal();
    test_clipping_culling();
    test_capacity_limit_80();
    test_shadow_emission();

    std::cout << "\nALL SAT PIPELINE CONTRACT TESTS PASSED!\n";
    return 0;
}
