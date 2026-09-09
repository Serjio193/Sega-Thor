#include "tools/hybrid/candidate_parent_suffix.hpp"

#include "core/ram_flag_routine.hpp"

#include <algorithm>
#include <stdexcept>
#include <unordered_map>
#include <utility>

namespace oasis::hybrid {
namespace {
constexpr unsigned kTarget = 0x604F0;
constexpr unsigned kRamFlag = 0x604BC;
constexpr unsigned kRamReturn = 0x604FA;
constexpr unsigned kEpilogue = 0x611D6;
constexpr unsigned kExpectedBase = 0xFF001A;

oasis::core::RamFlagRoutineContract ram_contract() {
    return {0x100, 0x101, 0x102, 0x103, 0x104, 0x105, 0x106, 0x107,
            0x108, 0x109, 0x10A, 0xFF0628, 0xFF06F2, 0xFF0016};
}

oasis::core::ParentSuffixContract suffix_contract() {
    return {1, 2, 3, 4, 5, 6, 7, 8, 9, 0xFF0012,
            {0xFF0010, 0xFF0011, 0xFF0013}, 0xFF0014, kExpectedBase};
}

oasis::core::RoutineBoundaryReason boundary_reason(const CandidateApi& api) {
    if (!api.boundary_reason) return oasis::core::RoutineBoundaryReason::CONTINUE;
    switch (api.boundary_reason()) {
    case 2: return oasis::core::RoutineBoundaryReason::EVENT;
    case 3: return oasis::core::RoutineBoundaryReason::INTERRUPT;
    case 4: return oasis::core::RoutineBoundaryReason::TRACE;
    case 5: return oasis::core::RoutineBoundaryReason::FALLBACK;
    default: return oasis::core::RoutineBoundaryReason::CONTINUE;
    }
}

unsigned ram_opcode(oasis::core::RamFlagRoutineStep step) {
    switch (step) {
    case oasis::core::RamFlagRoutineStep::FIRST_FLAG_ADDRESS: return 0x4DF9;
    case oasis::core::RamFlagRoutineStep::FIRST_FLAG_SET: return 0x08EE;
    case oasis::core::RamFlagRoutineStep::SECOND_FLAG_ADDRESS: return 0x4DF9;
    case oasis::core::RamFlagRoutineStep::SECOND_FLAG_SET: return 0x08EE;
    case oasis::core::RamFlagRoutineStep::OUTPUT_ADDRESS: return 0x41ED;
    case oasis::core::RamFlagRoutineStep::OUTPUT_FALSE_0: return 0x51D8;
    case oasis::core::RamFlagRoutineStep::OUTPUT_FALSE_1: return 0x51D8;
    case oasis::core::RamFlagRoutineStep::OUTPUT_FALSE_2: return 0x51D8;
    case oasis::core::RamFlagRoutineStep::ABSOLUTE_FALSE: return 0x51F9;
    case oasis::core::RamFlagRoutineStep::RETURN: return 0x4E75;
    }
    throw std::runtime_error("invalid RamFlag step");
}

unsigned ram_pc(oasis::core::RamFlagRoutineStep step) {
    static constexpr unsigned pcs[] = {0x604BC, 0x604C2, 0x604C8, 0x604CE,
        0x604D4, 0x604D8, 0x604DA, 0x604DC, 0x604DE, 0x604E4};
    return pcs[static_cast<unsigned>(step)];
}

} // namespace

class CandidateParentSuffix::ApiMachine final : public oasis::core::ParentSuffixMachine {
public:
    explicit ApiMachine(CandidateApi api) : api_(api) {}
    unsigned suffix_instructions{}, suffix_yields{}, suffix_resumptions{};
    unsigned ram_instructions{}, ram_yields{}, ram_resumptions{};

