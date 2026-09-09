#pragma once

#include <cstdint>

namespace oasis::core {

enum class MechanicalOperation { MEMORY_CLEAR, MEMORY_COPY };
enum class PrimitiveStep { BODY, DBF };
enum class DbfResult { TAKEN, FALLTHROUGH };

enum class PrimitiveBoundaryReason {
    CONTINUE_BLOCK,
    EVENT_BOUNDARY,
    INTERRUPT_BOUNDARY,
    TRACE_BOUNDARY,
    FALLBACK,
};

enum class PrimitiveExitReason {
    NORMAL_EXIT,
    EVENT_BOUNDARY,
    INTERRUPT_BOUNDARY,
    TRACE_BOUNDARY,
    FALLBACK,
};

// Tokens are opaque to core. The hybrid adapter supplies guest PCs; standalone
// users may use any unrelated labels.
struct MechanicalLoopContract {
    MechanicalOperation operation{MechanicalOperation::MEMORY_CLEAR};
    std::uint32_t body_token{};
    std::uint32_t loop_token{};
    std::uint32_t continuation_token{};
    unsigned counter_register{};
    unsigned address_register{};
    unsigned source_register{};
    unsigned destination_register{};
    unsigned width{};
};

struct PrimitiveContinuation {
    bool active{};
    std::uint32_t next_token{};
};

struct PrimitiveExit {
    std::uint32_t next_token{};
    PrimitiveExitReason reason{PrimitiveExitReason::NORMAL_EXIT};
    unsigned guest_instructions{};
    unsigned iterations{};
    unsigned boundary_yields{};
};

class MechanicalMachine {
public:
    virtual ~MechanicalMachine() = default;
    virtual std::uint32_t reg(unsigned index) const = 0;
    virtual void set_reg(unsigned index, std::uint32_t value) = 0;
    virtual std::uint32_t read(std::uint32_t address, unsigned width) = 0;
    virtual void write(std::uint32_t address, unsigned width, std::uint32_t value) = 0;
    virtual void begin(PrimitiveStep step) = 0;
    virtual void fetch_dbf_displacement() = 0;
    virtual void finish(PrimitiveStep step, DbfResult result) = 0;
    virtual PrimitiveBoundaryReason boundary() const = 0;
};

[[nodiscard]] PrimitiveExit execute_mechanical_loop(
    MechanicalMachine& machine, const MechanicalLoopContract& contract,
    std::uint32_t entry_token, PrimitiveContinuation& continuation);

[[nodiscard]] PrimitiveExit execute_memory_clear(
    MechanicalMachine& machine, const MechanicalLoopContract& contract,
    std::uint32_t entry_token, PrimitiveContinuation& continuation);

} // namespace oasis::core
