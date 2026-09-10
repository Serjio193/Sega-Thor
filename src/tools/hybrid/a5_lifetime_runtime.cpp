#include "tools/hybrid/a5_lifetime_runtime.hpp"

#include "tools/hybrid/a5_lifetime_observer.hpp"
#include "tools/hybrid/callee_62ae0_observer.hpp"
#include "tools/hybrid/callee_61934_observer.hpp"
#include "tools/hybrid/callee_623ac_observer.hpp"
#include <cstdlib>
#include <memory>

namespace oasis::hybrid {
namespace {
std::unique_ptr<A5LifetimeObserver> observer;
std::unique_ptr<Callee62AE0Observer> callee_observer;
std::unique_ptr<Callee61934Observer> callee_61934_observer;
std::unique_ptr<Callee623ACObserver> callee_623ac_observer;
}

bool any_a5_observer_requested() {
    return std::getenv("OASIS_A5_LIFETIME") || std::getenv("OASIS_CALLEE_62AE0") ||
           std::getenv("OASIS_CALLEE_61934") || std::getenv("OASIS_CALLEE_623AC");
}

void start_a5_lifetime_observer(CallerAttributionApi api) {
    if (std::getenv("OASIS_A5_LIFETIME")) observer = std::make_unique<A5LifetimeObserver>(api);
    if (std::getenv("OASIS_CALLEE_62AE0"))
        callee_observer = std::make_unique<Callee62AE0Observer>(api);
    if (std::getenv("OASIS_CALLEE_61934"))
        callee_61934_observer = std::make_unique<Callee61934Observer>(api);
    if (std::getenv("OASIS_CALLEE_623AC"))
        callee_623ac_observer = std::make_unique<Callee623ACObserver>(api);
}

void record_a5_lifetime_event(int type, int width, unsigned address, unsigned value,
                              unsigned frame, unsigned previous_execute_pc) {
    if (observer) observer->event(type, width, address, value, frame, previous_execute_pc);
    if (callee_61934_observer)
        callee_61934_observer->event(type, width, address, value, frame, previous_execute_pc);
    if (callee_623ac_observer)
        callee_623ac_observer->event(type, width, address, value, frame, previous_execute_pc);
}

void finish_a5_lifetime_observer(const std::filesystem::path& output) {
    if (observer) observer->finish(output);
    if (callee_61934_observer) callee_61934_observer->finish(output.parent_path() / "callee_61934.jsonl");
    if (callee_623ac_observer) callee_623ac_observer->finish(output.parent_path() / "callee_623ac.jsonl");
}

void stop_a5_lifetime_observer() {
    observer.reset(); callee_observer.reset(); callee_61934_observer.reset(); callee_623ac_observer.reset();
}

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
