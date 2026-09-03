#include "game/spirits/spirit_slots.hpp"

#include <cassert>

int main() {
    using namespace oasis::game::spirits;

    SpiritSlots slots;
    slots.apply_event(0x15);
    assert(slots.acquired_mask() == 0);
    slots.apply_event(0x16);
    slots.apply_event(0x19);
    assert(slots.acquired(0));
    assert(!slots.acquired(1));
    assert(!slots.acquired(2));
    assert(slots.acquired(3));

    const auto active = trace_observed_dispatch(
        {.input_40 = 0, .input_41 = 0x0A, .slot_flags = 0x02});
    assert(active.entered_observed_path);
    assert(active.slot_available);
    assert(active.selector_count == 2);
    assert(active.selectors[0] == kObservedResourceSelector);
    assert(active.selectors[1] == kObservedEffectSelector);
    assert((active.guard_flags & 0x01U) != 0);

    const auto guarded = trace_observed_dispatch(
        {.input_41 = 0x0A, .slot_flags = 0x02, .guard_flags = 0x01});
    assert(guarded.selector_count == 1);
    assert(guarded.selectors[0] == kObservedEffectSelector);

    const auto unavailable = trace_observed_dispatch(
        {.input_41 = 0x0A, .slot_flags = 0x01});
    assert(unavailable.entered_observed_path);
    assert(!unavailable.slot_available);
    assert(unavailable.selector_count == 1);
    assert(unavailable.selectors[0] == kObservedEffectSelector);

    const auto unrelated_input = trace_observed_dispatch(
        {.input_41 = 0x08, .slot_flags = 0x02});
    assert(!unrelated_input.entered_observed_path);
    assert(unrelated_input.selector_count == 0);
    return 0;
}
