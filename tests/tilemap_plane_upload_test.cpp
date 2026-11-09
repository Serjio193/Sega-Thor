#include "game/render/tilemap_plane_upload.hpp"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
    using namespace oasis::render;

    // Case 1: Scroll up row upload
    {
        std::vector<uint16_t> tiles(4096);
        for (size_t i = 0; i < tiles.size(); ++i) tiles[i] = static_cast<uint16_t>(0x0100 + i);

        PlaneUploadState state{};
        state.vram_base = 0xC000U;
        state.scroll_counter = 2;
        state.vram_offset = 0;
        state.plane_width = 0x80U;
        state.plane_height = 0x20U;
        state.x_pos = 4;
        state.y_pos = 5;
        state.shift_factor = 7;

        auto writes = execute_plane_row_upload(state, tiles);
        assert(writes.size() == 43); // 2 control writes + 41 data writes
        assert(writes[0].port == TilemapPlaneUploadContract::kVdpControlPort);
        assert(writes[1].port == TilemapPlaneUploadContract::kVdpControlPort);
        assert(state.scroll_counter == 1);
        assert(state.y_pos == 6);
        assert(state.vram_offset == 0x0080);
    }

    // Case 2: Scroll down row upload
    {
        std::vector<uint16_t> tiles(4096);
        for (size_t i = 0; i < tiles.size(); ++i) tiles[i] = static_cast<uint16_t>(0x0200 + i);

        PlaneUploadState state{};
        state.vram_base = 0xC000U;
        state.scroll_counter = -3;
        state.vram_offset = 0x0200;
        state.plane_width = 0x80U;
        state.plane_height = 0x20U;
        state.x_pos = 10;
        state.y_pos = 8;
        state.shift_factor = 7;

        auto writes = execute_plane_row_upload(state, tiles);
        assert(writes.size() == 43);
        assert(state.scroll_counter == -2);
        assert(state.y_pos == 7);
        assert(state.vram_offset == 0x0180);
    }

    // Case 3: Column wrap boundary
    {
        std::vector<uint16_t> tiles(4096);
        for (size_t i = 0; i < tiles.size(); ++i) tiles[i] = static_cast<uint16_t>(0x0300 + i);

        PlaneUploadState state{};
        state.vram_base = 0xC000U;
        state.scroll_counter = 1;
        state.vram_offset = 0x0070;
        state.plane_width = 0x80U;
        state.plane_height = 0x20U;
        state.x_pos = 2;
        state.y_pos = 1;
        state.shift_factor = 7;

        auto writes = execute_plane_row_upload(state, tiles);
        assert(writes.size() == 45); // 2 initial control + 41 data + 2 wrap control writes
        assert(state.scroll_counter == 0);
        assert(state.y_pos == 2);
    }

    // Case 4: Plane B scroll up row upload
    {
        std::vector<uint16_t> tiles(4096);
        for (size_t i = 0; i < tiles.size(); ++i) tiles[i] = static_cast<uint16_t>(0x0400 + i);

        PlaneUploadState state{};
        state.vram_base = 0xE000U;
        state.scroll_counter = 1;
        state.vram_offset = 0x0100;
        state.plane_width = 0x80U;
        state.plane_height = 0x20U;
        state.x_pos = 8;
        state.y_pos = 3;
        state.shift_factor = 7;

        auto writes = execute_plane_row_upload(state, tiles);
        assert(writes.size() == 43);
        assert(state.scroll_counter == 0);
        assert(state.y_pos == 4);
    }

    // Case 5: Scroll right column upload
    {
        std::vector<uint16_t> tiles(4096);
        for (size_t i = 0; i < tiles.size(); ++i) tiles[i] = static_cast<uint16_t>(0x0500 + i);

        PlaneUploadState state{};
        state.vram_base = 0xC000U;
        state.scroll_counter = 2;
        state.vram_offset = 0;
        state.plane_width = 0x80U;
        state.plane_height = 0x20U;
        state.x_pos = 4;
        state.y_pos = 5;
        state.shift_factor = 7;

        auto writes = execute_plane_column_upload(state, tiles);
        assert(writes.size() == 33); // 1 autoinc + 2 cmd + 29 data + 1 restore autoinc
        assert(writes[0].port == TilemapPlaneUploadContract::kVdpControlPort);
        assert(writes[0].value == TilemapPlaneUploadContract::kVdpAutoincrement128);
        assert(writes.back().port == TilemapPlaneUploadContract::kVdpControlPort);
        assert(writes.back().value == TilemapPlaneUploadContract::kVdpAutoincrement2);
        assert(state.scroll_counter == 1);
        assert(state.x_pos == 5);
    }

    // Case 6: Scroll left column upload
    {
        std::vector<uint16_t> tiles(4096);
        for (size_t i = 0; i < tiles.size(); ++i) tiles[i] = static_cast<uint16_t>(0x0600 + i);

        PlaneUploadState state{};
        state.vram_base = 0xC000U;
        state.scroll_counter = -1;
        state.vram_offset = 0x0020;
        state.plane_width = 0x80U;
        state.plane_height = 0x20U;
        state.x_pos = 8;
        state.y_pos = 2;
        state.shift_factor = 7;

        auto writes = execute_plane_column_upload(state, tiles);
        assert(writes.size() == 33);
        assert(writes[0].port == TilemapPlaneUploadContract::kVdpControlPort);
        assert(writes[0].value == TilemapPlaneUploadContract::kVdpAutoincrement128);
        assert(writes.back().port == TilemapPlaneUploadContract::kVdpControlPort);
        assert(writes.back().value == TilemapPlaneUploadContract::kVdpAutoincrement2);
        assert(state.scroll_counter == 0);
        assert(state.x_pos == 7);
    }

    // Case 7: Column wrap vertical (row index >= 32 wraps plane)
    {
        std::vector<uint16_t> tiles(4096);
        for (size_t i = 0; i < tiles.size(); ++i) tiles[i] = static_cast<uint16_t>(0x0700 + i);

        PlaneUploadState state{};
        state.vram_base = 0xC000U;
        state.scroll_counter = 1;
        state.vram_offset = 0x0E80;
        state.plane_width = 0x80U;
        state.plane_height = 0x20U;
        state.x_pos = 5;
        state.y_pos = 30;
        state.shift_factor = 7;

        auto writes = execute_plane_column_upload(state, tiles);
        assert(writes.size() == 35); // autoinc + 2 cmd + 29 data + 2 wrap cmd + restore autoinc
        assert(state.scroll_counter == 0);
        assert(state.x_pos == 6);
    }

    // Case 8: Plane B scroll right column upload
    {
        std::vector<uint16_t> tiles(4096);
        for (size_t i = 0; i < tiles.size(); ++i) tiles[i] = static_cast<uint16_t>(0x0800 + i);

        PlaneUploadState state{};
        state.vram_base = 0xE000U;
        state.scroll_counter = 3;
        state.vram_offset = 0x0040;
        state.plane_width = 0x80U;
        state.plane_height = 0x20U;
        state.x_pos = 12;
        state.y_pos = 10;
        state.shift_factor = 7;

        auto writes = execute_plane_column_upload(state, tiles);
        assert(writes.size() == 33);
        assert(state.scroll_counter == 2);
        assert(state.x_pos == 13);
    }

    std::cout << "[PASS] tilemap_plane_upload_test: all 8 row and column oracle cases verified.\n";
    return 0;
}
