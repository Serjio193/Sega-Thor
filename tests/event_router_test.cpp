#include "game/scripts/event_router.hpp"

#include <cassert>

int main() {
    using namespace oasis::game::scripts;

    const auto transfer = produce_observed_event({
        .raw_type = kObservedEventSourceType,
        .raw_32 = 0x0012,
        .raw_52 = 0x34,
        .raw_04 = 0x5678,
        .raw_4e = -0x123456,
    });
    assert(transfer);
    assert(transfer->raw_event_code == 0x1234);
    assert(transfer->raw_event_word == 0x5678);
    assert(transfer->raw_event_long == -0x123456);
    assert(transfer->source_type_cleared);
    assert(!produce_observed_event({.raw_type = 0x0007}));

    const auto flag_clear = route_observed_event(0x16, 0);
    assert(flag_clear.eligible);
    assert(flag_clear.handler_address == kObservedRouteFlagClearAddress);

    assert(route_observed_event(0x00, 0x02).handler_address == 0x7A6C);
    assert(route_observed_event(0x0C, 0x02).handler_address == 0x7B64);
    assert(route_observed_event(0x0D, 0x02).handler_address == 0x7BD4);
    assert(route_observed_event(0x11, 0x02).handler_address == 0x7BF6);
    assert(route_observed_event(0x12, 0x02).handler_address == 0x7BA4);
    assert(route_observed_event(0x16, 0x02).handler_address == 0x7BE8);
    assert(route_observed_event(0x1A, 0x02).handler_address == 0x7B84);
    assert(route_observed_event(0x3F, 0x02).handler_address == 0x7B84);
    assert(route_observed_event(0x40, 0x02).handler_address == 0x7A6C);
    return 0;
}
