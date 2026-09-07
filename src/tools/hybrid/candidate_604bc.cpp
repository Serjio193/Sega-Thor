#include "tools/hybrid/candidate_604bc.hpp"

#include <stdexcept>

namespace oasis::hybrid {
namespace {
constexpr unsigned kTarget = 0x604BC;
constexpr unsigned kEnd = 0x604E6;
constexpr unsigned kFirstFlag = 0xFF0628;
constexpr unsigned kSecondFlag = 0xFF06F2;
constexpr unsigned kAbsoluteOutput = 0xFF0016;
}

Candidate604BC::Candidate604BC(CandidateApi api, Mode mode, std::ostream& log)
    : api_(api), mode_(mode), log_(log) {}

Candidate604BC::State Candidate604BC::state() const {
    State value{};
    for (unsigned i = 0; i < value.size(); ++i) value[i] = api_.reg(i);
    return value;
}

int Candidate604BC::peek(unsigned address) const {
    const auto value = api_.peek(address);
    if (value < 0 || value > 0xFF) throw std::runtime_error("candidate 0x604BC memory peek failed");
    return value;
}

void Candidate604BC::require(bool condition, const std::string& message) {
    if (!condition) throw std::runtime_error(message);
}

void Candidate604BC::fail(const std::string& message) {
    ++divergences;
    error_ = "FIRST_DIVERGENCE target=0x604BC call=" + std::to_string(calls) +
             " pc=" + std::to_string(current_pc_) + " " + message;
    active_ = false;
}

void Candidate604BC::begin() {
    require(++calls <= 128, "candidate 0x604BC call budget exceeded");
    entry_ = state();
    require(entry_[16] == kTarget && entry_[15] >= 4 && !(entry_[15] & 1),
            "candidate 0x604BC entry bounds");
    require(entry_[13] >= 0xFF0000 && entry_[13] < 0x1000000,
            "candidate 0x604BC A5 is not RAM");
    entry_stack_ = entry_[15];
    entry_cycles_ = api_.cycles ? api_.cycles() : 0;
    entry_refresh_ = api_.refresh_cycles ? api_.refresh_cycles() : 0;
    return_pc_ = (peek(entry_stack_) << 24U) | (peek(entry_stack_ + 1) << 16U) |
                 (peek(entry_stack_ + 2) << 8U) | peek(entry_stack_ + 3);
    require(return_pc_ < 0x300000 && !(return_pc_ & 1), "candidate 0x604BC return bound");
    output_base_ = entry_[13] + 5;
    first_flag_ = peek(kFirstFlag);
    second_flag_ = peek(kSecondFlag);
    writes_.clear();
    body_instructions = 0;
    active_ = true;
    if (mode_ == Mode::NATIVE_OVERRIDE) apply_override();
}

std::string Candidate604BC::compare_state(const State& expected, const State& actual) const {
    for (unsigned i = 0; i < expected.size(); ++i)
        if (expected[i] != actual[i])
            return "FIRST_DIVERGENCE register[" + std::to_string(i) + "] expected=" +
                   std::to_string(expected[i]) + " actual=" + std::to_string(actual[i]);
    return {};
}

std::string Candidate604BC::compare_byte(unsigned address, unsigned expected, const char* name) const {
    if (peek(address) != static_cast<int>(expected & 0xFF))
        return std::string("FIRST_DIVERGENCE ") + name + " address=" + std::to_string(address);
    return {};
}

void Candidate604BC::finish() {
    State expected = entry_;
    expected[8] = entry_[13] + 8;
    expected[14] = kSecondFlag;
    expected[15] = entry_stack_ + 4;
    expected[16] = return_pc_;
    expected[17] = (entry_[17] & ~4U) | ((second_flag_ & 0x10) ? 0U : 4U);
    auto diff = compare_state(expected, state());
    require(diff.empty(), diff);
    const auto scc = (entry_[17] & 1U) ? 0xFFU : 0U;
    require(writes_.size() == 6, "candidate 0x604BC write count");
    require(writes_[0].address == kFirstFlag && writes_[0].value == (first_flag_ | 0x10),
            "FIRST_DIVERGENCE 0x604BC first BSET");
    require(writes_[1].address == kSecondFlag && writes_[1].value == (second_flag_ | 0x10),
            "FIRST_DIVERGENCE 0x604BC second BSET");
    for (unsigned i = 0; i < 3; ++i)
        require(writes_[i + 2].address == output_base_ + i && writes_[i + 2].value == scc,
                "FIRST_DIVERGENCE 0x604BC postincrement write");
    require(writes_[5].address == kAbsoluteOutput && writes_[5].value == scc,
            "FIRST_DIVERGENCE 0x604BC absolute write");
    ++comparisons;
    log_ << "{\"call\":" << calls << ",\"body_instruction_starts\":" << body_instructions
         << ",\"entry_cycles\":" << entry_cycles_ << ",\"entry_refresh\":" << entry_refresh_ << ",\"cycle_delta\":" << (api_.cycles ? api_.cycles() - entry_cycles_ : 0)
         << ",\"refresh_delta\":" << (api_.refresh_cycles ? api_.refresh_cycles() - entry_refresh_ : 0)
         << ",\"override\":false,\"interrupts\":0}\n";
    active_ = false;
}

void Candidate604BC::apply_override() {
    const auto scc = (entry_[17] & 1U) ? 0xFFU : 0U;
    api_.poke(kFirstFlag, 1, first_flag_ | 0x10);
    api_.poke(kSecondFlag, 1, second_flag_ | 0x10);
    for (unsigned i = 0; i < 3; ++i) api_.poke(output_base_ + i, 1, scc);
    api_.poke(kAbsoluteOutput, 1, scc);
    api_.set_reg(11, output_base_ + 3);
    api_.set_reg(14, kSecondFlag);
    api_.set_reg(15, entry_stack_ + 4);
    api_.set_reg(17, (entry_[17] & ~4U) | ((second_flag_ & 0x10) ? 0U : 4U));
    api_.set_reg(16, return_pc_);
    if (api_.set_return_state)
        api_.set_return_state(return_pc_, kEnd, (peek(kEnd) << 8) | peek(kEnd + 1));
    if (api_.add_cycles) api_.add_cycles(1022);
    ++override_calls;
    active_ = false;
    log_ << "{\"call\":" << calls << ",\"body_instruction_starts\":0,\"override\":true,\"entry_cycles\":" << entry_cycles_ << ",\"exit_cycles\":" << (api_.cycles ? api_.cycles() : 0) << ",\"interrupts\":0}\n";
}

void Candidate604BC::event(int type, int width, unsigned address, unsigned value) {
    if (type == 1) {
        current_pc_ = address;
        if (active_ && address == return_pc_ && api_.reg(15) == entry_stack_ + 4) {
            finish();
            return;
        }
        if (address == kTarget) {
            if (mode_ == Mode::EMULATED) { ++calls; return; }
            require(!active_, "nested candidate 0x604BC call");
            begin();
            return;
        }
        if (active_) {
            require(address >= kTarget && address < kEnd, "candidate 0x604BC left body");
            ++body_instructions;
        }
        return;
    }
    if (!active_) return;
    address &= 0xFFFFFF;
    if (type == 2) {
        const bool flag = (width == 1 && (address == kFirstFlag || address == kSecondFlag));
        const bool stack = address >= entry_stack_ && address + width <= entry_stack_ + 4;
        require(flag || stack, "candidate 0x604BC read outside contract");
        return;
    }
    if (type == 4) {
        require(width == 1 && ((address >= output_base_ && address < output_base_ + 3) ||
                               address == kAbsoluteOutput || address == kFirstFlag || address == kSecondFlag),
                "candidate 0x604BC write outside contract address=" + std::to_string(address) +
                " width=" + std::to_string(width) + " output_base=" + std::to_string(output_base_) +
                " a5=" + std::to_string(entry_[13]) + " a0=" + std::to_string(entry_[8]));
        writes_.push_back({address, value & 0xFFU});
    }
}

void Candidate604BC::hook(int type, int width, unsigned address, unsigned value) noexcept {
    if (!error_.empty()) return;
    try { event(type, width, address, value); }
    catch (const std::exception& error) { fail(error.what()); }
}

} // namespace oasis::hybrid
