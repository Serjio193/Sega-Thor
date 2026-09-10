#include "tools/hybrid/callee_62ae0_observer.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid {
namespace {
constexpr unsigned kCallSite = 0x0601E2U;
constexpr unsigned kEntry = 0x062AE0U;
constexpr unsigned kReturn = 0x0601E6U;
constexpr unsigned kDirectNested = 0x062B4EU;
constexpr unsigned kIndirectNested = 0x062CECU;
}

Callee62AE0Observer::Callee62AE0Observer(CallerAttributionApi api) : api_(api) {
    if (!api_.reg || !api_.cycles) throw std::invalid_argument("callee observer requires API");
}

unsigned Callee62AE0Observer::a5() const { return api_.reg(13U) & 0xFFFFFFU; }
void Callee62AE0Observer::execute(unsigned pc) {
    if (active_) {
        if (pc == kReturn) { finish_call(); return; }
        path_hash_ ^= pc;
        path_hash_ *= 16777619U;
        ++path_length_;
        path_pcs_.push_back(pc);
        if (pc == kDirectNested) ++direct_nested_;
        if (pc == kIndirectNested) ++indirect_nested_;
        return;
    }
    if (pc == kCallSite) { call_pending_ = true; return; }
    if (!call_pending_ || pc != kEntry) return;
    call_pending_ = false;
    active_ = true;
    ++entries_;
    entry_a5_ = a5();
    path_hash_ = 2166136261U;
    path_length_ = 0;
    path_pcs_.clear();
}

void Callee62AE0Observer::memory(int type, int width, unsigned address,
                                 unsigned frame, unsigned previous_execute_pc) {
    if (!active_ || (type != 2 && type != 4)) return;
    address &= 0xFFFFFFU;
    for (auto& effect : effects_) {
        if (effect.pc == previous_execute_pc && effect.address == address &&
            effect.type == static_cast<unsigned>(type) && effect.width == static_cast<unsigned>(width)) {
            ++effect.count;
            return;
        }
    }
    effects_.push_back({previous_execute_pc, address, static_cast<unsigned>(type),
                        static_cast<unsigned>(width), 1U});
}

void Callee62AE0Observer::finish_call() {
    active_ = false;
    ++returns_;
    if (a5() == entry_a5_) ++equal_a5_; else ++unequal_a5_;
    for (auto& path : paths_) {
        if (path.hash == path_hash_ && path.length == path_length_ && path.pcs == path_pcs_) {
            ++path.count;
            return;
        }
    }
    paths_.push_back({path_hash_, path_length_, 1U, path_pcs_});
}

void Callee62AE0Observer::event(int type, int width, unsigned address, unsigned value,
                                unsigned frame, unsigned previous_execute_pc) {
    (void)frame;
    if (active_ && type >= 0 && type < static_cast<int>(hook_type_counts_.size()))
        ++hook_type_counts_[type];
    if (type == 1) execute(address & 0xFFFFFFU);
    else memory(type, width, address, frame, previous_execute_pc & 0xFFFFFFU);
    (void)value;
}

void Callee62AE0Observer::finish(const std::filesystem::path& output) {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.hybrid.callee-62ae0.v1\",\"entries\":" << entries_
        << ",\"returns\":" << returns_ << ",\"a5_equal\":" << equal_a5_
        << ",\"a5_unequal\":" << unequal_a5_ << ",\"direct_nested\":"
        << direct_nested_ << ",\"indirect_nested\":" << indirect_nested_ << ",\"hook_types\":[";
    for (std::size_t i = 0; i < hook_type_counts_.size(); ++i) {
        if (i) out << ',';
        out << hook_type_counts_[i];
    }
    out << "],\"paths\":[";
    for (std::size_t i = 0; i < paths_.size(); ++i) {
        if (i) out << ',';
        out << "{\"hash\":" << paths_[i].hash << ",\"length\":"
            << paths_[i].length << ",\"count\":" << paths_[i].count << ",\"pcs\":[";
        for (std::size_t p = 0; p < paths_[i].pcs.size(); ++p) {
            if (p) out << ',';
            out << paths_[i].pcs[p];
        }
        out << "]}";
    }
    out << "],\"effects\":[";
    for (std::size_t i = 0; i < effects_.size(); ++i) {
        if (i) out << ',';
        const auto& e = effects_[i];
        out << "{\"pc\":" << e.pc << ",\"address\":" << e.address
            << ",\"type\":" << e.type << ",\"width\":" << e.width
            << ",\"count\":" << e.count << '}';
    }
    out << "]}\n";
    jsonl_ = out.str();
    if (output.empty()) return;
    std::ofstream file(output);
    file.exceptions(std::ios::failbit | std::ios::badbit);
    file << jsonl_;
}

} // namespace oasis::hybrid
