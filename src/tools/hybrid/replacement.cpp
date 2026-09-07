#include "tools/hybrid/replacement.hpp"

#include <stdexcept>

namespace oasis::hybrid {

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
    }
    return result;
}

} // namespace oasis::hybrid