    std::uint32_t reg(unsigned index) const override { return api_.reg(index); }
    void set_reg(unsigned index, std::uint32_t value) override { api_.set_reg(index, value); }
    std::uint32_t read(std::uint32_t address, unsigned width) override {
        std::uint32_t result = 0;
        for (unsigned i = 0; i < width; ++i) {
            const auto value = api_.peek(address + i);
            if (value < 0 || value > 0xFF) throw std::runtime_error("suffix read outside RAM");
            result = (result << 8U) | static_cast<unsigned>(value);
        }
        return result;
    }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        if (width != 1U) throw std::runtime_error("suffix write width mismatch");
        if (address < 0xFF0000U) throw std::runtime_error("suffix write outside main RAM");
        api_.poke(address, 1, value);
    }
    void begin(oasis::core::RamFlagRoutineStep step) override {
        const auto pc = ram_pc(step);
        const auto opcode = ram_opcode(step);
        set_instruction(pc, opcode);
        switch (step) {
        case oasis::core::RamFlagRoutineStep::FIRST_FLAG_ADDRESS:
        case oasis::core::RamFlagRoutineStep::SECOND_FLAG_ADDRESS:
            require_fetch(0x00FF, "RamFlag LEA high extension");
            require_fetch(step == oasis::core::RamFlagRoutineStep::FIRST_FLAG_ADDRESS ? 0x0628 : 0x06F2,
                          "RamFlag LEA low extension");
            break;
        case oasis::core::RamFlagRoutineStep::FIRST_FLAG_SET:
        case oasis::core::RamFlagRoutineStep::SECOND_FLAG_SET:
            require_fetch(0x0004, "RamFlag BSET extension");
            break;
        case oasis::core::RamFlagRoutineStep::OUTPUT_ADDRESS:
            require_fetch(0x0005, "RamFlag output LEA extension");
            break;
        case oasis::core::RamFlagRoutineStep::ABSOLUTE_FALSE:
            require_fetch(0x00FF, "RamFlag output high extension");
            require_fetch(0x0016, "RamFlag output low extension");
            break;
        default: break;
        }
        pending_ram_ = true;
    }
    void finish(oasis::core::RamFlagRoutineStep) override {
        if (!api_.finish_instruction) throw std::runtime_error("suffix finish bridge unavailable");
        api_.finish_instruction(opcode_);
        ++ram_instructions;
    }
    void begin_suffix(oasis::core::ParentSuffixStep step) override {
        const auto [pc, opcode] = suffix_instruction(step);
        set_instruction(pc, opcode);
        if (step == oasis::core::ParentSuffixStep::RAM_FLAG_CALL) {
            require_fetch(0xFFC4, "suffix BSR displacement");
            const auto stack = api_.reg(15) - 4U;
            api_.poke(stack, 4, kRamReturn);
            api_.set_reg(15, stack);
        } else if (step == oasis::core::ParentSuffixStep::FIRST_OUTPUT_CLEAR) {
            require_fetch(0x00FF, "suffix first output high extension");
            require_fetch(0x0012, "suffix first output low extension");
        } else if (step == oasis::core::ParentSuffixStep::OUTPUT_CLEAR_0) {
            require_fetch(0x00FF, "suffix output 0 high extension");
            require_fetch(0x0010, "suffix output 0 low extension");
        } else if (step == oasis::core::ParentSuffixStep::OUTPUT_CLEAR_1) {
            require_fetch(0x00FF, "suffix output 1 high extension");
            require_fetch(0x0011, "suffix output 1 low extension");
        } else if (step == oasis::core::ParentSuffixStep::OUTPUT_CLEAR_2) {
            require_fetch(0x00FF, "suffix output 2 high extension");
            require_fetch(0x0013, "suffix output 2 low extension");
        } else if (step == oasis::core::ParentSuffixStep::ABSOLUTE_OUTPUT_CLEAR) {
            require_fetch(0x00FF, "suffix absolute output high extension");
            require_fetch(0x0014, "suffix absolute output low extension");
        } else if (step == oasis::core::ParentSuffixStep::PARENT_HANDOFF) {
            require_fetch(0x0CC2, "suffix branch displacement");
        }
        pending_suffix_ = true;
    }
    void finish_suffix(oasis::core::ParentSuffixStep step) override {
        if (!api_.finish_instruction) throw std::runtime_error("suffix finish bridge unavailable");
        api_.finish_instruction(opcode_);
        ++suffix_instructions;
        if (api_.add_cycles && step != oasis::core::ParentSuffixStep::RAM_FLAG_CALL &&
            step != oasis::core::ParentSuffixStep::PARENT_HANDOFF)
            api_.add_cycles(-14);
        if (step == oasis::core::ParentSuffixStep::PARENT_HANDOFF) {
            if (api_.set_return_state) api_.set_return_state(kEpilogue, kEpilogue, 0x7000);
        }
    }
    oasis::core::RoutineBoundaryReason boundary() override {
        const auto reason = boundary_reason(api_);
        if (reason != oasis::core::RoutineBoundaryReason::CONTINUE) {
            if (pending_suffix_) ++suffix_yields;
            if (pending_ram_) ++ram_yields;
        }
        pending_suffix_ = pending_ram_ = false;
        return reason;
    }
    void complete_return(std::uint32_t return_pc) override {
        api_.set_reg(16U, return_pc);
        if (api_.set_return_state)
            api_.set_return_state(return_pc, 0x604E6, 0x6100);
        pending_suffix_ = true;
    }

private:
    void require_fetch(unsigned expected, const char* message) {
        if (!api_.fetch16 || api_.fetch16() != expected) throw std::runtime_error(message);
    }
    void set_instruction(unsigned pc, unsigned opcode) {
        if (!api_.fetch16 || !api_.begin_instruction) throw std::runtime_error("suffix instruction bridge unavailable");
        if (api_.set_return_state) api_.set_return_state(pc, pc, opcode);
        if (api_.fetch16() != opcode) throw std::runtime_error("suffix opcode mismatch");
        api_.begin_instruction(opcode);
        opcode_ = opcode;
    }
    static std::pair<unsigned, unsigned> suffix_instruction(oasis::core::ParentSuffixStep step) {
        switch (step) {
        case oasis::core::ParentSuffixStep::FIRST_OUTPUT_CLEAR: return {0x604F0, 0x51F9};
        case oasis::core::ParentSuffixStep::RAM_FLAG_CALL: return {0x604F6, 0x6100};
        case oasis::core::ParentSuffixStep::OUTPUT_CLEAR_0: return {0x604FA, 0x51F9};
        case oasis::core::ParentSuffixStep::OUTPUT_CLEAR_1: return {0x60500, 0x51F9};
        case oasis::core::ParentSuffixStep::OUTPUT_CLEAR_2: return {0x60506, 0x51F9};
        case oasis::core::ParentSuffixStep::ABSOLUTE_OUTPUT_CLEAR: return {0x6050C, 0x51F9};
        case oasis::core::ParentSuffixStep::PARENT_HANDOFF: return {0x60512, 0x6000};
        }
        throw std::runtime_error("invalid suffix step");
    }
    CandidateApi api_;
    unsigned opcode_{};
    bool pending_suffix_{}, pending_ram_{};
};

