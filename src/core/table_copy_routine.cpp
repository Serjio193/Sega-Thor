#include "core/table_copy_routine.hpp"

#include <stdexcept>

namespace oasis::core {
namespace {
constexpr unsigned kStatusRegister = 17U;
constexpr unsigned kCcrMask = 0x0FU;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void validate(const TableCopyRoutineContract& contract) {
    const auto tokens = {
        contract.entry_token, contract.clear_token, contract.offset_token,
        contract.destination_token, contract.add_destination_token,
        contract.count_token, contract.copy_token, contract.dbf_token,
        contract.restore_token, contract.return_token, contract.continuation_token};
    for (auto left = tokens.begin(); left != tokens.end(); ++left)
        for (auto right = left + 1; right != tokens.end(); ++right)
            require(*left != *right, "table-copy routine token collision");
    require(contract.source_register >= 8U && contract.source_register < 16U &&
                contract.counter_register < 8U &&
                contract.destination_register >= 8U && contract.destination_register < 16U &&
                contract.stack_register >= 8U && contract.stack_register < 16U,
            "table-copy routine register contract is invalid");
}

void validate_entry(const TableCopyRoutineContract& contract,
                    const RoutineContinuation& continuation,
                    std::uint32_t entry_token) {
    if (!continuation.active) {
        require(entry_token == contract.entry_token,
                "table-copy routine initial entry must start at entry token");
        return;
    }
    require(entry_token == continuation.next_token && entry_token != contract.entry_token,
            "table-copy routine continuation entry mismatch");
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

void logical_flags(TableCopyRoutineMachine& machine, std::uint32_t value, unsigned width) {
    const auto mask = width == 1U ? 0xFFU : 0xFFFFU;
    const auto sign = width == 1U ? 0x80U : 0x8000U;
    auto status = machine.reg(kStatusRegister);
    status = (status & ~kCcrMask) | (((value & mask) == 0U) ? 0x04U : 0U) |
             (((value & sign) != 0U) ? 0x08U : 0U);
    machine.set_reg(kStatusRegister, status);
}

std::uint32_t next_after(const TableCopyRoutineContract& contract,
                         TableCopyStep step, bool dbf_taken) {
    switch (step) {
    case TableCopyStep::SAVE_FRAME: return contract.clear_token;
    case TableCopyStep::CLEAR_COUNTER: return contract.offset_token;
    case TableCopyStep::READ_OFFSET: return contract.destination_token;
    case TableCopyStep::SET_DESTINATION: return contract.add_destination_token;
    case TableCopyStep::ADD_DESTINATION: return contract.count_token;
    case TableCopyStep::READ_COUNT: return contract.copy_token;
    case TableCopyStep::COPY_WORD: return contract.dbf_token;
    case TableCopyStep::DBF: return dbf_taken ? contract.copy_token : contract.restore_token;
    case TableCopyStep::RESTORE_FRAME: return contract.return_token;
    case TableCopyStep::RETURN: return contract.continuation_token;
    }
    return contract.continuation_token;
}

RoutineExit finish_step(TableCopyRoutineMachine& machine,
                        const TableCopyRoutineContract& contract,
                        RoutineContinuation& continuation, TableCopyStep step,
                        bool dbf_taken, unsigned& instructions, unsigned& iterations,
                        unsigned& yields) {
    machine.finish(step);
    ++instructions;
    const auto next = next_after(contract, step, dbf_taken);
    const auto reason = machine.boundary();
    if (reason != RoutineBoundaryReason::CONTINUE) {
        continuation.active = true;
        continuation.next_token = next;
        ++yields;
        return {next, exit_reason(reason), instructions, iterations, yields};
    }
    continuation.active = true;
    continuation.next_token = next;
    return {next, RoutineExitReason::NORMAL, instructions, iterations, yields};
}
}

RoutineExit execute_table_copy_routine(TableCopyRoutineMachine& machine,
                                       const TableCopyRoutineContract& contract,
                                       std::uint32_t entry_token,
                                       RoutineContinuation& continuation) {
    validate(contract);
    validate_entry(contract, continuation, entry_token);
    auto token = entry_token;
    unsigned instructions = 0U;
    unsigned iterations = 0U;
    unsigned yields = 0U;
    const auto source = contract.source_register;
    const auto counter = contract.counter_register;
    const auto destination = contract.destination_register;
    const auto stack = contract.stack_register;

    for (;;) {
        if (token == contract.entry_token) {
            machine.begin(TableCopyStep::SAVE_FRAME);
            auto address = machine.reg(stack);
            const auto saved_destination = machine.reg(destination);
            const auto saved_counter = machine.reg(counter);
            address -= 2U; machine.write(address, 2U, saved_destination);
            address -= 2U; machine.write(address, 2U, saved_destination >> 16U);
            address -= 2U; machine.write(address, 2U, saved_counter);
            address -= 2U; machine.write(address, 2U, saved_counter >> 16U);
            machine.set_reg(stack, address);
            const auto result = finish_step(machine, contract, continuation,
                                            TableCopyStep::SAVE_FRAME, false,
                                            instructions, iterations, yields);
            if (result.reason != RoutineExitReason::NORMAL || continuation.active &&
                result.next_token != contract.clear_token) return result;
            token = result.next_token;
        }
        if (token == contract.clear_token) {
            machine.begin(TableCopyStep::CLEAR_COUNTER);
            machine.set_reg(counter, 0U);
            logical_flags(machine, 0U, 2U);
            const auto result = finish_step(machine, contract, continuation,
                                            TableCopyStep::CLEAR_COUNTER, false,
                                            instructions, iterations, yields);
            if (result.reason != RoutineExitReason::NORMAL || result.next_token != contract.offset_token)
                return result;
            token = result.next_token;
        }
        if (token == contract.offset_token) {
            machine.begin(TableCopyStep::READ_OFFSET);
            const auto value = machine.read(machine.reg(source), 1U);
            machine.set_reg(source, machine.reg(source) + 1U);
            machine.set_reg(counter, (machine.reg(counter) & 0xFFFFFF00U) | (value & 0xFFU));
            logical_flags(machine, value, 1U);
            const auto result = finish_step(machine, contract, continuation,
                                            TableCopyStep::READ_OFFSET, false,
                                            instructions, iterations, yields);
            if (result.reason != RoutineExitReason::NORMAL || result.next_token != contract.destination_token)
                return result;
            token = result.next_token;
        }
        if (token == contract.destination_token) {
            machine.begin(TableCopyStep::SET_DESTINATION);
            machine.set_reg(destination, contract.destination_base);
            const auto result = finish_step(machine, contract, continuation,
                                            TableCopyStep::SET_DESTINATION, false,
                                            instructions, iterations, yields);
            if (result.reason != RoutineExitReason::NORMAL || result.next_token != contract.add_destination_token)
                return result;
            token = result.next_token;
        }
        if (token == contract.add_destination_token) {
            machine.begin(TableCopyStep::ADD_DESTINATION);
            const auto offset = static_cast<std::int16_t>(machine.reg(counter) & 0xFFFFU);
            machine.set_reg(destination, machine.reg(destination) + static_cast<std::int32_t>(offset));
            const auto result = finish_step(machine, contract, continuation,
                                            TableCopyStep::ADD_DESTINATION, false,
                                            instructions, iterations, yields);
            if (result.reason != RoutineExitReason::NORMAL || result.next_token != contract.count_token)
                return result;
            token = result.next_token;
        }
        if (token == contract.count_token) {
            machine.begin(TableCopyStep::READ_COUNT);
            const auto value = machine.read(machine.reg(source), 1U);
            machine.set_reg(source, machine.reg(source) + 1U);
            machine.set_reg(counter, (machine.reg(counter) & 0xFFFFFF00U) | (value & 0xFFU));
            logical_flags(machine, value, 1U);
            const auto result = finish_step(machine, contract, continuation,
                                            TableCopyStep::READ_COUNT, false,
                                            instructions, iterations, yields);
            if (result.reason != RoutineExitReason::NORMAL || result.next_token != contract.copy_token)
                return result;
            token = result.next_token;
        }
        if (token == contract.copy_token) {
            machine.begin(TableCopyStep::COPY_WORD);
            const auto value = machine.read(machine.reg(source), 2U);
            machine.set_reg(source, machine.reg(source) + 2U);
            machine.write(machine.reg(destination), 2U, value);
            machine.set_reg(destination, machine.reg(destination) + 2U);
            logical_flags(machine, value, 2U);
            ++iterations;
            const auto result = finish_step(machine, contract, continuation,
                                            TableCopyStep::COPY_WORD, false,
                                            instructions, iterations, yields);
            if (result.reason != RoutineExitReason::NORMAL || result.next_token != contract.dbf_token)
                return result;
            token = result.next_token;
        }
        if (token == contract.dbf_token) {
            machine.begin(TableCopyStep::DBF);
            const auto value = machine.reg(counter);
            const auto low = (value - 1U) & 0xFFFFU;
            machine.set_reg(counter, (value & 0xFFFF0000U) | low);
            const auto taken = low != 0xFFFFU;
            const auto result = finish_step(machine, contract, continuation,
                                            TableCopyStep::DBF, taken,
                                            instructions, iterations, yields);
            if (result.reason != RoutineExitReason::NORMAL ||
                result.next_token == contract.restore_token) {
                if (result.reason == RoutineExitReason::NORMAL && result.next_token == contract.restore_token)
                    token = result.next_token;
                else return result;
            } else {
                token = result.next_token;
            }
        }
        if (token == contract.copy_token) continue;
        if (token == contract.restore_token) {
            machine.begin(TableCopyStep::RESTORE_FRAME);
            const auto address = machine.reg(stack);
            const auto counter_high = machine.read(address, 2U);
            const auto counter_low = machine.read(address + 2U, 2U);
            const auto destination_high = machine.read(address + 4U, 2U);
            const auto destination_low = machine.read(address + 6U, 2U);
            machine.set_reg(counter, (counter_high << 16U) | counter_low);
            machine.set_reg(destination, (destination_high << 16U) | destination_low);
            machine.set_reg(stack, address + 8U);
            const auto result = finish_step(machine, contract, continuation,
                                            TableCopyStep::RESTORE_FRAME, false,
                                            instructions, iterations, yields);
            if (result.reason != RoutineExitReason::NORMAL || result.next_token != contract.return_token)
                return result;
            token = result.next_token;
        }
        if (token == contract.return_token) {
            machine.begin(TableCopyStep::RETURN);
            const auto address = machine.reg(stack);
            const auto return_pc = machine.read(address, 4U);
            machine.set_reg(stack, address + 4U);
            machine.complete_return(return_pc);
            continuation.active = false;
            continuation.next_token = contract.continuation_token;
            machine.finish(TableCopyStep::RETURN);
            ++instructions;
            return {contract.continuation_token, RoutineExitReason::NORMAL,
                    instructions, iterations, yields};
        }
        require(false, "table-copy routine reached an invalid token");
    }
}

} // namespace oasis::core
