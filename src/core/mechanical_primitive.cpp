#include "core/mechanical_primitive.hpp"

#include <stdexcept>

namespace oasis::core {
namespace {
constexpr unsigned kProgramCounter = 16U;
constexpr unsigned kStatusRegister = 17U;
constexpr unsigned kCcrMask = 0x0FU;

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

void validate(const MechanicalLoopContract& contract) {
    require(contract.body_token != contract.loop_token &&
                contract.body_token != contract.continuation_token &&
                contract.loop_token != contract.continuation_token,
            "primitive contract token collision");
    require(contract.counter_register < 8U, "primitive counter is not a data register");
    if (contract.operation == MechanicalOperation::MEMORY_CLEAR) {
        require(contract.address_register < 8U &&
                    (contract.width == 1U || contract.width == 2U) &&
                    contract.source_register == 0U && contract.destination_register == 0U,
                "unsupported memory-clear form");
    } else if (contract.operation == MechanicalOperation::MEMORY_COPY) {
        require(contract.source_register < 8U && contract.destination_register < 8U &&
                    contract.width == 1U,
                "unsupported memory-copy form");
    } else {
        throw std::runtime_error("unsupported mechanical operation");
    }
}

void validate_entry(const MechanicalLoopContract& contract,
                    const PrimitiveContinuation& continuation,
                    std::uint32_t entry_token) {
    if (!continuation.active) {
        require(entry_token == contract.body_token,
                "primitive initial entry must start at the body");
        return;
    }
    require(entry_token == continuation.next_token &&
                (entry_token == contract.body_token || entry_token == contract.loop_token),
            "primitive continuation entry mismatch");
}

PrimitiveExitReason exit_reason(PrimitiveBoundaryReason reason) {
    switch (reason) {
    case PrimitiveBoundaryReason::EVENT_BOUNDARY:
        return PrimitiveExitReason::EVENT_BOUNDARY;
    case PrimitiveBoundaryReason::INTERRUPT_BOUNDARY:
        return PrimitiveExitReason::INTERRUPT_BOUNDARY;
    case PrimitiveBoundaryReason::TRACE_BOUNDARY:
        return PrimitiveExitReason::TRACE_BOUNDARY;
    case PrimitiveBoundaryReason::FALLBACK:
        return PrimitiveExitReason::FALLBACK;
    case PrimitiveBoundaryReason::CONTINUE_BLOCK:
        break;
    }
    return PrimitiveExitReason::NORMAL_EXIT;
}

PrimitiveExit finish_exit(PrimitiveContinuation& continuation,
                           const MechanicalLoopContract& contract,
                           std::uint32_t next_token, PrimitiveExitReason reason,
                           unsigned instructions, unsigned iterations,
                           unsigned yields) {
    continuation.next_token = next_token;
    continuation.active = next_token == contract.loop_token || next_token == contract.body_token;
    return {next_token, reason, instructions, iterations, yields};
}

void logical_flags(MechanicalMachine& machine, std::uint32_t value, unsigned width) {
    const auto mask = width == 1U ? 0xFFU : width == 2U ? 0xFFFFU : 0U;
    const auto sign = width == 1U ? 0x80U : width == 2U ? 0x8000U : 0U;
    require(mask != 0U, "unsupported primitive flag width");
    auto status = machine.reg(kStatusRegister);
    status = (status & ~kCcrMask) | (((value & mask) == 0U) ? 0x04U : 0U) |
             (((value & sign) != 0U) ? 0x08U : 0U);
    machine.set_reg(kStatusRegister, status);
}
}

PrimitiveExit execute_mechanical_loop(MechanicalMachine& machine,
                                      const MechanicalLoopContract& contract,
                                      std::uint32_t entry_token,
                                      PrimitiveContinuation& continuation) {
    validate(contract);
    validate_entry(contract, continuation, entry_token);
    auto token = entry_token;
    PrimitiveExit exit{entry_token, PrimitiveExitReason::NORMAL_EXIT, 0U, 0U, 0U};
    for (;;) {
        if (token == contract.body_token) {
            machine.begin(PrimitiveStep::BODY);
            if (contract.operation == MechanicalOperation::MEMORY_CLEAR) {
                const auto address = machine.reg(8U + contract.address_register);
                require(contract.width != 2U || (address & 1U) == 0U,
                        "odd word address outside primitive contract");
                machine.write(address, contract.width, 0U);
                machine.set_reg(8U + contract.address_register, address + contract.width);
                logical_flags(machine, 0U, contract.width);
            } else {
                const auto source = 8U + contract.source_register;
                const auto destination = 8U + contract.destination_register;
                const auto value = machine.read(machine.reg(source), contract.width);
                machine.set_reg(source, machine.reg(source) + contract.width);
                machine.write(machine.reg(destination), contract.width, value);
                machine.set_reg(destination, machine.reg(destination) + contract.width);
                logical_flags(machine, value, contract.width);
            }
            machine.set_reg(kProgramCounter, contract.loop_token);
            machine.finish(PrimitiveStep::BODY, DbfResult::FALLTHROUGH);
            ++exit.guest_instructions;
            ++exit.iterations;
            const auto reason = machine.boundary();
            if (reason != PrimitiveBoundaryReason::CONTINUE_BLOCK) {
                ++exit.boundary_yields;
                return finish_exit(continuation, contract, contract.loop_token,
                                   exit_reason(reason), exit.guest_instructions,
                                   exit.iterations, exit.boundary_yields);
            }
            token = contract.loop_token;
        }

        machine.begin(PrimitiveStep::DBF);
        const auto value = machine.reg(contract.counter_register);
        const auto result = (value - 1U) & 0xFFFFU;
        machine.set_reg(contract.counter_register, (value & 0xFFFF0000U) | result);
        const auto taken = result != 0xFFFFU;
        if (taken) machine.fetch_dbf_displacement();
        machine.set_reg(kProgramCounter, taken ? contract.body_token : contract.continuation_token);
        machine.finish(PrimitiveStep::DBF, taken ? DbfResult::TAKEN : DbfResult::FALLTHROUGH);
        ++exit.guest_instructions;
        const auto reason = machine.boundary();
        if (reason != PrimitiveBoundaryReason::CONTINUE_BLOCK) {
            ++exit.boundary_yields;
            return finish_exit(continuation, contract,
                               taken ? contract.body_token : contract.continuation_token,
                               exit_reason(reason), exit.guest_instructions,
                               exit.iterations, exit.boundary_yields);
        }
        if (!taken)
            return finish_exit(continuation, contract, contract.continuation_token,
                               PrimitiveExitReason::NORMAL_EXIT,
                               exit.guest_instructions, exit.iterations,
                               exit.boundary_yields);
        token = contract.body_token;
    }
}

PrimitiveExit execute_memory_clear(MechanicalMachine& machine,
                                   const MechanicalLoopContract& contract,
                                   std::uint32_t entry_token,
                                   PrimitiveContinuation& continuation) {
    require(contract.operation == MechanicalOperation::MEMORY_CLEAR,
            "memory-clear executor received another operation");
    return execute_mechanical_loop(machine, contract, entry_token, continuation);
}

} // namespace oasis::core
