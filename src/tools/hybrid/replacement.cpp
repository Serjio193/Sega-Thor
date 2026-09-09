#include "tools/hybrid/replacement.hpp"

#include <stdexcept>

namespace oasis::hybrid {

int Registry::dispatch(unsigned address) noexcept {
    if (!error_.empty()) return 0;
    try {
        for (auto* target : targets_) {
            const auto result = target->dispatch(address);
            if (!target->error().empty()) throw std::runtime_error(target->error());
            if (result) {
                active_ = target->complete() ? nullptr : target;
                return result;
            }
        }
    } catch (const std::exception& error) {
        error_ = error.what();
    }
    return 0;
}

void Registry::hook(int type, int width, unsigned address, unsigned value) noexcept {
    if (!error_.empty()) return;
    try {
        if (active_) {
            active_->hook(type, width, address, value);
            if (!active_->error().empty()) throw std::runtime_error(active_->error());
            if (active_->complete()) active_ = nullptr;
            return;
        }
        if (type != 1) return;
        for (auto* target : targets_) {
            if (target->target_address() != address) continue;
            target->hook(type, width, address, value);
            if (!target->error().empty()) throw std::runtime_error(target->error());
            if (!target->complete()) active_ = target;
            return;
        }
    } catch (const std::exception& error) {
        error_ = error.what();
    }
}

bool Registry::complete() const {
    if (!error_.empty() || active_) return false;
    for (const auto* target : targets_)
        if (!target->complete()) return false;
    return true;
}

ReplacementMetrics Registry::totals() const {
    ReplacementMetrics result{};
    for (const auto* target : targets_) {
        const auto metrics = target->metrics();
        result.calls += metrics.calls;
        result.comparisons += metrics.comparisons;
        result.divergences += metrics.divergences;
        result.body_instructions += metrics.body_instructions;
        result.override_calls += metrics.override_calls;
        result.interrupts += metrics.interrupts;
        result.native_routine_instructions += metrics.native_routine_instructions;
        result.native_routine_invocations += metrics.native_routine_invocations;
        result.native_routine_iterations += metrics.native_routine_iterations;
        result.native_routine_boundary_yields += metrics.native_routine_boundary_yields;
        result.native_routine_resumptions += metrics.native_routine_resumptions;
        result.native_routine_second_instructions += metrics.native_routine_second_instructions;
        result.native_routine_second_invocations += metrics.native_routine_second_invocations;
        result.native_routine_second_boundary_yields += metrics.native_routine_second_boundary_yields;
        result.native_routine_second_resumptions += metrics.native_routine_second_resumptions;
        result.native_internal_helper_instructions += metrics.native_internal_helper_instructions;
        result.native_internal_helper_invocations += metrics.native_internal_helper_invocations;
        result.native_internal_helper_boundary_yields += metrics.native_internal_helper_boundary_yields;
        result.native_internal_helper_resumptions += metrics.native_internal_helper_resumptions;
    }
    return result;
}

} // namespace oasis::hybrid
