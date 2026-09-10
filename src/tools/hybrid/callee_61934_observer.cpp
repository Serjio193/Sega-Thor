#include "tools/hybrid/callee_61934_observer.hpp"

#include <fstream>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid {
namespace {
constexpr unsigned kEntry = 0x061934U;
constexpr std::array<unsigned, 6> kParentCalls{0x0601EEU, 0x0601FAU, 0x060206U,
                                                 0x060212U, 0x06021EU, 0x060260U};
struct Direct { unsigned pc, target; };
constexpr std::array<Direct, 12> kDirect{{
    {0x061964U, 0x061E48U}, {0x0619B2U, 0x061EF4U}, {0x0619DAU, 0x062156U},
    {0x061A12U, 0x06138EU}, {0x061A16U, 0x0613F8U}, {0x061ABAU, 0x0626BCU},
    {0x061AC2U, 0x0613B2U}, {0x061AC6U, 0x06147EU}, {0x061D28U, 0x061CD2U},
    {0x061DB6U, 0x061CD2U}, {0x061EE4U, 0x061EF4U}, {0x06219AU, 0x061CD2U}}};
constexpr unsigned kIndirect = 0x061F60U;
}

Callee61934Observer::Callee61934Observer(CallerAttributionApi api) : api_(api) {
    if (!api_.reg || !api_.cycles) throw std::invalid_argument("callee observer requires API");
}

unsigned Callee61934Observer::a5() const { return api_.reg(13U) & 0xFFFFFFU; }

bool Callee61934Observer::parent_call(unsigned pc, unsigned& return_pc) {
    for (const auto call : kParentCalls) if (pc == call) { return_pc = pc + 4U; return true; }
    return false;
}

bool Callee61934Observer::direct_call(unsigned pc, unsigned& target) {
    for (const auto call : kDirect) if (pc == call.pc) { target = call.target; return true; }
    return false;
}

bool Callee61934Observer::indirect_call(unsigned pc) { return pc == kIndirect; }

Callee61934Observer::Nested& Callee61934Observer::nested(unsigned callsite, unsigned target,
                                                          unsigned return_pc) {
    for (auto& item : nested_)
        if (item.callsite == callsite && item.target == target && item.return_pc == return_pc) return item;
    nested_.push_back({callsite, target, return_pc, 0, 0, 0, 0});
    return nested_.back();
}

void Callee61934Observer::execute(unsigned pc) {
    if (!active_) {
        unsigned return_pc{};
        if (parent_call(pc, return_pc)) {
            parent_pending_ = true; parent_return_ = return_pc;
            for (std::size_t i = 0; i < kParentCalls.size(); ++i)
                if (kParentCalls[i] == pc) { ++parent_calls_[i]; break; }
            return;
        }
        if (!parent_pending_ || pc != kEntry) return;
        parent_pending_ = false; active_ = true; ++entries_; entry_a5_ = a5();
        path_hash_ = 2166136261U; path_length_ = 0; path_pcs_.clear(); stack_.clear(); return;
    }
    if (pc == parent_return_ && stack_.empty() && !indirect_pending_) { finish_parent(); return; }
    if (direct_pending_ && pc == pending_.target) {
        direct_pending_ = false;
        pending_.entry_a5 = a5();
        stack_.push_back(pending_);
        auto& item = nested(pending_.callsite, pending_.target, pending_.return_pc);
        ++item.calls;
    } else if (indirect_pending_) {
        indirect_pending_ = false;
        pending_.target = pc;
        pending_.entry_a5 = a5();
        stack_.push_back(pending_);
        auto& item = nested(pending_.callsite, pc, pending_.return_pc);
        ++item.calls;
    } else if (!stack_.empty() && pc == stack_.back().return_pc) {
        finish_nested(pc);
    }
    path_hash_ ^= pc; path_hash_ *= 16777619U; ++path_length_; path_pcs_.push_back(pc);
    unsigned target{};
    if (direct_call(pc, target)) {
        ++direct_calls_; pending_ = {pc, target, pc + 4U, 0U, false}; direct_pending_ = true;
    } else if (indirect_call(pc)) {
        ++indirect_calls_; pending_ = {pc, 0U, pc + 4U, 0U, true}; indirect_pending_ = true;
    }
}

void Callee61934Observer::finish_nested(unsigned pc) {
    auto frame = stack_.back(); stack_.pop_back();
    auto& item = nested(frame.callsite, frame.target, frame.return_pc);
    ++item.returns;
    if (a5() == frame.entry_a5) ++item.a5_equal; else ++item.a5_unequal;
    if (frame.indirect) ++indirect_returns_; else ++direct_returns_;
    (void)pc;
}

