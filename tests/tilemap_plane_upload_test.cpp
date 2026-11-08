#include "game/render/tilemap_plane_upload.hpp"

#include <cassert>
#include <iostream>
#include <vector>

int main() {
    using namespace oasis::render;

    std::vector<uint16_t> tiles(2048, 0x0155U);
    PlaneUploadState state{};
    state.vram_base = 0xC000U;
    state.scroll_counter = 1;
    state.vram_offset = 0;
    state.plane_width = 64;
    state.plane_height = 32;
    state.x_pos = 0;
    state.y_pos = 0;
    state.shift_factor = 7;

    auto writes = execute_plane_row_upload(state, tiles);

    assert(!writes.empty());
    assert(writes[0].port == TilemapPlaneUploadContract::kVdpControlPort);
    assert(writes[1].port == TilemapPlaneUploadContract::kVdpControlPort);
    assert(state.scroll_counter == 0);

    unsigned data_writes = 0;
    for (const auto& w : writes) {
        if (w.port == TilemapPlaneUploadContract::kVdpDataPort) {
            data_writes++;
        }
    }
    assert(data_writes == TilemapPlaneUploadContract::kWordsPerUpload);

    std::cout << "[PASS] tilemap_plane_upload_test: " << writes.size() << " VDP writes emitted.\n";
    return 0;
}
