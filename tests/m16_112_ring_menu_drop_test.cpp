#include "game/ui/ring_menu_drop.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::ui;

void test_cursor_4x2_wrap() {
    assert(step_cursor_4x2(0U, -1, 0) == 3U);
    assert(step_cursor_4x2(3U, 1, 0) == 0U);
    assert(step_cursor_4x2(2U, 0, 1) == 6U);
    assert(step_cursor_4x2(6U, 0, 1) == 2U);
    assert(step_cursor_4x2(7U, 1, 1) == 0U);
    assert(step_cursor_4x2(0U, -1, -1) == 7U);
}

void test_item_drop_guards_and_success() {
    std::array<InventorySlotPair, kRingInventorySlotCount> inv = {{
        {1U, 1U}, // item_id <= 1 (not droppable)
        {5U, 3U}, // protected slot
        {6U, 4U}, // droppable
        {0U, 0U},
        {0U, 0U},
        {0U, 0U},
        {0U, 0U},
        {0U, 0U},
    }};

    // Guard 1: item_id <= 1
    assert(!try_drop_inventory_item(inv, 0U, 1U, false).dropped);
    assert(inv[0].item_id == 1U);

    // Guard 2: cursor == protected_slot
    assert(!try_drop_inventory_item(inv, 1U, 1U, false).dropped);
    assert(inv[1].item_id == 5U);

    // Guard 3: terrain_blocked
    assert(!try_drop_inventory_item(inv, 2U, 1U, true).dropped);
    assert(inv[2].item_id == 6U);

    // Valid drop
    const auto res = try_drop_inventory_item(inv, 2U, 1U, false);
    assert(res.dropped);
    assert(res.drop_item_id == (6U | 0x80U));
    assert(res.drop_item_qty == 4U);
    assert(inv[2].item_id == 0U);
    assert(inv[2].qty == 0U);
}

} // namespace

int main() {
    test_cursor_4x2_wrap();
    test_item_drop_guards_and_success();
    return 0;
}