void Callee61934Observer::finish_parent() {
    active_ = false; ++returns_;
    for (std::size_t i = 0; i < kParentCalls.size(); ++i)
        if (kParentCalls[i] + 4U == parent_return_) { ++parent_returns_[i]; break; }
    if (a5() == entry_a5_) ++equal_a5_; else ++unequal_a5_;
    for (auto& path : paths_)
        if (path.hash == path_hash_ && path.length == path_length_ && path.pcs == path_pcs_) { ++path.count; return; }
    paths_.push_back({path_hash_, path_length_, 1U, path_pcs_});
}

void Callee61934Observer::memory(int type, int width, unsigned address, unsigned previous_pc) {
    if (type != 2 && type != 4) return;
    address &= 0xFFFFFFU; previous_pc &= 0xFFFFFFU;
    for (auto& effect : effects_)
        if (effect.pc == previous_pc && effect.address == address && effect.type == static_cast<unsigned>(type) &&
            effect.width == static_cast<unsigned>(width)) { ++effect.count; return; }
    effects_.push_back({previous_pc, address, static_cast<unsigned>(type), static_cast<unsigned>(width), 1U});
}

void Callee61934Observer::event(int type, int width, unsigned address, unsigned value,
                                unsigned frame, unsigned previous_execute_pc) {
    (void)value; (void)frame;
    if (active_ && type >= 0 && type < static_cast<int>(hook_types_.size())) ++hook_types_[type];
    if (active_ && type == (1 << 15)) ++interrupt_events_;
    if (type == 1) execute(address & 0xFFFFFFU);
    else if (active_) memory(type, width, address, previous_execute_pc);
}

void Callee61934Observer::finish(const std::filesystem::path& output) {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.hybrid.callee-61934.v1\",\"entries\":" << entries_
        << ",\"returns\":" << returns_ << ",\"a5_equal\":" << equal_a5_
        << ",\"a5_unequal\":" << unequal_a5_ << ",\"direct_calls\":" << direct_calls_
        << ",\"indirect_calls\":" << indirect_calls_ << ",\"direct_returns\":" << direct_returns_
        << ",\"indirect_returns\":" << indirect_returns_ << ",\"interrupt_events\":" << interrupt_events_
        << ",\"hook_types\":[";
    for (std::size_t i = 0; i < hook_types_.size(); ++i) { if (i) out << ','; out << hook_types_[i]; }
    out << "],\"parent_calls\":[";
    for (std::size_t i = 0; i < parent_calls_.size(); ++i) { if (i) out << ','; out << parent_calls_[i]; }
    out << "],\"parent_returns\":[";
    for (std::size_t i = 0; i < parent_returns_.size(); ++i) { if (i) out << ','; out << parent_returns_[i]; }
    out << "],\"paths\":[";
    for (std::size_t i = 0; i < paths_.size(); ++i) {
        if (i) out << ','; const auto& path = paths_[i];
        out << "{\"hash\":" << path.hash << ",\"length\":" << path.length << ",\"count\":" << path.count << ",\"pcs\":[";
        for (std::size_t j = 0; j < path.pcs.size(); ++j) { if (j) out << ','; out << path.pcs[j]; }
        out << "]}";
    }
    out << "],\"nested\":[";
    for (std::size_t i = 0; i < nested_.size(); ++i) {
        if (i) out << ','; const auto& n = nested_[i];
        out << "{\"callsite\":" << n.callsite << ",\"target\":" << n.target
            << ",\"return_pc\":" << n.return_pc << ",\"calls\":" << n.calls
            << ",\"returns\":" << n.returns << ",\"a5_equal\":" << n.a5_equal
            << ",\"a5_unequal\":" << n.a5_unequal << '}';
    }
    out << "],\"effects\":[";
    for (std::size_t i = 0; i < effects_.size(); ++i) {
        if (i) out << ','; const auto& e = effects_[i];
        out << "{\"pc\":" << e.pc << ",\"address\":" << e.address << ",\"type\":" << e.type
            << ",\"width\":" << e.width << ",\"count\":" << e.count << '}';
    }
    out << "]}\n"; jsonl_ = out.str();
    if (output.empty()) return;
    std::ofstream file(output); file.exceptions(std::ios::failbit | std::ios::badbit); file << jsonl_;
}

} // namespace oasis::hybrid
