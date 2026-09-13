#include "core/ram_flag_routine.hpp"

#include <string>
#include <stdexcept>

namespace oasis::core {
namespace {
constexpr unsigned kZeroFlag = 0x04U;
constexpr unsigned kBit = 0x10U;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void validate(const RamFlagRoutineContract& contract) {
    const auto tokens = {contract.entry_token, contract.first_flag_token,
                         contract.second_flag_token, contract.second_flag_set_token,
                         contract.output_address_token,
                         contract.output_false_0_token, contract.output_false_1_token,
                         contract.output_false_2_token, contract.absolute_false_token,
                         contract.return_token, contract.continuation_token};
    for (auto left = tokens.begin(); left != tokens.end(); ++left)
        for (auto right = left + 1; right != tokens.end(); ++right)
            require(*left != *right, "RAM flag routine token collision");
    require(contract.status_register < 18U && contract.output_register >= 8U &&
                contract.output_register < 16U && contract.base_register >= 8U &&
                contract.base_register < 16U && contract.flag_register >= 8U &&
                contract.flag_register < 16U && contract.stack_register >= 8U &&
                contract.stack_register < 16U,
            "RAM flag routine register contract is invalid");
}

void validate_entry(const RamFlagRoutineContract& contract,
                    const RoutineContinuation& continuation,
                    std::uint32_t entry_token) {
    if (!continuation.active) {
        require(entry_token == contract.entry_token,
                "RAM flag routine initial entry must start at entry token");
        return;
    }
    require(entry_token == continuation.next_token && entry_token != contract.entry_token,
            "RAM flag routine continuation entry mismatch");
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

RoutineExit finish_step(RamFlagRoutineMachine& machine,
                        const RamFlagRoutineContract& contract,
                        RoutineContinuation& continuation, RamFlagRoutineStep step,
                        std::uint32_t next, unsigned& instructions,
                        unsigned& yields) {
    machine.finish(step);
    ++instructions;
    continuation.active = true;
    continuation.next_token = next;
    const auto reason = machine.boundary();
    if (reason == RoutineBoundaryReason::CONTINUE)
        return {next, RoutineExitReason::NORMAL, instructions, 0U, yields};
    ++yields;
    return {next, exit_reason(reason), instructions, 0U, yields};
}

void set_bit_test_flag(RamFlagRoutineMachine& machine,
                       const RamFlagRoutineContract& contract,
                       std::uint32_t old_value) {
    auto status = machine.reg(contract.status_register);
    status = (status & ~kZeroFlag) | ((old_value & kBit) ? 0U : kZeroFlag);
    machine.set_reg(contract.status_register, status);
}
}

RoutineExit execute_ram_flag_routine(RamFlagRoutineMachine& machine,
                                     const RamFlagRoutineContract& contract,
                                     std::uint32_t entry_token,
                                     RoutineContinuation& continuation) {
    validate(contract);
    validate_entry(contract, continuation, entry_token);
    auto token = entry_token;
    unsigned instructions = 0U;
    unsigned yields = 0U;

    for (;;) {
        if (token == contract.entry_token) {
            machine.begin(RamFlagRoutineStep::FIRST_FLAG_ADDRESS);
            machine.set_reg(contract.flag_register, contract.first_flag_address);
            auto result = finish_step(machine, contract, continuation,
                                      RamFlagRoutineStep::FIRST_FLAG_ADDRESS,
                                      contract.first_flag_token, instructions, yields);
            if (result.reason != RoutineExitReason::NORMAL) return result;
            token = result.next_token;
        }
        if (token == contract.first_flag_token) {
            machine.begin(RamFlagRoutineStep::FIRST_FLAG_SET);
            const auto old = machine.read(contract.first_flag_address, 1U);
            machine.write(contract.first_flag_address, 1U, old | kBit);
            set_bit_test_flag(machine, contract, old);
            auto result = finish_step(machine, contract, continuation,
                                      RamFlagRoutineStep::FIRST_FLAG_SET,
                                      contract.second_flag_token, instructions, yields);
            if (result.reason != RoutineExitReason::NORMAL) return result;
            token = result.next_token;
        }
        if (token == contract.second_flag_token) {
            machine.begin(RamFlagRoutineStep::SECOND_FLAG_ADDRESS);
            machine.set_reg(contract.flag_register, contract.second_flag_address);
            auto result = finish_step(machine, contract, continuation,
                                      RamFlagRoutineStep::SECOND_FLAG_ADDRESS,
                                      contract.second_flag_set_token, instructions, yields);
            if (result.reason != RoutineExitReason::NORMAL) return result;
            token = result.next_token;
        }
        if (token == contract.second_flag_set_token) {
            machine.begin(RamFlagRoutineStep::SECOND_FLAG_SET);
            const auto old = machine.read(contract.second_flag_address, 1U);
            machine.write(contract.second_flag_address, 1U, old | kBit);
            set_bit_test_flag(machine, contract, old);
            auto result = finish_step(machine, contract, continuation,
                                      RamFlagRoutineStep::SECOND_FLAG_SET,
                                      contract.output_address_token, instructions, yields);
            if (result.reason != RoutineExitReason::NORMAL) return result;
            token = result.next_token;
        }
        if (token == contract.output_address_token) {
            machine.begin(RamFlagRoutineStep::OUTPUT_ADDRESS);
            machine.set_reg(contract.output_register,
                            machine.reg(contract.base_register) + 5U);
            auto result = finish_step(machine, contract, continuation,
                                      RamFlagRoutineStep::OUTPUT_ADDRESS,
                                      contract.output_false_0_token, instructions, yields);
            if (result.reason != RoutineExitReason::NORMAL) return result;
            token = result.next_token;
        }
        if (token == contract.output_false_0_token ||
            token == contract.output_false_1_token || token == contract.output_false_2_token) {
            const auto step = token == contract.output_false_0_token ?
                RamFlagRoutineStep::OUTPUT_FALSE_0 : token == contract.output_false_1_token ?
                RamFlagRoutineStep::OUTPUT_FALSE_1 : RamFlagRoutineStep::OUTPUT_FALSE_2;
            const auto next = token == contract.output_false_0_token ? contract.output_false_1_token :
                token == contract.output_false_1_token ? contract.output_false_2_token :
                contract.absolute_false_token;
            machine.begin(step);
            const auto address = machine.reg(contract.output_register);
            machine.write(address, 1U, 0U);
            machine.set_reg(contract.output_register, address + 1U);
            auto result = finish_step(machine, contract, continuation, step, next, instructions, yields);
            if (result.reason != RoutineExitReason::NORMAL) return result;
            token = result.next_token;
            continue;
        }
        if (token == contract.absolute_false_token) {
            machine.begin(RamFlagRoutineStep::ABSOLUTE_FALSE);
            machine.write(contract.absolute_output_address, 1U, 0U);
            auto result = finish_step(machine, contract, continuation,
                                      RamFlagRoutineStep::ABSOLUTE_FALSE,
                                      contract.return_token, instructions, yields);
            if (result.reason != RoutineExitReason::NORMAL) return result;
            token = result.next_token;
        }
        if (token == contract.return_token) {
            machine.begin(RamFlagRoutineStep::RETURN);
            const auto address = machine.reg(contract.stack_register);
            const auto return_pc = machine.read(address, 4U);
            machine.set_reg(contract.stack_register, address + 4U);
            machine.complete_return(return_pc);
            machine.finish(RamFlagRoutineStep::RETURN);
            ++instructions;
            continuation.active = false;
            continuation.next_token = contract.continuation_token;
            return {contract.continuation_token, RoutineExitReason::NORMAL,
                    instructions, 0U, yields};
        }
        throw std::runtime_error("RAM flag routine reached an invalid token " +
                                 std::to_string(token));
    }
}

} // namespace oasis::core
