#include "tools/hybrid/a5_lifetime_runtime.hpp"

#include "tools/hybrid/a5_lifetime_observer.hpp"
#include "tools/hybrid/callee_62ae0_observer.hpp"
#include <cstdlib>
#include <memory>

namespace oasis::hybrid {
namespace {
std::unique_ptr<A5LifetimeObserver> observer;
std::unique_ptr<Callee62AE0Observer> callee_observer;
}

void start_a5_lifetime_observer(CallerAttributionApi api) {
    observer = std::make_unique<A5LifetimeObserver>(api);
    if (std::getenv("OASIS_CALLEE_62AE0"))
        callee_observer = std::make_unique<Callee62AE0Observer>(api);
}

void record_a5_lifetime_event(int type, int width, unsigned address, unsigned value,
                              unsigned frame, unsigned previous_execute_pc) {
    if (observer) observer->event(type, width, address, value, frame, previous_execute_pc);
}

void finish_a5_lifetime_observer(const std::filesystem::path& output) {
    if (observer) observer->finish(output);
}

void stop_a5_lifetime_observer() { observer.reset(); }

void start_callee_62ae0_observer(CallerAttributionApi api) {
    callee_observer = std::make_unique<Callee62AE0Observer>(api);
}

void record_callee_62ae0_event(int type, int width, unsigned address, unsigned value,
                               unsigned frame, unsigned previous_execute_pc) {
    if (callee_observer)
        callee_observer->event(type, width, address, value, frame, previous_execute_pc);
}

void finish_callee_62ae0_observer(const std::filesystem::path& output) {
    if (callee_observer) callee_observer->finish(output);
}

void stop_callee_62ae0_observer() { callee_observer.reset(); }

} // namespace oasis::hybrid
