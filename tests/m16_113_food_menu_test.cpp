#include "game/ui/food_menu.hpp"

#include <array>
#include <cassert>

namespace {

using namespace oasis::game::ui;

void test_cursor_4x4_wrap() {
    assert(step_cursor_4x4(0U, -1, 0) == 3U);
    assert(step_cursor_4x4(3U, 1, 0) == 0U);
    assert(step_cursor_4x4(0U, 0, -1) == 12U);
    assert(step_cursor_4x4(12U, 0, 1) == 0U);
    assert(step_cursor_4x4(15U, 1, 1) == 0U);
    assert(step_cursor_4x4(0U, -1, -1) == 15U);
}

void test_food_drop_guards_and_constants() {
    static_assert(kFoodConsumeTimerFrames == 0x005AU);
    static_assert(kFoodConsumeSfxCmd == 0x00A00256U);

    std::array<std::uint8_t, 8> flags{};
    flags[3] = kFoodUndroppableMask; // bit 5 set -> undroppable
    flags[4] = 0x00U;                // droppable

    std::array<InventorySlotPair, kFoodInventorySlotCount> inv{};
    inv[0] = {0U, 0U};
    inv[1] = {3U, 1U};
    inv[2] = {4U, 2U};

    // Guard 1: empty slot (item_id == 0)
    assert(!try_drop_food_item(inv, 0U, flags, false).dropped);

    // Guard 2: undroppable bit 5 set in 0x05CE16
    assert(!try_drop_food_item(inv, 1U, flags, false).dropped);
    assert(inv[1].item_id == 3U);

    // Guard 3: terrain_blocked
    assert(!try_drop_food_item(inv, 2U, flags, true).dropped);
    assert(inv[2].item_id == 4U);

    // Valid drop
    const auto res = try_drop_food_item(inv, 2U, flags, false);
    assert(res.dropped);
    assert(res.drop_item_id == (4U | 0x80U));
    assert(res.drop_item_qty == 2U);
    assert(inv[2].item_id == 0U);
    assert(inv[2].qty == 0U);
}

} // namespace

int main() {
    test_cursor_4x4_wrap();
    test_food_drop_guards_and_constants();
    return 0;
}
