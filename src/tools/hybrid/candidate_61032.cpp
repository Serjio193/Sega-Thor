#include "tools/hybrid/candidate_61032.hpp"

#include <stdexcept>

namespace oasis::hybrid {
namespace {
constexpr unsigned kTarget = 0x61032;
constexpr unsigned kEnd = 0x610C8;
constexpr unsigned kCcrMask = 0x1F;
}

Candidate61032::Candidate61032(CandidateApi api, Mode mode, std::ostream& log)
    : api_(api), mode_(mode), log_(log) {}

Candidate61032::State Candidate61032::state() const {
    State value{};
    for (unsigned i = 0; i < value.size(); ++i) value[i] = api_.reg(i);
    return value;
}

unsigned Candidate61032::read(unsigned address, unsigned width) const {
    unsigned value = 0;
    for (unsigned i = 0; i < width; ++i) {
        const auto byte = api_.peek(address + i);
        if (byte < 0 || byte > 0xFF) throw std::runtime_error("candidate 0x61032 memory peek failed");
        value = (value << 8U) | static_cast<unsigned>(byte);
    }
    return value;
}

void Candidate61032::require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void Candidate61032::fail(const std::string& message) {
    ++divergences;
    error_ = "FIRST_DIVERGENCE target=0x61032 call=" + std::to_string(calls) +
             " pc=" + std::to_string(current_pc_) + " " + message;
    active_ = false;
}

void Candidate61032::begin() {
    require(++calls <= 128, "candidate 0x61032 call budget exceeded");
    entry_ = state();
    require(entry_[16] == kTarget && entry_[13] >= 0xFF0000 && entry_[13] < 0x1000000 &&
            entry_[14] >= 0xFF0000 && entry_[14] < 0x1000000 && entry_[15] >= 4 &&
            !(entry_[15] & 1), "candidate 0x61032 entry bounds");
    base_ = entry_[14];
    source_ = entry_[8];
    return_pc_ = read(entry_[15], 4);
    require(return_pc_ < 0x300000 && !(return_pc_ & 1), "candidate 0x61032 return bound");
    entry_stack_ = entry_[15];
    entry_cycles_ = api_.cycles ? api_.cycles() : 0;
    entry_refresh_ = api_.refresh_cycles ? api_.refresh_cycles() : 0;
    writes_.clear();
    body_instructions = 0;
    active_ = true;
    if (mode_ == Mode::NATIVE_OVERRIDE) apply_override();
}

std::string Candidate61032::compare_state(const State& expected, const State& actual) const {
    for (unsigned i = 0; i < expected.size(); ++i)
        if (expected[i] != actual[i])
            return "FIRST_DIVERGENCE register[" + std::to_string(i) + "] expected=" +
                   std::to_string(expected[i]) + " actual=" + std::to_string(actual[i]);
    return {};
}

void Candidate61032::finish() {
    const auto first = read(source_, 2);
    const auto d2 = entry_[2] + entry_[1];
    const auto indirect = entry_[2] + entry_[1];
    require(indirect < 0x300000 || indirect >= 0xFF0000,
            "candidate 0x61032 indirect pointer outside ROM/RAM value=" + std::to_string(indirect));
    const auto indirect_byte = read(indirect, 1);
    State expected = entry_;
    expected[0] = 0;
    expected[2] = d2;
    expected[8] = source_ + 2;
    expected[11] = indirect;
    expected[15] = entry_stack_ + 4;
    expected[16] = return_pc_;
    expected[17] = entry_[17] & ~kCcrMask;
    if (indirect_byte != 0) {
        expected[17] |= indirect_byte & 0x80 ? 8U : 0U;
        expected[17] |= indirect_byte == 0 ? 4U : 0U;
    } else {
        expected[17] |= 4U;
    }
    auto diff = compare_state(expected, state());
    require(diff.empty(), diff);
    const auto expected_count = first ? 25U : 24U;
    require(writes_.size() == expected_count, "candidate 0x61032 write count");
    unsigned i = 0;
    auto check = [&](unsigned address, int width, unsigned value) {
        require(writes_[i].address == address && writes_[i].width == width &&
                writes_[i].value == value, "FIRST_DIVERGENCE 0x61032 write[" + std::to_string(i) +
                "] expected_address=" + std::to_string(address) + " actual_address=" +
                std::to_string(writes_[i].address) + " expected_value=" + std::to_string(value) +
                " actual_value=" + std::to_string(writes_[i].value));
        ++i;
    };
    check(base_ + 2, 4, d2);
    check(base_ + 0x1E, 4, 0);
    if (first) check(base_ + 0x1E, 4, entry_[1] + d2);
    check(base_, 1, 0); check(base_ + 1, 1, 0); check(base_ + 0x75, 1, 0);
    check(base_ + 6, 2, 0x0101); check(base_ + 0x70, 1, 0);
    check(base_ + 0x18, 1, 0xFF); check(base_ + 0x1C, 2, 0);
    check(base_ + 0x76, 2, 0); check(base_ + 8, 2, 0);
    check(base_ + 0x71, 1, 4); check(base_ + 0x73, 1, 0xFF);
    check(base_ + 0x72, 1, 0); check(base_ + 0x14, 1, 0);
    check(base_ + 0x10, 1, 0); check(base_ + 0x11, 1, 7); check(base_ + 0x12, 1, 8);
    check(base_ + 0x16, 1, 0); check(base_ + 0x17, 1, 0); check(base_ + 0x23, 1, 0);
    check(base_ + 0x24, 4, 0); check(base_ + 0x28, 4, 0); check(base_ + 0x6C, 1, 0);
    ++comparisons;
    log_ << "{\"call\":" << calls << ",\"body_instruction_starts\":" << body_instructions
         << ",\"entry_cycles\":" << entry_cycles_ << ",\"entry_refresh\":" << entry_refresh_ << ",\"cycle_delta\":" << (api_.cycles ? api_.cycles() - entry_cycles_ : 0)
         << ",\"refresh_delta\":" << (api_.refresh_cycles ? api_.refresh_cycles() - entry_refresh_ : 0)
         << ",\"override\":false,\"interrupts\":0}\n";
    active_ = false;
}

void Candidate61032::apply_override() {
    const auto first = read(source_, 2);
    const auto d2 = entry_[2] + entry_[1];
    const auto indirect = d2;
    const auto indirect_byte = read(indirect, 1);
    api_.poke(base_ + 2, 4, d2);
    api_.poke(base_ + 0x1E, 4, 0);
    if (first) api_.poke(base_ + 0x1E, 4, entry_[1] + d2);
    for (const auto offset : {0U, 1U, 0x75U}) api_.poke(base_ + offset, 1, 0);
    api_.poke(base_ + 6, 2, 0x0101); api_.poke(base_ + 0x70, 1, 0);
    api_.poke(base_ + 0x18, 1, 0xFF); api_.poke(base_ + 0x1C, 2, 0);
    api_.poke(base_ + 0x76, 2, 0); api_.poke(base_ + 8, 2, 0);
    api_.poke(base_ + 0x71, 1, 4); api_.poke(base_ + 0x73, 1, 0xFF);
    api_.poke(base_ + 0x72, 1, 0); api_.poke(base_ + 0x14, 1, 0);
    api_.poke(base_ + 0x10, 1, 0); api_.poke(base_ + 0x11, 1, 7); api_.poke(base_ + 0x12, 1, 8);
    api_.poke(base_ + 0x16, 1, 0); api_.poke(base_ + 0x17, 1, 0); api_.poke(base_ + 0x23, 1, 0);
    api_.poke(base_ + 0x24, 4, 0); api_.poke(base_ + 0x28, 4, 0); api_.poke(base_ + 0x6C, 1, 0);
    auto sr = entry_[17] & ~kCcrMask;
    if (indirect_byte != 0) sr |= indirect_byte & 0x80 ? 8U : 0U;
    else sr |= 4U;
    api_.set_reg(0, 0); api_.set_reg(2, d2); api_.set_reg(8, source_ + 2);
    api_.set_reg(11, indirect); api_.set_reg(15, entry_stack_ + 4);
    api_.set_reg(17, sr); api_.set_reg(16, return_pc_);
    if (api_.set_return_state)
        api_.set_return_state(return_pc_, kEnd, (api_.peek(kEnd) << 8) | api_.peek(kEnd + 1));
    if (api_.add_cycles) api_.add_cycles(3304);
    ++override_calls;
    active_ = false;
    log_ << "{\"call\":" << calls << ",\"body_instruction_starts\":0,\"override\":true,\"entry_cycles\":" << entry_cycles_ << ",\"exit_cycles\":" << (api_.cycles ? api_.cycles() : 0) << ",\"interrupts\":0}\n";
}

void Candidate61032::event(int type, int width, unsigned address, unsigned value) {
    if (type == 1) {
        current_pc_ = address;
        if (active_ && address == return_pc_ && api_.reg(15) == entry_stack_ + 4) {
            finish();
            return;
        }
        if (address == kTarget) {
            if (mode_ == Mode::EMULATED) { ++calls; return; }
            require(!active_, "nested candidate 0x61032 call"); begin(); return;
        }
        if (active_) {
            require(address >= kTarget && address < kEnd, "candidate 0x61032 left body");
            ++body_instructions;
        }
        return;
    }
    if (!active_) return;
    address &= 0xFFFFFF;
    if (type == 2) {
        const bool stack = address >= entry_stack_ && address + width <= entry_stack_ + 4;
        const bool source = address >= source_ && address + width <= source_ + 2;
        const bool base = address >= base_ && address + width <= base_ + 0x77;
        const bool indirect = address == read(base_ + 2, 4);
        require(stack || source || base || (indirect && width == 1),
                "candidate 0x61032 read outside contract");
        if (width == 4 && address == 0x64) { ++interrupts; require(false, "interrupt inside candidate"); }
        return;
    }
    if (type == 4) {
        require(width == 1 || width == 2 || width == 4,
                "candidate 0x61032 unsupported write width");
        const auto offset = address - base_;
        const bool allowed = (width == 4 && (offset == 2 || offset == 0x1E || offset == 0x24 || offset == 0x28)) ||
                             (width == 2 && (offset == 6 || offset == 8 || offset == 0x1C || offset == 0x76)) ||
                             (width == 1 && (offset == 0 || offset == 1 || offset == 0x11 || offset == 0x12 ||
                                             offset == 0x10 || offset == 0x14 || offset == 0x16 || offset == 0x17 || offset == 0x18 ||
                                             offset == 0x6C ||
                                             offset == 0x23 || offset == 0x70 || offset == 0x71 || offset == 0x72 ||
                                             offset == 0x73 || offset == 0x75));
        require(address >= base_ && allowed,
                "candidate 0x61032 write outside RAM contract address=" + std::to_string(address) +
                " width=" + std::to_string(width) + " base=" + std::to_string(base_));
        writes_.push_back({address, width, value});
    }
}

void Candidate61032::hook(int type, int width, unsigned address, unsigned value) noexcept {
    if (!error_.empty()) return;
    try { event(type, width, address, value); }
    catch (const std::exception& error) { fail(error.what()); }
}

} // namespace oasis::hybrid
