#include "tools/hybrid/callee_623ac_observer.hpp"

#include <algorithm>
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid {
namespace {
constexpr unsigned kEntry = 0x0623ACU;
constexpr std::array<unsigned, 4> kParentCalls{0x060234U, 0x060242U, 0x060250U,
                                                0x060276U};
struct Direct { unsigned pc, target; };
constexpr std::array<Direct, 16> kDirect{{
    {0x0623CCU, 0x062732U}, {0x062418U, 0x062722U}, {0x062430U, 0x06273AU},
    {0x06243CU, 0x06281AU}, {0x062464U, 0x062A78U}, {0x0624A0U, 0x06138EU},
    {0x0624C8U, 0x06282EU}, {0x062526U, 0x062722U}, {0x062544U, 0x0626BCU},
    {0x062556U, 0x062788U}, {0x062572U, 0x06273AU}, {0x062576U, 0x0613B2U},
    {0x06260AU, 0x062732U}, {0x062620U, 0x06273AU}, {0x062862U, 0x06273AU},
    {0x062A9EU, 0x062722U}}};
constexpr unsigned kIndirect = 0x062878U;
}

Callee623ACObserver::Callee623ACObserver(CallerAttributionApi api) : api_(api) {
    if (!api_.reg || !api_.cycles) throw std::invalid_argument("callee observer requires API");
}

unsigned Callee623ACObserver::a5() const { return api_.reg(13U) & 0xFFFFFFU; }

bool Callee623ACObserver::parent_call(unsigned pc, unsigned& return_pc) {
    for (const auto call : kParentCalls) if (pc == call) { return_pc = pc + 4U; return true; }
    return false;
}

bool Callee623ACObserver::direct_call(unsigned pc, unsigned& target) {
    for (const auto call : kDirect) if (pc == call.pc) { target = call.target; return true; }
    return false;
}

bool Callee623ACObserver::indirect_call(unsigned pc) { return pc == kIndirect; }

Callee623ACObserver::Nested& Callee623ACObserver::nested(unsigned callsite, unsigned target,
                                                          unsigned return_pc) {
    for (auto& item : nested_)
        if (item.callsite == callsite && item.target == target && item.return_pc == return_pc) return item;
    nested_.push_back({callsite, target, return_pc, 0, 0, 0, 0});
    return nested_.back();
}

void Callee623ACObserver::execute(unsigned pc) {
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
    if (pc == parent_return_ && stack_.empty() && !direct_pending_ && !indirect_pending_) {
        finish_parent(); return;
    }
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

void Callee623ACObserver::finish_nested(unsigned pc) {
    auto frame = stack_.back(); stack_.pop_back();
    auto& item = nested(frame.callsite, frame.target, frame.return_pc);
    ++item.returns;
    if (a5() == frame.entry_a5) ++item.a5_equal; else ++item.a5_unequal;
    if (frame.indirect) ++indirect_returns_; else ++direct_returns_;
    (void)pc;
}

void Callee623ACObserver::finish_parent() {
    active_ = false; ++returns_;
    for (std::size_t i = 0; i < kParentCalls.size(); ++i)
        if (kParentCalls[i] + 4U == parent_return_) { ++parent_returns_[i]; break; }
    if (a5() == entry_a5_) ++equal_a5_; else ++unequal_a5_;
    for (const auto index : current_effects_) {
        auto& hashes = effects_[index].path_hashes;
        if (std::find(hashes.begin(), hashes.end(), path_hash_) == hashes.end()) hashes.push_back(path_hash_);
    }
    current_effects_.clear();
    for (auto& path : paths_)
        if (path.hash == path_hash_ && path.length == path_length_ && path.pcs == path_pcs_) { ++path.count; return; }
    paths_.push_back({path_hash_, path_length_, 1U, path_pcs_});
}

void Callee623ACObserver::memory(int type, int width, unsigned address, unsigned previous_pc) {
    if (type != 2 && type != 4) return;
    address &= 0xFFFFFFU; previous_pc &= 0xFFFFFFU;
    for (std::size_t i = 0; i < effects_.size(); ++i) {
        auto& effect = effects_[i];
        if (effect.pc == previous_pc && effect.address == address && effect.type == static_cast<unsigned>(type) &&
            effect.width == static_cast<unsigned>(width)) {
            ++effect.count; effect.last_order = path_length_;
            if (std::find(current_effects_.begin(), current_effects_.end(), i) == current_effects_.end())
                current_effects_.push_back(i);
            return;
        }
    }
    effects_.push_back({previous_pc, address, static_cast<unsigned>(type), static_cast<unsigned>(width),
                        1U, path_length_, path_length_, {}});
    current_effects_.push_back(effects_.size() - 1U);
}

void Callee623ACObserver::event(int type, int width, unsigned address, unsigned value,
                                unsigned frame, unsigned previous_execute_pc) {
    (void)value; (void)frame;
    if (active_ && type >= 0 && type < static_cast<int>(hook_types_.size())) ++hook_types_[type];
    if (active_ && type == (1 << 15)) ++interrupt_events_;
    if (type == 1) execute(address & 0xFFFFFFU);
    else if (active_) memory(type, width, address, previous_execute_pc);
}

void Callee623ACObserver::finish(const std::filesystem::path& output) {
    std::ostringstream out;
    out << "{\"schema\":\"oasis.hybrid.callee-623ac.v1\",\"entries\":" << entries_
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
            << ",\"width\":" << e.width << ",\"count\":" << e.count
            << ",\"first_order\":" << e.first_order << ",\"last_order\":" << e.last_order
            << ",\"path_hashes\":[";
        for (std::size_t p = 0; p < e.path_hashes.size(); ++p) {
            if (p) out << ',';
            out << e.path_hashes[p];
        }
        out << "]}";
    }
    out << "]}\n"; jsonl_ = out.str();
    if (output.empty()) return;
    std::ofstream file(output); file.exceptions(std::ios::failbit | std::ios::badbit); file << jsonl_;
}

} // namespace oasis::hybrid
