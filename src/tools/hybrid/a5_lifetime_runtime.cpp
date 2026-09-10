#include "tools/hybrid/a5_lifetime_runtime.hpp"

#include "tools/hybrid/a5_lifetime_observer.hpp"
#include <memory>

namespace oasis::hybrid {
namespace {
std::unique_ptr<A5LifetimeObserver> observer;
}

void start_a5_lifetime_observer(CallerAttributionApi api) {
    observer = std::make_unique<A5LifetimeObserver>(api);
}

void record_a5_lifetime_event(int type, int width, unsigned address, unsigned value,
                              unsigned frame, unsigned previous_execute_pc) {
    if (observer) observer->event(type, width, address, value, frame, previous_execute_pc);
}

void finish_a5_lifetime_observer(const std::filesystem::path& output) {
    if (observer) observer->finish(output);
}

void stop_a5_lifetime_observer() { observer.reset(); }

} // namespace oasis::hybrid
