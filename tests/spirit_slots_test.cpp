#include "game/spirits/spirit_slots.hpp"

#include <array>
#include <cassert>
#include <span>

namespace {

void write_word(std::span<std::uint8_t> bytes,
                std::size_t offset,
                std::uint16_t value) {
    bytes[offset] = static_cast<std::uint8_t>(value >> 8U);
    bytes[offset + 1U] = static_cast<std::uint8_t>(value & 0xFFU);
}

} // namespace

int main() {
    using namespace oasis::game::spirits;
    using oasis::game::entities::EntityPoolView;
    using oasis::game::entities::kEntityPoolAtFf19e8;
    using oasis::game::entities::kEntityPoolAtFf2d8c;

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

    std::array<std::uint8_t, 6U * 0x5AU> owner_storage{};
    write_word(owner_storage, 0x00, 0x2E);
    write_word(owner_storage, 0x08, 100);
    write_word(owner_storage, 0x0C, 100);
    write_word(owner_storage, 0x10, 0);
    write_word(owner_storage, 0x14, 0);
    std::array<std::uint8_t, 21U * 0xBCU> target_storage{};
    const auto target_offset = 0x00U;
    write_word(target_storage, target_offset + 0x00, 0x16);
    write_word(target_storage, target_offset + 0x08, 100);
    write_word(target_storage, target_offset + 0x0C, 100);
    write_word(target_storage, target_offset + 0x10, 0);
    write_word(target_storage, target_offset + 0x14, 0);
    write_word(target_storage, target_offset + 0x42, 10);
    write_word(target_storage, target_offset + 0x44, 6);
    write_word(target_storage, target_offset + 0x4A, 8);
    const EntityPoolView owner_pool(owner_storage, kEntityPoolAtFf2d8c);
    const EntityPoolView target_pool(target_storage, kEntityPoolAtFf19e8);
    assert(find_observed_target(owner_pool, target_pool, {.owner_index = 0}) == 0);

    write_word(target_storage, target_offset + 0x00, 0x17);
    assert(!find_observed_target(owner_pool, target_pool, {.owner_index = 0}));
    return 0;
}