class CandidateParentSuffix::ShadowMachine final : public oasis::core::ParentSuffixMachine {
public:
    ShadowMachine(const CandidateApi& api, const CandidateParentSuffix::State& initial)
        : api_(api), state_(initial) {}
    std::vector<CandidateParentSuffix::Write> writes;
    std::uint32_t reg(unsigned index) const override { return state_[index]; }
    void set_reg(unsigned index, std::uint32_t value) override { state_[index] = value; }
    std::uint32_t read(std::uint32_t address, unsigned width) override {
        std::uint32_t result = 0;
        for (unsigned i = 0; i < width; ++i) {
            const auto it = memory_.find(address + i);
            const auto value = it == memory_.end() ? api_.peek(address + i) : it->second;
            if (value < 0 || value > 0xFF) throw std::runtime_error("shadow read failed");
            result = (result << 8U) | static_cast<unsigned>(value);
        }
        return result;
    }
    void write(std::uint32_t address, unsigned width, std::uint32_t value) override {
        if (width != 1U && width != 4U) throw std::runtime_error("shadow width mismatch");
        writes.push_back({address, width, value});
        for (unsigned i = 0; i < width; ++i)
            memory_[address + i] = (value >> (8U * (width - i - 1U))) & 0xFFU;
    }
    void begin(oasis::core::RamFlagRoutineStep) override {}
    void finish(oasis::core::RamFlagRoutineStep) override {}
    void begin_suffix(oasis::core::ParentSuffixStep step) override {
        if (step == oasis::core::ParentSuffixStep::RAM_FLAG_CALL) {
            const auto stack = state_[15] - 4U;
            write(stack, 4, kRamReturn);
            state_[15] = stack;
        }
    }
    void finish_suffix(oasis::core::ParentSuffixStep) override {}
    oasis::core::RoutineBoundaryReason boundary() override {
        return oasis::core::RoutineBoundaryReason::CONTINUE;
    }
    void complete_return(std::uint32_t return_pc) override { state_[16] = return_pc; }
    const CandidateParentSuffix::State& state() const { return state_; }
private:
    CandidateApi api_;
    CandidateParentSuffix::State state_;
    std::unordered_map<std::uint32_t, std::uint8_t> memory_;
};
CandidateParentSuffix::CandidateParentSuffix(CandidateApi api, Mode mode, std::ostream& log)
    : api_(api), mode_(mode), log_(log) {}

