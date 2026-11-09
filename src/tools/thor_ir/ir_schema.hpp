#ifndef OASIS_TOOLS_THOR_IR_IR_SCHEMA_HPP_
#define OASIS_TOOLS_THOR_IR_IR_SCHEMA_HPP_

#include <cstdint>
#include <string>
#include <vector>
#include <memory>
#include <optional>

namespace oasis::recompiler::ir {

enum class IrOpSize : std::uint8_t {
    Byte = 1,
    Word = 2,
    Long = 4
};

enum class IrRegKind : std::uint8_t {
    Data,
    Address,
    PC,
    SR,
    CCR
};

struct IrRegister {
    IrRegKind kind{IrRegKind::Data};
    std::uint8_t index{0};
};

enum class IrOperandKind : std::uint8_t {
    Register,
    Immediate,
    MemoryDirect,
    MemoryIndirect,
    MemoryDisplacement,
    MemoryPredecrement,
    MemoryPostincrement
};

struct IrOperand {
    IrOperandKind kind{IrOperandKind::Register};
    IrOpSize size{IrOpSize::Word};
    IrRegister reg{};
    std::uint32_t imm_val{0};
    std::int32_t displacement{0};
};

enum class IrCondition : std::uint8_t {
    Always,
    Equal,
    NotEqual,
    GreaterEqual,
    LessThan,
    GreaterThan,
    LessEqual,
    CarryClear,
    CarrySet,
    Plus,
    Minus
};

enum class IrOpcode : std::uint8_t {
    Nop,
    Move,
    Add,
    Sub,
    Cmp,
    And,
    Or,
    Eor,
    Lsl,
    Lsr,
    Asr,
    Btst,
    Bset,
    Bclr,
    Branch,
    Jump,
    Call,
    Return,
    SyncBoundary
};

struct IrInstruction {
    std::uint32_t original_pc{0};
    IrOpcode opcode{IrOpcode::Nop};
    IrOpSize size{IrOpSize::Word};
    std::vector<IrOperand> operands{};
    IrCondition condition{IrCondition::Always};
    std::uint32_t target_pc{0};
    int base_cycles{4};
    int variable_cycles{0};
    bool updates_ccr{true};
    bool is_sync_boundary{false};
};

struct IrBasicBlock {
    std::uint32_t start_pc{0};
    std::uint32_t end_pc{0};
    std::vector<IrInstruction> instructions{};
    std::vector<std::uint32_t> successors{};
};

struct IrFunction {
    std::string name{};
    std::uint32_t entry_pc{0};
    std::vector<IrBasicBlock> blocks{};
};

class IGuestEnvironment {
public:
    virtual ~IGuestEnvironment() = default;
    virtual std::uint32_t read_register(IrRegister reg) = 0;
    virtual void write_register(IrRegister reg, std::uint32_t val) = 0;
    virtual std::uint32_t read_memory(std::uint32_t addr, IrOpSize size) = 0;
    virtual void write_memory(std::uint32_t addr, IrOpSize size, std::uint32_t val) = 0;
    virtual std::uint16_t read_ccr() = 0;
    virtual void write_ccr(std::uint16_t ccr) = 0;
    virtual void add_cycles(int cycles) = 0;
    virtual void on_sync_boundary(std::uint32_t pc) = 0;
};

} // namespace oasis::recompiler::ir

#endif // OASIS_TOOLS_THOR_IR_IR_SCHEMA_HPP_
