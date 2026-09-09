#pragma once

#include <cstdint>

namespace oasis::core {

enum class TableCopyStep {
    SAVE_FRAME,
    CLEAR_COUNTER,
    READ_OFFSET,
    SET_DESTINATION,
    ADD_DESTINATION,
    READ_COUNT,
    COPY_WORD,
    DBF,
    RESTORE_FRAME,
    RETURN,
};

enum class RoutineBoundaryReason {
    CONTINUE,
    EVENT,
    INTERRUPT,
    TRACE,
    FALLBACK,
};

enum class RoutineExitReason {
    NORMAL,
    EVENT,
    INTERRUPT,
    TRACE,
    FALLBACK,
};

// All tokens are supplied by an adapter. Core does not know ROM PCs.
struct TableCopyRoutineContract {
    std::uint32_t entry_token{};
    std::uint32_t clear_token{};
    std::uint32_t offset_token{};
    std::uint32_t destination_token{};
    std::uint32_t add_destination_token{};
    std::uint32_t count_token{};
    std::uint32_t copy_token{};
    std::uint32_t dbf_token{};
    std::uint32_t restore_token{};
    std::uint32_t return_token{};
    std::uint32_t continuation_token{};
    std::uint32_t destination_base{};
    unsigned source_register{14U};
    unsigned counter_register{7U};
    unsigned destination_register{11U};
    unsigned stack_register{15U};
};

struct RoutineContinuation {
    bool active{};
    std::uint32_t next_token{};
};

struct RoutineExit {
    std::uint32_t next_token{};
    RoutineExitReason reason{RoutineExitReason::NORMAL};
    unsigned guest_instructions{};
    unsigned copy_iterations{};
    unsigned boundary_yields{};
};

class TableCopyRoutineMachine {
public:
    virtual ~TableCopyRoutineMachine() = default;
    virtual std::uint32_t reg(unsigned index) const = 0;
    virtual void set_reg(unsigned index, std::uint32_t value) = 0;
    virtual std::uint32_t read(std::uint32_t address, unsigned width) = 0;
    virtual void write(std::uint32_t address, unsigned width, std::uint32_t value) = 0;
    virtual void begin(TableCopyStep step) = 0;
    virtual void finish(TableCopyStep step) = 0;
    virtual RoutineBoundaryReason boundary() = 0;
    virtual void complete_return(std::uint32_t return_pc) = 0;
};

[[nodiscard]] RoutineExit execute_table_copy_routine(
    TableCopyRoutineMachine& machine, const TableCopyRoutineContract& contract,
    std::uint32_t entry_token, RoutineContinuation& continuation);

} // namespace oasis::core