CandidateParentSuffix::State CandidateParentSuffix::state() const {
    State result{};
    for (unsigned i = 0; i < result.size(); ++i) result[i] = api_.reg(i);
    return result;
}

ReplacementMetrics CandidateParentSuffix::metrics() const {
    return {calls_, comparisons_, divergences_, 0, override_calls_, 0,
            0, 0, 0, 0, 0, ram_instructions_, ram_invocations_, ram_yields_, ram_resumptions_,
            helper_instructions_, helper_invocations_, helper_yields_, helper_resumptions_};
}

void CandidateParentSuffix::fail(const std::string& message) {
    ++divergences_;
    error_ = "FIRST_DIVERGENCE target=0x604F0 " + message;
    active_ = false;
    shadow_active_ = false;
}

unsigned CandidateParentSuffix::resume_pc() const {
    if (continuation_.next_token == 2U)
        return 0x604F6;
    if (continuation_.next_token == 3U)
        return continuation_.ram_flag.active ? ram_pc(static_cast<oasis::core::RamFlagRoutineStep>(
            continuation_.ram_flag.next_token - 0x100U)) : kRamFlag;
    switch (continuation_.next_token) {
    case 4: return 0x604FA; case 5: return 0x60500; case 6: return 0x60506;
    case 7: return 0x6050C; case 8: return 0x60512; case 9: return kEpilogue;
    default: return 0;
    }
}

int CandidateParentSuffix::result_code(oasis::core::RoutineExitReason reason) {
    switch (reason) {
    case oasis::core::RoutineExitReason::NORMAL: return 1;
    case oasis::core::RoutineExitReason::EVENT: return 2;
    case oasis::core::RoutineExitReason::INTERRUPT: return 3;
    case oasis::core::RoutineExitReason::TRACE: return 4;
    case oasis::core::RoutineExitReason::FALLBACK: return 0;
    }
    return 0;
}

int CandidateParentSuffix::execute_native(std::uint32_t token) {
    const auto entry_cycles = api_.cycles ? api_.cycles() : 0U;
    const auto entry_refresh = api_.refresh_cycles ? api_.refresh_cycles() : 0U;
    ApiMachine machine(api_);
    const bool ram_resume = continuation_.ram_flag.active;
    const auto result = oasis::core::execute_parent_suffix(
        machine, suffix_contract(), ram_contract(), token, continuation_);
    helper_instructions_ += machine.suffix_instructions;
    helper_yields_ += machine.suffix_yields;
    ram_instructions_ += machine.ram_instructions;
    ram_yields_ += machine.ram_yields;
    if (ram_resume) ++ram_resumptions_;
    if (!ram_resume && machine.ram_instructions > 0U) ++ram_invocations_;
    if (result.reason != oasis::core::RoutineExitReason::NORMAL) return result_code(result.reason);
    if (result.guest_instructions == 0U && result.next_token == 9U) {
        active_ = false;
        return 0;
    }
    if (result.next_token == 9U && !continuation_.active) {
        ++override_calls_;
        active_ = false;
        log_ << "{\"target\":\"0x604F0\",\"helper_instructions\":" << helper_instructions_
             << ",\"ramflag_instructions\":" << ram_instructions_
             << ",\"entry_cycles\":" << entry_cycles
             << ",\"exit_cycles\":" << (api_.cycles ? api_.cycles() : 0U)
             << ",\"entry_refresh\":" << entry_refresh
             << ",\"exit_refresh\":" << (api_.refresh_cycles ? api_.refresh_cycles() : 0U)
             << "}\n";
        return 1;
    }
    return 1;
}

