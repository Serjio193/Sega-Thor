#include "tools/hybrid/candidate_604bc.hpp"
#include "core/ram_flag_routine.hpp"

#include <stdexcept>

namespace oasis::hybrid {
namespace {
constexpr unsigned kTarget = 0x604BC;
constexpr unsigned kEnd = 0x604E6;
constexpr unsigned kFirstFlag = 0xFF0628;
constexpr unsigned kSecondFlag = 0xFF06F2;
constexpr unsigned kAbsoluteOutput = 0xFF0016;

oasis::core::RamFlagRoutineContract portable_contract() {
    return {0x604BC, 0x604C2, 0x604C8, 0x604CE, 0x604D4, 0x604D8,
            0x604DA, 0x604DC, 0x604DE, 0x604E4, 0x604E6, kFirstFlag,
            kSecondFlag, kAbsoluteOutput};
}

class ApiMachine final : public oasis::core::RamFlagRoutineMachine {
public:
    explicit ApiMachine(CandidateApi api) : api_(api) {}

    std::uint32_t reg(unsigned index) const override { return api_.reg(index); }
    void set_reg(unsigned index, std::uint32_t value) override { api_.set_reg(index, value); }
    std::uint32_t read(std::uint32_t address, unsigned width) override {
        std::uint32_t value = 0;
        for (unsigned i = 0; i < width; ++i) {
            const auto byte = api_.peek(address + i);
            if (byte < 0 || byte > 0xFF) throw std::runtime_error("RAM flag routine memory read failed");
            value = (value << 8U) | static_cast<unsigned>(byte);
        }
        return value;
    }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        api_.poke(address, static_cast<int>(width), value);
    }
    void begin(oasis::core::RamFlagRoutineStep step) override {
        static const unsigned opcodes[] = {
            0x4DF9, 0x08EE, 0x4DF9, 0x08EE, 0x41ED,
            0x51D8, 0x51D8, 0x51D8, 0x51F9, 0x4E75};
        static const unsigned pcs[] = {
            0x604BC, 0x604C2, 0x604C8, 0x604CE, 0x604D4,
            0x604D8, 0x604DA, 0x604DC, 0x604DE, 0x604E4};
        const auto index = static_cast<unsigned>(step);
        require(api_.fetch16 && api_.begin_instruction, "routine instruction bridge unavailable");
        opcode_ = opcodes[index];
        if (api_.set_return_state)
            api_.set_return_state(pcs[index], pcs[index], opcode_);
        else
            api_.set_reg(16U, pcs[index]);
        const auto pc_before = api_.reg(16);
        const auto actual = api_.fetch16();
        if (actual != opcode_)
            throw std::runtime_error("0x604BC opcode/prefetch mismatch expected=" +
                                     std::to_string(opcode_) + " actual=" +
                                     std::to_string(actual) + " pc_before=" +
                                     std::to_string(pc_before) + " pc_after=" +
                                     std::to_string(api_.reg(16)) + " step=" +
                                     std::to_string(index));
        api_.begin_instruction(opcode_);
        switch (step) {
        case oasis::core::RamFlagRoutineStep::FIRST_FLAG_ADDRESS:
        case oasis::core::RamFlagRoutineStep::SECOND_FLAG_ADDRESS:
            require(api_.fetch16() == 0x00FFU, "0x604BC LEA high extension mismatch");
            require(api_.fetch16() == (step == oasis::core::RamFlagRoutineStep::FIRST_FLAG_ADDRESS ?
                                       0x0628U : 0x06F2U), "0x604BC LEA low extension mismatch");
            break;
        case oasis::core::RamFlagRoutineStep::FIRST_FLAG_SET:
        case oasis::core::RamFlagRoutineStep::SECOND_FLAG_SET:
        {
            const auto extension = api_.fetch16();
            if (extension != 0x0004U)
                throw std::runtime_error("0x604BC BSET extension mismatch pc=" +
                                         std::to_string(api_.reg(16U)) + " actual=" +
                                         std::to_string(extension) + " peek=" +
                                         std::to_string(api_.peek(0x604C6U)));
            break;
        }
        case oasis::core::RamFlagRoutineStep::OUTPUT_ADDRESS:
            require(api_.fetch16() == 0x0005U, "0x604BC output LEA extension mismatch");
            break;
        case oasis::core::RamFlagRoutineStep::ABSOLUTE_FALSE:
            require(api_.fetch16() == 0x00FFU, "0x604BC absolute Scc high extension mismatch");
            require(api_.fetch16() == 0x0016U, "0x604BC absolute Scc low extension mismatch");
            break;
        default:
            break;
        }
    }
    void finish(oasis::core::RamFlagRoutineStep) override {
        require(api_.finish_instruction, "routine instruction bridge unavailable");
        api_.finish_instruction(opcode_);
    }
    oasis::core::RoutineBoundaryReason boundary() override {
        if (!api_.boundary_reason) return oasis::core::RoutineBoundaryReason::CONTINUE;
        switch (api_.boundary_reason()) {
        case 2: return oasis::core::RoutineBoundaryReason::EVENT;
        case 3: return oasis::core::RoutineBoundaryReason::INTERRUPT;
        case 4: return oasis::core::RoutineBoundaryReason::TRACE;
        case 5: return oasis::core::RoutineBoundaryReason::FALLBACK;
        default: return oasis::core::RoutineBoundaryReason::CONTINUE;
        }
    }
    void complete_return(std::uint32_t return_pc) override {
        api_.set_reg(16U, return_pc);
        if (api_.set_return_state)
            api_.set_return_state(return_pc, kEnd, (api_.peek(kEnd) << 8) | api_.peek(kEnd + 1));
    }
private:
    static void require(bool condition, const char* message) {
        if (!condition) throw std::runtime_error(message);
    }
    CandidateApi api_;
    unsigned opcode_{};
};
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
    if (mode_ == Mode::NATIVE_OVERRIDE && !block_dispatch_) apply_override();
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

int Candidate604BC::dispatch_result(oasis::core::RoutineExitReason reason) {
    switch (reason) {
    case oasis::core::RoutineExitReason::NORMAL: return 1;
    case oasis::core::RoutineExitReason::EVENT: return 2;
    case oasis::core::RoutineExitReason::INTERRUPT: return 3;
    case oasis::core::RoutineExitReason::TRACE: return 4;
    case oasis::core::RoutineExitReason::FALLBACK: return 0;
    }
    return 0;
}

int Candidate604BC::execute_native(unsigned entry_token) {
    // The execute hook reports the entry PC before the core's prefetched PC
    // cursor; rebase the adapter cursor to the exact entry before refetching.
    api_.set_reg(16U, entry_token);
    if (api_.reg(16U) != entry_token)
        throw std::runtime_error("0x604BC PC rebase failed actual=" +
                                 std::to_string(api_.reg(16U)));
    ApiMachine machine(api_);
    const auto result = oasis::core::execute_ram_flag_routine(
        machine, portable_contract(), entry_token, continuation_);
    native_instructions += result.guest_instructions;
    native_boundary_yields += result.boundary_yields;
    native_call_instructions_ += result.guest_instructions;
    if (result.reason != oasis::core::RoutineExitReason::NORMAL) return dispatch_result(result.reason);
    require(!continuation_.active && result.next_token == kEnd,
            "portable RAM flag routine returned with invalid continuation");
    ++override_calls;
    active_ = false;
    log_ << "{\"call\":" << calls << ",\"body_instruction_starts\":0,\"override\":true,\"native_instructions\":"
         << native_call_instructions_ << ",\"entry_cycles\":" << entry_cycles_
         << ",\"exit_cycles\":" << (api_.cycles ? api_.cycles() : 0)
         << ",\"entry_refresh\":" << entry_refresh_ << ",\"exit_refresh\":"
         << (api_.refresh_cycles ? api_.refresh_cycles() : 0) << ",\"interrupts\":0}\n";
    native_call_instructions_ = 0;
    return 1;
}

void Candidate604BC::apply_override() {
    continuation_ = {};
    native_invocations += 1U;
    const auto result = execute_native(kTarget);
    require(result == 1, "portable RAM flag routine yielded without block continuation");
}

int Candidate604BC::dispatch(unsigned address) noexcept {
    if (mode_ != Mode::NATIVE_OVERRIDE || !error_.empty()) return 0;
    if (address != kTarget && !(continuation_.active && address == continuation_.next_token)) return 0;
    try {
        current_pc_ = address;
        if (!active_) {
            require(address == kTarget, "candidate 0x604BC continuation without entry");
            block_dispatch_ = true;
            begin();
            block_dispatch_ = false;
            continuation_ = {};
            native_invocations += 1U;
        } else {
            ++native_resumptions;
        }
        return execute_native(address);
    } catch (const std::exception& error) {
        block_dispatch_ = false;
        fail(error.what());
        return 0;
    }
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
