#pragma once

#include "core/table_copy_routine.hpp"

#include <cstdint>

namespace oasis::core {

enum class RamFlagRoutineStep {
    FIRST_FLAG_ADDRESS,
    FIRST_FLAG_SET,
    SECOND_FLAG_ADDRESS,
    SECOND_FLAG_SET,
    OUTPUT_ADDRESS,
    OUTPUT_FALSE_0,
    OUTPUT_FALSE_1,
    OUTPUT_FALSE_2,
    ABSOLUTE_FALSE,
    RETURN,
};

// Tokens and addresses are supplied by the ROM-specific adapter. The core
// intentionally has no ROM address, opcode, decoder or host-runtime concept.
struct RamFlagRoutineContract {
    std::uint32_t entry_token{};
    std::uint32_t first_flag_token{};
    std::uint32_t second_flag_token{};
    std::uint32_t second_flag_set_token{};
    std::uint32_t output_address_token{};
    std::uint32_t output_false_0_token{};
    std::uint32_t output_false_1_token{};
    std::uint32_t output_false_2_token{};
    std::uint32_t absolute_false_token{};
    std::uint32_t return_token{};
    std::uint32_t continuation_token{};
    std::uint32_t first_flag_address{};
    std::uint32_t second_flag_address{};
    std::uint32_t absolute_output_address{};
    unsigned status_register{17U};
    unsigned output_register{8U};
    unsigned base_register{13U};
    unsigned flag_register{14U};
    unsigned stack_register{15U};
};

class RamFlagRoutineMachine {
public:
    virtual ~RamFlagRoutineMachine() = default;
    virtual std::uint32_t reg(unsigned index) const = 0;
    virtual void set_reg(unsigned index, std::uint32_t value) = 0;
    virtual std::uint32_t read(std::uint32_t address, unsigned width) = 0;
    virtual void write(std::uint32_t address, unsigned width, std::uint32_t value) = 0;
    virtual void begin(RamFlagRoutineStep step) = 0;
    virtual void finish(RamFlagRoutineStep step) = 0;
    virtual RoutineBoundaryReason boundary() = 0;
    virtual void complete_return(std::uint32_t return_pc) = 0;
};

[[nodiscard]] RoutineExit execute_ram_flag_routine(
    RamFlagRoutineMachine& machine, const RamFlagRoutineContract& contract,
    std::uint32_t entry_token, RoutineContinuation& continuation);

} // namespace oasis::core