int CandidateParentSuffix::dispatch(unsigned address) noexcept {
    if (mode_ != Mode::NATIVE_OVERRIDE || !error_.empty()) return 0;
    try {
        if (!active_) {
            if (address != kTarget) return 0;
            ++calls_;
            ++helper_invocations_;
            entry_ = state();
            if (entry_[16] != kTarget || entry_[13] != kExpectedBase || (entry_[15] & 1U))
                throw std::runtime_error("invalid parent suffix entry state");
            continuation_ = {};
            active_ = true;
            return execute_native(1U);
        }
        if (address != resume_pc()) throw std::runtime_error("parent suffix resume PC mismatch");
        if (continuation_.next_token == 9U && continuation_.active) {
            const auto result = execute_native(9U);
            return result;
        }
        ++helper_resumptions_;
        return execute_native(continuation_.next_token == 2U ? 2U : continuation_.next_token);
    } catch (const std::exception& error) {
        fail(error.what());
        return 0;
    }
}

void CandidateParentSuffix::begin_shadow() {
    entry_ = state();
    if (entry_[13] != kExpectedBase || (entry_[15] & 1U)) throw std::runtime_error("shadow entry contract");
    ShadowMachine machine(api_, entry_);
    auto continuation = oasis::core::ParentSuffixContinuation{};
    const auto result = oasis::core::execute_parent_suffix(
        machine, suffix_contract(), ram_contract(), 1U, continuation);
    if (result.reason != oasis::core::RoutineExitReason::NORMAL || continuation.active)
        throw std::runtime_error("shadow helper did not complete");
    expected_state_ = machine.state();
    expected_writes_ = machine.writes;
    writes_.clear();
    path_.clear();
    active_ = true;
    shadow_active_ = true;
}

void CandidateParentSuffix::finish_shadow() {
    const std::vector<unsigned> expected_path{0x604F0, 0x604F6, 0x604BC, 0x604C2,
        0x604C8, 0x604CE, 0x604D4, 0x604D8, 0x604DA, 0x604DC, 0x604DE,
        0x604E4, 0x604FA, 0x60500, 0x60506, 0x6050C, 0x60512, 0x611D6};
    if (path_ != expected_path) throw std::runtime_error("shadow path mismatch");
    if (writes_.size() != expected_writes_.size()) throw std::runtime_error("shadow write count mismatch");
    for (std::size_t i = 0; i < writes_.size(); ++i)
        if (writes_[i].address != expected_writes_[i].address || writes_[i].width != expected_writes_[i].width ||
            (writes_[i].value & 0xFFFFFFFFU) != (expected_writes_[i].value & 0xFFFFFFFFU))
            throw std::runtime_error("shadow write journal mismatch");
    const auto actual = state();
    for (unsigned i = 0; i < actual.size(); ++i)
        if (i != 16U && actual[i] != expected_state_[i]) throw std::runtime_error("shadow register mismatch");
    ++comparisons_;
    shadow_active_ = false;
    active_ = false;
}

void CandidateParentSuffix::hook(int type, int width, unsigned address, unsigned value) noexcept {
    if (mode_ != Mode::SHADOW_NATIVE || !error_.empty()) return;
    try {
        address &= 0xFFFFFFU;
        if (type == 1 && address == kTarget && !shadow_active_) {
            ++calls_;
            begin_shadow();
        }
        if (!shadow_active_) return;
        if (type == 1) {
            path_.push_back(address);
            if (address == kEpilogue) finish_shadow();
        } else if (type == 4) {
            writes_.push_back({address, static_cast<unsigned>(width), value});
        }
    } catch (const std::exception& error) {
        fail(error.what());
    }
}

} // namespace oasis::hybrid
