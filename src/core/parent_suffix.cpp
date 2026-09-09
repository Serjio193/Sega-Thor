#include "core/parent_suffix.hpp"

#include <array>
#include <stdexcept>

namespace oasis::core {
namespace {
void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

RoutineExitReason exit_reason(RoutineBoundaryReason reason) {
    switch (reason) {
    case RoutineBoundaryReason::EVENT: return RoutineExitReason::EVENT;
    case RoutineBoundaryReason::INTERRUPT: return RoutineExitReason::INTERRUPT;
    case RoutineBoundaryReason::TRACE: return RoutineExitReason::TRACE;
    case RoutineBoundaryReason::FALLBACK: return RoutineExitReason::FALLBACK;
    case RoutineBoundaryReason::CONTINUE: break;
    }
    return RoutineExitReason::NORMAL;
}

void validate(const ParentSuffixContract& contract,
              const RamFlagRoutineContract& ram_flag) {
    const std::array tokens{contract.entry_token, contract.ram_flag_call_token,
        contract.ram_flag_phase_token,
        contract.output_0_token, contract.output_1_token, contract.output_2_token,
        contract.absolute_output_token, contract.handoff_token,
        contract.continuation_token};
    for (auto left = tokens.begin(); left != tokens.end(); ++left)
        for (auto right = left + 1; right != tokens.end(); ++right)
            require(*left != *right, "parent suffix token collision");
    require(contract.base_register < 16U && contract.base_register >= 8U,
            "parent suffix base register is invalid");
    require(ram_flag.base_register == contract.base_register,
            "parent suffix/RamFlag base register mismatch");
    const std::array addresses{contract.first_output_address,
        contract.output_addresses[0], contract.output_addresses[1],
        contract.output_addresses[2], contract.absolute_output_address};
    for (auto left = addresses.begin(); left != addresses.end(); ++left)
        for (auto right = left + 1; right != addresses.end(); ++right)
            require(*left != *right, "parent suffix output address collision");
}

void validate_entry(const ParentSuffixContract& contract,
                    const ParentSuffixContinuation& continuation,
                    std::uint32_t entry_token) {
    if (!continuation.active) {
        require(entry_token == contract.entry_token ||
                entry_token == contract.continuation_token,
                "parent suffix initial entry mismatch");
        require(entry_token != contract.continuation_token,
                "parent suffix cannot start at its completed continuation");
        return;
    }
    require(entry_token == continuation.next_token,
            "parent suffix continuation token mismatch");
    require(entry_token != contract.entry_token,
            "parent suffix active continuation cannot use entry token");
}

RoutineExit finish_step(ParentSuffixMachine& machine,
                        ParentSuffixContinuation& continuation,
                        ParentSuffixStep step, std::uint32_t next,
                        unsigned& instructions, unsigned& yields) {
    machine.finish_suffix(step);
    ++instructions;
    continuation.active = true;
    continuation.next_token = next;
    const auto reason = machine.boundary();
    if (reason == RoutineBoundaryReason::CONTINUE)
        return {next, RoutineExitReason::NORMAL, instructions, 0U, yields};
    ++yields;
    return {next, exit_reason(reason), instructions, 0U, yields};
}

RoutineExit handoff(ParentSuffixMachine& machine,
                    ParentSuffixContinuation& continuation,
                    const ParentSuffixContract& contract,
                    unsigned& instructions, unsigned& yields) {
    auto result = finish_step(machine, continuation, ParentSuffixStep::PARENT_HANDOFF,
                              contract.continuation_token, instructions, yields);
    if (result.reason != RoutineExitReason::NORMAL) return result;
    continuation.active = false;
    continuation.next_token = contract.continuation_token;
    return result;
}
} // namespace

RoutineExit execute_parent_suffix(ParentSuffixMachine& machine,
                                  const ParentSuffixContract& contract,
                                  const RamFlagRoutineContract& ram_flag_contract,
                                  std::uint32_t entry_token,
                                  ParentSuffixContinuation& continuation) {
    validate(contract, ram_flag_contract);
    validate_entry(contract, continuation, entry_token);
    if (continuation.active && entry_token == contract.continuation_token) {
        continuation.active = false;
        continuation.next_token = contract.continuation_token;
        return {contract.continuation_token, RoutineExitReason::NORMAL, 0U, 0U, 0U};
    }
    require(machine.reg(contract.base_register) == contract.expected_base_address,
            "parent suffix required base register mismatch");
    unsigned instructions = 0U;
    unsigned yields = 0U;
    auto token = entry_token;
    for (;;) {
        if (token == contract.entry_token) {
            machine.begin_suffix(ParentSuffixStep::FIRST_OUTPUT_CLEAR);
            machine.write(contract.first_output_address, 1U, 0U);
            auto result = finish_step(machine, continuation,
                                      ParentSuffixStep::FIRST_OUTPUT_CLEAR,
                                      contract.ram_flag_call_token, instructions, yields);
            if (result.reason != RoutineExitReason::NORMAL) return result;
            token = result.next_token;
        }
        if (token == contract.ram_flag_call_token) {
            machine.begin_suffix(ParentSuffixStep::RAM_FLAG_CALL);
            auto result = finish_step(machine, continuation,
                                      ParentSuffixStep::RAM_FLAG_CALL,
                                      contract.ram_flag_phase_token, instructions, yields);
            if (result.reason != RoutineExitReason::NORMAL) return result;
            token = result.next_token;
        }
        if (token == contract.ram_flag_phase_token) {
            const auto ram_token = continuation.ram_flag.active ?
                continuation.ram_flag.next_token : ram_flag_contract.entry_token;
            const auto ram_result = execute_ram_flag_routine(
                machine, ram_flag_contract, ram_token, continuation.ram_flag);
            instructions += ram_result.guest_instructions;
            yields += ram_result.boundary_yields;
            if (ram_result.reason != RoutineExitReason::NORMAL) {
                continuation.active = true;
                continuation.next_token = contract.ram_flag_phase_token;
                return {contract.ram_flag_phase_token, ram_result.reason,
                        instructions, 0U, yields};
            }
            continuation.ram_flag = {};
            continuation.active = true;
            continuation.next_token = contract.output_0_token;
            const auto after_call = machine.boundary();
            if (after_call != RoutineBoundaryReason::CONTINUE) {
                ++yields;
                return {contract.output_0_token, exit_reason(after_call),
                        instructions, 0U, yields};
            }
            token = contract.output_0_token;
        }
        const std::array<std::pair<ParentSuffixStep, std::uint32_t>, 3> outputs{{
            {ParentSuffixStep::OUTPUT_CLEAR_0, contract.output_0_token},
            {ParentSuffixStep::OUTPUT_CLEAR_1, contract.output_1_token},
            {ParentSuffixStep::OUTPUT_CLEAR_2, contract.output_2_token}}};
        for (unsigned index = 0; index < outputs.size(); ++index) {
            if (token != outputs[index].second) continue;
            machine.begin_suffix(outputs[index].first);
            machine.write(contract.output_addresses[index], 1U, 0U);
            const auto next = index == 0U ? contract.output_1_token :
                index == 1U ? contract.output_2_token : contract.absolute_output_token;
            auto result = finish_step(machine, continuation, outputs[index].first,
                                      next, instructions, yields);
            if (result.reason != RoutineExitReason::NORMAL) return result;
            token = result.next_token;
        }
        if (token == contract.absolute_output_token) {
            machine.begin_suffix(ParentSuffixStep::ABSOLUTE_OUTPUT_CLEAR);
            machine.write(contract.absolute_output_address, 1U, 0U);
            auto result = finish_step(machine, continuation,
                                      ParentSuffixStep::ABSOLUTE_OUTPUT_CLEAR,
                                      contract.handoff_token, instructions, yields);
            if (result.reason != RoutineExitReason::NORMAL) return result;
            token = result.next_token;
        }
        if (token == contract.handoff_token)
            return handoff(machine, continuation, contract, instructions, yields);
        throw std::runtime_error("parent suffix reached an invalid continuation token");
    }
}

} // namespace oasis::core
