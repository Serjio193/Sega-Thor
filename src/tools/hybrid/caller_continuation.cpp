#include "tools/hybrid/caller_continuation.hpp"
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid {

CallerContinuationObserver::CallerContinuationObserver(CallerAttributionApi api) : api_(api) {
    if (!api.reg || !api.peek || !api.cycles || !api.refresh_cycles)
        throw std::invalid_argument("continuation observer requires complete read API");
}

std::string CallerContinuationObserver::snapshot(int type, int width, unsigned address,
    unsigned value, unsigned frame, unsigned previous) const {
    std::ostringstream out;
    out << "{\"type\":" << type << ",\"width\":" << width
        << ",\"address\":" << address << ",\"value\":" << value
        << ",\"frame\":" << frame << ",\"cycles\":" << api_.cycles()
        << ",\"refresh\":" << api_.refresh_cycles()
        << ",\"previous_execute\":" << previous << ",\"registers\":[";
    for (unsigned i = 0; i < 18; ++i) {
        if (i) out << ',';
        out << api_.reg(i);
    }
    out << "],\"stack_bytes\":[";
    const auto stack = api_.reg(15) & 0xFFFFFFU;
    // Includes 56 saved register bytes, saved SR, original return and margin.
    for (unsigned i = 0; i < 68; ++i) {
        if (i) out << ',';
        out << api_.peek((stack + i) & 0xFFFFFFU);
    }
    out << "]}\n";
    return out.str();
}

void CallerContinuationObserver::event(int type, int width, unsigned address,
    unsigned value, unsigned frame, unsigned previous) {
    address &= 0xFFFFFFU;
    if (type == 1 && address == 0x604F0) ++entries_;
    if (complete_ || truncated_) return;
    if (type == 1 && address == 0x60004 && !active_) {
        armed_ = true;
        events_ = 0;
        pending_.clear();
    }
    if (!armed_) return;
    if (++events_ > 512) {
        if (active_) truncated_ = true;
        armed_ = false;
        pending_.clear();
        return;
    }
    if (type == 1 && address == 0x604F0) {
        active_ = true;
        captured_ = pending_;
        pending_.clear();
    }
    auto record = snapshot(type, width, address, value, frame, previous);
    if (active_) captured_ += record;
    else pending_ += record;
    // Include RTS post-state and the following execute boundary. Do not infer
    // a return from A7 alone, or confuse RamFlag's nested RTS with the parent.
    if (active_ && returning_ && type == 1) complete_ = true;
    if (active_ && type == 1 && address == 0x611DE) returning_ = true;
}

std::string CallerContinuationObserver::jsonl() const {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.hybrid.caller-continuation.v1\",\"entries\":" << entries_
        << ",\"complete\":" << (complete_ ? "true" : "false")
        << ",\"truncated\":" << (truncated_ ? "true" : "false") << "}\n";
    return out.str() + captured_;
}

} // namespace oasis::hybrid
