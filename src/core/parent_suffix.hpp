#pragma once

#include "core/ram_flag_routine.hpp"

#include <array>
#include <cstdint>

namespace oasis::core {

enum class ParentSuffixStep {
    FIRST_OUTPUT_CLEAR,
    RAM_FLAG_CALL,
    OUTPUT_CLEAR_0,
    OUTPUT_CLEAR_1,
    OUTPUT_CLEAR_2,
    ABSOLUTE_OUTPUT_CLEAR,
    PARENT_HANDOFF,
};

// The helper owns only its safe-RAM writes and the structural RamFlag call.
// All tokens and addresses are supplied by the adapter; none identify ROM PCs.
struct ParentSuffixContract {
    std::uint32_t entry_token{};
    std::uint32_t ram_flag_call_token{};
    std::uint32_t ram_flag_phase_token{};
    std::uint32_t output_0_token{};
    std::uint32_t output_1_token{};
    std::uint32_t output_2_token{};
    std::uint32_t absolute_output_token{};
    std::uint32_t handoff_token{};
    std::uint32_t continuation_token{};
    std::uint32_t first_output_address{};
    std::array<std::uint32_t, 3> output_addresses{};
    std::uint32_t absolute_output_address{};
    std::uint32_t expected_base_address{};
    unsigned base_register{13U};
};

struct ParentSuffixContinuation {
    bool active{};
    std::uint32_t next_token{};
    RoutineContinuation ram_flag{};
};

class ParentSuffixMachine : public RamFlagRoutineMachine {
public:
    ~ParentSuffixMachine() override = default;
    virtual void begin_suffix(ParentSuffixStep step) = 0;
    virtual void finish_suffix(ParentSuffixStep step) = 0;
};

[[nodiscard]] RoutineExit execute_parent_suffix(
    ParentSuffixMachine& machine, const ParentSuffixContract& contract,
    const RamFlagRoutineContract& ram_flag_contract,
    std::uint32_t entry_token, ParentSuffixContinuation& continuation);

} // namespace oasis::core
