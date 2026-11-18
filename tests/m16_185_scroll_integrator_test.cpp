#include "game/render/scroll_integrator.hpp"

#include <array>
#include <cassert>
#include <cstdint>
#include <iostream>

namespace {

using namespace thor::render;

void test_constants_and_struct_layout() {
    static_assert(kAccumulateLongDeltaAndIntegrateAddr == 0x00D146U);
    static_assert(kAccumulateWordDeltaAndIntegrateAddr == 0x00D168U);
    static_assert(kCheckEnableAndIntegrateAddr == 0x00D17EU);
    static_assert(kIntegrateScrollDeltasAddr == 0x00D188U);
    static_assert(kNormalizeToroidalMapCoordsAddr == 0x00D266U);

    static_assert(kPlaneARamOffset == 0x1716U);
    static_assert(kPlaneBRamOffset == 0x173EU);

    static_assert(kHScrollMask == 0x01FFU);
    static_assert(kVScrollMask == 0x00FFU);

    static_assert(kDirtyBitTileColStrip == 0x01U);
    static_assert(kDirtyBitTileRowStrip == 0x02U);
    static_assert(kDirtyBitPlaneAHScroll == 0x04U);
    static_assert(kDirtyBitPlaneAVScroll == 0x08U);

    static_assert(kControlEnableIntegration == 0x01U);
    static_assert(kControlSuppressTileStripShift == 0x02U);
}

void test_integrate_scroll_deltas_horizontal() {
    TilemapCameraState state{};
    state.cam_x = 100;
    state.cam_y = 50;
    state.vdp_hscroll = 0;
    state.map_width_tiles = 60;
    state.map_height_tiles = 40;
    state.control_flags = kControlEnableIntegration;

    // Moving from 100 to 105:
    // old_tile_col = 100 >> 3 = 12
    // new_tile_col = 105 >> 3 = 13 (col_step = 1)
    // delta_x = +5
    // vdp_hscroll = (0 - 5) & 0x1FF = 507 (0x01FB)
    integrate_scroll_deltas(state, 105, 50);

    assert(state.cam_x == 105);
    assert(state.cam_y == 50);
    assert(state.delta_x == 5);
    assert(state.delta_y == 0);
    assert(state.tile_col_step == 1);
    assert(state.vdp_hscroll == 507);
    assert(state.dirty_flags == (kDirtyBitTileColStrip | kDirtyBitPlaneAHScroll)); // 0x05

    // Small move without tile boundary crossing: 105 to 106 (both >> 3 == 13)
    integrate_scroll_deltas(state, 106, 50);

    assert(state.cam_x == 106);
    assert(state.delta_x == 1);
    assert(state.vdp_hscroll == 506);
    assert(state.dirty_flags == kDirtyBitPlaneAHScroll); // Only HScroll dirty (0x04)
}

void test_integrate_scroll_deltas_vertical() {
    TilemapCameraState state{};
    state.cam_x = 100;
    state.cam_y = 50;
    state.vdp_vscroll = 0;
    state.map_width_tiles = 60;
    state.map_height_tiles = 40;
    state.control_flags = kControlEnableIntegration;

    // Moving from 50 to 57:
    // old_tile_row = 50 >> 3 = 6
    // new_tile_row = 57 >> 3 = 7 (row_step = 1)
    // delta_y = +7
    // vdp_vscroll = (0 + 7) & 0xFF = 7
    integrate_scroll_deltas(state, 100, 57);

    assert(state.cam_x == 100);
    assert(state.cam_y == 57);
    assert(state.delta_x == 0);
    assert(state.delta_y == 7);
    assert(state.tile_row_step == 1);
    assert(state.vdp_vscroll == 7);
    assert(state.dirty_flags == (kDirtyBitTileRowStrip | kDirtyBitPlaneAVScroll)); // 0x0A

    // Small move without tile boundary crossing: 57 to 58 (both >> 3 == 7)
    integrate_scroll_deltas(state, 100, 58);

    assert(state.cam_y == 58);
    assert(state.delta_y == 1);
    assert(state.vdp_vscroll == 8);
    assert(state.dirty_flags == kDirtyBitPlaneAVScroll); // Only VScroll dirty (0x08)
}

void test_suppress_tile_strip_shift() {
    TilemapCameraState state{};
    state.cam_x = 100;
    state.cam_y = 50;
    state.vdp_hscroll = 100;
    state.vdp_vscroll = 50;
    state.control_flags = kControlEnableIntegration | kControlSuppressTileStripShift; // bit 1 set

    // Move crossing tile boundaries in both axes
    integrate_scroll_deltas(state, 120, 70);

    assert(state.cam_x == 120);
    assert(state.cam_y == 70);
    // Tile step bits should NOT be set when bit 1 of control_flags is set
    assert((state.dirty_flags & kDirtyBitTileColStrip) == 0);
    assert((state.dirty_flags & kDirtyBitTileRowStrip) == 0);
    // VDP scroll dirty bits must still be set
    assert((state.dirty_flags & kDirtyBitPlaneAHScroll) != 0);
    assert((state.dirty_flags & kDirtyBitPlaneAVScroll) != 0);
}

void test_coarse_range_limiter() {
    TilemapCameraState state{};
    state.cam_x = 9998;
    state.cam_y = -9998;
    state.map_width_tiles = 40;  // 320 px
    state.map_height_tiles = 28; // 224 px
    state.control_flags = kControlEnableIntegration;

    // Moving cam_x to >= 10000: subtracted by 2 * map_width_px (2 * 320 = 640)
    // 9998 + 10 = 10008 >= 10000 -> 10008 - 640 = 9368
    integrate_scroll_deltas(state, 10008, state.cam_y);
    assert(state.cam_x == 9368);

    // Moving cam_y to <= -10000: added by 2 * map_height_px (2 * 224 = 448)
    // -9998 - 10 = -10008 <= -10000 -> -10008 + 448 = -9560
    integrate_scroll_deltas(state, state.cam_x, -10008);
    assert(state.cam_y == -9560);
}

void test_accumulate_word_delta_and_integrate() {
    TilemapCameraState state{};
    state.cam_x = 100;
    state.cam_y = 100;
    state.control_flags = kControlEnableIntegration;

    accumulate_word_and_integrate(state, 10, -5);
    assert(state.cam_x == 110);
    assert(state.cam_y == 95);
    assert(state.delta_x == 10);
    assert(state.delta_y == -5);

    // When disabled (control_flags bit 0 = 0)
    state.control_flags = 0;
    accumulate_word_and_integrate(state, 20, 20);
    assert(state.cam_x == 110);
    assert(state.cam_y == 95);
}

void test_accumulate_long_delta_and_integrate() {
    TilemapCameraState state{};
    state.cam_x = 100;
    state.cam_sub_x = 0x8000; // 100.5
    state.cam_y = 200;
    state.cam_sub_y = 0xC000; // 200.75
    state.control_flags = kControlEnableIntegration;

    // Add +1.5 to X (0x00018000) and +0.25 to Y (0x00004000)
    // New X: 100.5 + 1.5 = 102.0 (sub_x = 0, cam_x = 102)
    // New Y: 200.75 + 0.25 = 201.0 (sub_y = 0, cam_y = 201)
    accumulate_long_and_integrate(state, 0x00018000, 0x00004000);

    assert(state.cam_x == 102);
    assert(state.cam_sub_x == 0x0000);
    assert(state.cam_y == 201);
    assert(state.cam_sub_y == 0x0000);
    assert(state.delta_x == 2);
    assert(state.delta_y == 1);

    // When disabled
    state.control_flags = 0;
    accumulate_long_and_integrate(state, 0x00050000, 0x00050000);
    assert(state.cam_x == 102);
}

void test_check_enable_and_integrate() {
    TilemapCameraState state{};
    state.cam_x = 100;
    state.cam_y = 100;

    state.control_flags = 0; // Disabled
    assert(!check_enable_and_integrate(state, 120, 120));
    assert(state.cam_x == 100);

    state.control_flags = kControlEnableIntegration; // Enabled
    assert(check_enable_and_integrate(state, 120, 120));
    assert(state.cam_x == 120);
}

void test_normalize_toroidal_coords() {
    TilemapCameraState state{};
    state.map_width_tiles = 40;  // 320 px
    state.map_height_tiles = 28; // 224 px

    // Normal values within [0..size_px) remain unchanged
    state.cam_x = 100;
    state.cam_y = 100;
    normalize_toroidal_coords(state);
    assert(state.cam_x == 100);
    assert(state.cam_y == 100);

    // Positive overflow
    state.cam_x = 700; // 700 - 320 - 320 = 60
    state.cam_y = 500; // 500 - 224 - 224 = 52
    normalize_toroidal_coords(state);
    assert(state.cam_x == 60);
    assert(state.cam_y == 52);

    // Negative underflow
    state.cam_x = -50; // -50 + 320 = 270
    state.cam_y = -30; // -30 + 224 = 194
    normalize_toroidal_coords(state);
    assert(state.cam_x == 270);
    assert(state.cam_y == 194);
}

void test_ram_window_integration() {
    std::array<std::uint8_t, 0x10000> ram{};

    // Initialize Plane A struct at $FF1716
    // cam_x = 100, cam_y = 50, map_width = 40, map_height = 28, control = 1
    ram[kPlaneARamOffset + 0x00] = 0x00; ram[kPlaneARamOffset + 0x01] = 100;
    ram[kPlaneARamOffset + 0x04] = 0x00; ram[kPlaneARamOffset + 0x05] = 50;
    ram[kPlaneARamOffset + 0x0E] = 0x00; ram[kPlaneARamOffset + 0x0F] = 0;   // hscroll = 0
    ram[kPlaneARamOffset + 0x10] = 0x00; ram[kPlaneARamOffset + 0x11] = 0;   // vscroll = 0
    ram[kPlaneARamOffset + 0x20] = 0x00; ram[kPlaneARamOffset + 0x21] = 40;  // 40 tiles
    ram[kPlaneARamOffset + 0x22] = 0x00; ram[kPlaneARamOffset + 0x23] = 28;  // 28 tiles
    ram[kPlaneARamOffset + 0x27] = kControlEnableIntegration;

    accumulate_word_from_ram(ram, kPlaneARamOffset, 5, 7);

    // Validate RAM updates
    TilemapCameraState state = load_state_from_ram(ram, kPlaneARamOffset);
    assert(state.cam_x == 105);
    assert(state.cam_y == 57);
    assert(state.delta_x == 5);
    assert(state.delta_y == 7);
    assert(state.vdp_hscroll == 507); // (0 - 5) & 0x1FF
    assert(state.vdp_vscroll == 7);   // (0 + 7) & 0xFF
    assert(state.dirty_flags == 0x0F); // All 4 bits set (tile crossing in X and Y, and scroll)

    // Toroidal normalization in RAM
    ram[kPlaneARamOffset + 0x00] = 0x01; ram[kPlaneARamOffset + 0x01] = 0x90; // 400 px (> 320)
    normalize_toroidal_from_ram(ram, kPlaneARamOffset);
    state = load_state_from_ram(ram, kPlaneARamOffset);
    assert(state.cam_x == 80); // 400 - 320 = 80
}

} // namespace

int main() {
    test_constants_and_struct_layout();
    test_integrate_scroll_deltas_horizontal();
    test_integrate_scroll_deltas_vertical();
    test_suppress_tile_strip_shift();
    test_coarse_range_limiter();
    test_accumulate_word_delta_and_integrate();
    test_accumulate_long_delta_and_integrate();
    test_check_enable_and_integrate();
    test_normalize_toroidal_coords();
    test_ram_window_integration();

    std::cout << "[PASS] All M16.185 scroll integrator tests succeeded!\n";
    return 0;
}
