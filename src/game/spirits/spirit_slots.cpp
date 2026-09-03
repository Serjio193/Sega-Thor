#include "game/spirits/spirit_slots.hpp"

namespace oasis::game::spirits {

SpiritDispatchTrace trace_observed_dispatch(
    const SpiritDispatchInput& input) noexcept {
    SpiritDispatchTrace trace{};
    trace.guard_flags = input.guard_flags;

    // 0x31B80 reaches the slot-gated branch only when both observed bits in
    // entity +0x41 are set. This preserves the raw contract without naming
    // the buttons represented by those bits.
    constexpr std::uint8_t kRequiredInputBits = 0x0AU;
    if ((input.input_41 & kRequiredInputBits) != kRequiredInputBits) {
        return trace;
    }

    trace.entered_observed_path = true;
    trace.slot_available =
        (input.slot_flags &
         static_cast<std::uint8_t>(1U << kObservedDispatchSlot)) != 0;

    if (trace.slot_available &&
        (input.guard_flags &
         static_cast<std::uint8_t>(1U << kObservedDispatchGuardBit)) == 0) {
        trace.selectors[trace.selector_count++] = kObservedResourceSelector;
        trace.guard_flags |= static_cast<std::uint8_t>(
            1U << kObservedDispatchGuardBit);
    }

    // The observed path falls through to 0x31BF2 and queues selector 0x15
    // even when the slot-gated resource branch is skipped.
    trace.selectors[trace.selector_count++] = kObservedEffectSelector;
    return trace;
}

} // namespace oasis::game::spirits
