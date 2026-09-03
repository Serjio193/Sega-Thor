#include "game/spirits/spirit_slots.hpp"

namespace oasis::game::spirits {

namespace {

[[nodiscard]] std::int32_t read_i16(
    const entities::EntityRecordView& record,
    const std::size_t offset) noexcept {
    const auto value = record.read_u16(offset);
    return value ? static_cast<std::int16_t>(*value) : 0;
}

[[nodiscard]] bool horizontal_overlap(std::int32_t query_min,
                                      std::int32_t query_max,
                                      std::int32_t center,
                                      std::int32_t extent) noexcept {
    return query_min <= center + extent && query_max >= center - extent;
}

} // namespace

std::optional<std::size_t> find_observed_target(
    const entities::EntityPoolView& pool,
    const ObservedTargetQuery& query) noexcept {
    const auto owner = pool.record_view(query.owner_index);
    if (!owner) {
        return std::nullopt;
    }

    const auto owner_x = read_i16(*owner, entities::kEntityFieldPositionX);
    const auto owner_y = read_i16(*owner, entities::kEntityFieldPositionY) +
                         read_i16(*owner, entities::kEntityFieldWord14);
    const auto owner_z = read_i16(*owner, entities::kEntityFieldWord10) +
                         read_i16(*owner, entities::kEntityFieldWord14);
    const auto query_x_min = owner_x + query.relative_x_min;
    const auto query_x_max = owner_x + query.relative_x_max;
    const auto query_y_min = owner_y + query.relative_y_min;
    const auto query_y_max = owner_y + query.relative_y_max;
    const auto query_z_min = owner_z + query.relative_z_min;
    const auto query_z_max = owner_z + query.relative_z_max;

    for (std::size_t index = 0; index < pool.spec().record_count; ++index) {
        if (index == query.owner_index || !pool.active(index)) {
            continue;
        }
        const auto record = pool.record_view(index);
        if (!record) {
            continue;
        }

        const auto target_x = read_i16(*record, entities::kEntityFieldPositionX);
        const auto target_y =
            read_i16(*record, entities::kEntityFieldPositionY) +
            read_i16(*record, entities::kEntityFieldWord14);
        const auto target_z = read_i16(*record, entities::kEntityFieldWord10) +
                              read_i16(*record, entities::kEntityFieldWord14);
        if (!horizontal_overlap(
                query_x_min, query_x_max, target_x,
                read_i16(*record, entities::kEntityFieldWord42)) ||
            !horizontal_overlap(
                query_y_min, query_y_max, target_y,
                read_i16(*record, entities::kEntityFieldWord44)) ||
            query_z_min < target_z ||
            query_z_max >= target_z +
                               read_i16(*record, entities::kEntityFieldWord4A)) {
            continue;
        }

        const auto type = record->read_u16(entities::kEntityFieldType);
        if (type && *type == query.required_type) {
            return index;
        }
        return std::nullopt;
    }
    return std::nullopt;
}

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
