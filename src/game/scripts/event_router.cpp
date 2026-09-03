#include "game/scripts/event_router.hpp"

namespace oasis::game::scripts {

std::optional<ObservedEventTransfer> produce_observed_event(
    const ObservedEventSource& source) noexcept {
    if (source.raw_type != kObservedEventSourceType) {
        return std::nullopt;
    }

    const auto event_code = static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(source.raw_32 << 8U) | source.raw_52);
    return ObservedEventTransfer{
        .raw_event_code = event_code,
        .raw_event_word = source.raw_04,
        .raw_event_long = source.raw_4e,
        .source_type_cleared = true,
    };
}

} // namespace oasis::game::scripts
