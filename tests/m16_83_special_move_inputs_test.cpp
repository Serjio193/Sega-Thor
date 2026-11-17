#include "game/player/special_move_inputs.hpp"

#include <cassert>

int main() {
    using oasis::game::player::detect_fbf_lunge;
    using oasis::game::player::detect_spin_360;
    using oasis::game::video::InputHistoryRing64;

    // 1. 360 Spin Slash (CW: Up=1 -> Right=8 -> Down=2 -> Left=4, with diagonal/neutral ignored).
    InputHistoryRing64 ring;
    ring.record(1U); // Up
    ring.record(9U); // Up-Right diagonal (odd -> ignored)
    ring.record(8U); // Right
    ring.record(0U); // Neutral (0xFF -> ignored)
    ring.record(2U); // Down
    ring.record(4U); // Left
    assert(ring.count() == 6U);
    assert(detect_spin_360(ring));
    assert(ring.count() == 0U); // Cleared on match

    // CCW: Up=1 -> Left=4 -> Down=2 -> Right=8.
    ring.reset();
    ring.record(1U);
    ring.record(4U);
    ring.record(2U);
    ring.record(8U);
    assert(detect_spin_360(ring));
    assert(ring.count() == 0U);

    // Incomplete 3-step spin fails.
    ring.reset();
    ring.record(1U);
    ring.record(8U);
    ring.record(2U);
    assert(!detect_spin_360(ring));
    assert(ring.count() == 3U);

    // 2. F-B-F Lunge: Facing 1 (Right: 8, 4, 8), clears count on match.
    ring.reset();
    ring.record(8U);
    ring.record(4U);
    ring.record(8U);
    assert(detect_fbf_lunge(ring, 1U));
    assert(ring.count() == 0U);

    // Perpendicular input (Up=1) aborts Right F-B-F lunge.
    ring.reset();
    ring.record(8U);
    ring.record(4U);
    ring.record(1U); // Perpendicular Up
    ring.record(8U);
    assert(!detect_fbf_lunge(ring, 1U));
    assert(ring.count() == 4U);

    return 0;
}
