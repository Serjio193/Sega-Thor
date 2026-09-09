#pragma once

#include "tools/hybrid/caller_attribution.hpp"
#include <string>

namespace oasis::hybrid {

// Read-only bounded evidence, exclusively for the emulated natural path.
// A parent entry arms a bounded buffer; only reaching 0x604F0 retains it.
class CallerContinuationObserver {
public:
    explicit CallerContinuationObserver(CallerAttributionApi api);
    void event(int type, int width, unsigned address, unsigned value,
               unsigned frame, unsigned previous_execute_pc);
    [[nodiscard]] std::string jsonl() const;
    [[nodiscard]] unsigned entries() const noexcept { return entries_; }
    [[nodiscard]] bool complete() const noexcept { return complete_; }
private:
    CallerAttributionApi api_;
    std::string pending_, captured_;
    unsigned events_{}, entries_{};
    bool armed_{}, active_{}, returning_{}, complete_{}, truncated_{};
    std::string snapshot(int type, int width, unsigned address, unsigned value,
                         unsigned frame, unsigned previous) const;
};

} // namespace oasis::hybrid
