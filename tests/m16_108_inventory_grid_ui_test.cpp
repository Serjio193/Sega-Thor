#include "game/ui/inventory_grid_ui.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::ui;

void test_grid_slot_coordinates() {
    const auto s0 = grid_slot_to_xy(0U);
    assert(s0.x == 0x70 && s0.y == 0x40);

    const auto s3 = grid_slot_to_xy(3U);
    assert(s3.x == 0x70 + 3 * 24 && s3.y == 0x40);

    const auto s4 = grid_slot_to_xy(4U);
    assert(s4.x == 0x70 && s4.y == 0x40 + 24);

    const auto s7 = grid_slot_to_xy(7U);
    assert(s7.x == 0x70 + 3 * 24 && s7.y == 0x40 + 24);

    const auto s15 = grid_slot_to_xy(15U);
    assert(s15.x == 0x70 + 3 * 24 && s15.y == 0x40 + 3 * 24);
}

void test_inventory_grid_icon_plan() {
    const std::array<std::uint16_t, kInventoryGridSlots> inv = {
        1U, 0U, 2U, 0U, 5U, 0U, 0U, 9U
    };
    const auto plan = build_inventory_grid_icon_plan(inv, 3U, 7U);
    assert(plan.size() == 6U);

    // Slot 0: item 1 at (0x70, 0x40 + 0x18), 9-tile stride
    assert(plan[0].slot == 0U);
    assert(plan[0].x == 0x70 && plan[0].y == 0x58);
    assert(plan[0].item_id == 1U);
    assert(plan[0].tile_index == 9U);
    assert(plan[0].rom_byte_addr == kIconRomBase + 9U * 32U);

    // Slot 4: item 5 at (0x70, 0x40 + 24 + 0x18)
    assert(plan[2].slot == 4U);
    assert(plan[2].x == 0x70 && plan[2].y == 0x40 + 24 + 0x18);
    assert(plan[2].tile_index == 45U);

    // Spirit slot 0x20 and special slot 0x29 in $FF2FA8
    assert(plan[4].buffer_slot == 0x20U);
    assert(plan[4].item_id == 3U);
    assert(plan[4].tile_index == 27U);

    assert(plan[5].buffer_slot == 0x29U);
    assert(plan[5].item_id == 7U);
    assert(plan[5].tile_index == 63U);
}

} // namespace

int main() {
    test_grid_slot_coordinates();
    test_inventory_grid_icon_plan();
    return 0;
}
