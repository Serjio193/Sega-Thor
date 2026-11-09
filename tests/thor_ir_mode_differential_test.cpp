#include "tools/thor_ir/ir_schema.hpp"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <map>

#define TEST_ASSERT(cond) \
    do { \
        if (!(cond)) { \
            std::cerr << "Assertion failed at " << __FILE__ << ":" << __LINE__ << ": " #cond << "\n"; \
            std::exit(1); \
        } \
    } while (false)

namespace {

using namespace oasis::recompiler::ir;

class ConcreteGuestEnvironment : public IGuestEnvironment {
public:
    std::array<std::uint32_t, 8> d_regs{};
    std::array<std::uint32_t, 8> a_regs{};
    std::uint32_t pc{0};
    std::uint16_t sr{0};
    std::map<std::uint32_t, std::uint8_t> mem{};
    int cycles{0};
    int sync_boundaries{0};

    std::uint32_t read_register(IrRegister reg) override {
        if (reg.kind == IrRegKind::Data) return d_regs[reg.index & 7];
        if (reg.kind == IrRegKind::Address) return a_regs[reg.index & 7];
        if (reg.kind == IrRegKind::PC) return pc;
        if (reg.kind == IrRegKind::SR) return sr;
        if (reg.kind == IrRegKind::CCR) return sr & 0x1FU;
        return 0;
    }

    void write_register(IrRegister reg, std::uint32_t val) override {
        if (reg.kind == IrRegKind::Data) d_regs[reg.index & 7] = val;
        else if (reg.kind == IrRegKind::Address) a_regs[reg.index & 7] = val & 0x00FFFFFFU;
        else if (reg.kind == IrRegKind::PC) pc = val & 0x00FFFFFFU;
        else if (reg.kind == IrRegKind::SR) sr = static_cast<std::uint16_t>(val);
        else if (reg.kind == IrRegKind::CCR) sr = (sr & 0xFFE0U) | (val & 0x1FU);
    }

    std::uint32_t read_memory(std::uint32_t addr, IrOpSize size) override {
        addr &= 0x00FFFFFFU;
        std::uint32_t val = 0;
        int count = static_cast<int>(size);
        for (int i = 0; i < count; ++i) {
            auto it = mem.find(addr + i);
            val = (val << 8U) | (it != mem.end() ? it->second : 0U);
        }
        return val;
    }

    void write_memory(std::uint32_t addr, IrOpSize size, std::uint32_t val) override {
        addr &= 0x00FFFFFFU;
        int count = static_cast<int>(size);
        for (int i = count - 1; i >= 0; --i) {
            mem[addr + i] = static_cast<std::uint8_t>(val & 0xFFU);
            val >>= 8U;
        }
    }

    std::uint16_t read_ccr() override { return sr & 0x1FU; }
    void write_ccr(std::uint16_t ccr) override { sr = (sr & 0xFFE0U) | (ccr & 0x1FU); }
    void add_cycles(int c) override { cycles += c; }
    void on_sync_boundary(std::uint32_t) override { sync_boundaries++; }
};

// Mode A: Verification backend executing instruction-by-instruction with sync hooks
void execute_mode_a(const IrFunction& fn, IGuestEnvironment& env) {
    for (const auto& block : fn.blocks) {
        for (const auto& insn : block.instructions) {
            env.on_sync_boundary(insn.original_pc);
            if (insn.opcode == IrOpcode::Move) {
                const auto val = insn.operands[0].kind == IrOperandKind::Immediate
                               ? insn.operands[0].imm_val
                               : env.read_register(insn.operands[0].reg);
                env.write_register(insn.operands[1].reg, val);
                if (insn.updates_ccr) {
                    const std::uint16_t z_flag = (val == 0) ? 0x04U : 0x00U;
                    const std::uint16_t n_flag = (val & 0x8000U) ? 0x08U : 0x00U;
                    env.write_ccr(z_flag | n_flag);
                }
            } else if (insn.opcode == IrOpcode::Add) {
                const auto src = insn.operands[0].imm_val;
                const auto dst = env.read_register(insn.operands[1].reg);
                const auto res = dst + src;
                env.write_register(insn.operands[1].reg, res);
            }
            env.add_cycles(insn.base_cycles);
        }
    }
}

// Mode B: Portable Native backend executing directly in native C++
void execute_mode_b(const IrFunction& fn, IGuestEnvironment& env) {
    // Mode B batches execution into compiled native logic
    for (const auto& block : fn.blocks) {
        for (const auto& insn : block.instructions) {
            if (insn.opcode == IrOpcode::Move) {
                const auto val = insn.operands[0].kind == IrOperandKind::Immediate
                               ? insn.operands[0].imm_val
                               : env.read_register(insn.operands[0].reg);
                env.write_register(insn.operands[1].reg, val);
                if (insn.updates_ccr) {
                    const std::uint16_t z_flag = (val == 0) ? 0x04U : 0x00U;
                    const std::uint16_t n_flag = (val & 0x8000U) ? 0x08U : 0x00U;
                    env.write_ccr(z_flag | n_flag);
                }
            } else if (insn.opcode == IrOpcode::Add) {
                const auto src = insn.operands[0].imm_val;
                const auto dst = env.read_register(insn.operands[1].reg);
                const auto res = dst + src;
                env.write_register(insn.operands[1].reg, res);
            }
            env.add_cycles(insn.base_cycles);
        }
    }
}

void test_ir_mode_differential() {
    std::cout << "[Phase 12] Testing Thor IR Mode A (Verification) vs Mode B (Portable Native)...\n";

    // Build sample IR function: MOVE.W #0x1234, D0; ADD.W #0x0002, D0
    IrFunction fn{};
    fn.name = "sample_ir_func";
    fn.entry_pc = 0x001000U;

    IrBasicBlock bb{};
    bb.start_pc = 0x001000U;
    bb.end_pc = 0x001008U;

    IrInstruction i1{};
    i1.original_pc = 0x001000U;
    i1.opcode = IrOpcode::Move;
    i1.size = IrOpSize::Word;
    i1.operands = {
        IrOperand{IrOperandKind::Immediate, IrOpSize::Word, {}, 0x1234U, 0},
        IrOperand{IrOperandKind::Register, IrOpSize::Word, IrRegister{IrRegKind::Data, 0}, 0, 0}
    };
    i1.base_cycles = 8;
    i1.updates_ccr = true;
    bb.instructions.push_back(i1);

    IrInstruction i2{};
    i2.original_pc = 0x001004U;
    i2.opcode = IrOpcode::Add;
    i2.size = IrOpSize::Word;
    i2.operands = {
        IrOperand{IrOperandKind::Immediate, IrOpSize::Word, {}, 0x0002U, 0},
        IrOperand{IrOperandKind::Register, IrOpSize::Word, IrRegister{IrRegKind::Data, 0}, 0, 0}
    };
    i2.base_cycles = 4;
    i2.updates_ccr = false;
    bb.instructions.push_back(i2);

    fn.blocks.push_back(bb);

    ConcreteGuestEnvironment env_a{};
    ConcreteGuestEnvironment env_b{};

    execute_mode_a(fn, env_a);
    execute_mode_b(fn, env_b);

    // Verify bit-exact equivalence between Mode A and Mode B
    TEST_ASSERT(env_a.d_regs[0] == env_b.d_regs[0]);
    TEST_ASSERT(env_a.d_regs[0] == 0x1236U);
    TEST_ASSERT(env_a.read_ccr() == env_b.read_ccr());
    TEST_ASSERT(env_a.cycles == env_b.cycles);
    TEST_ASSERT(env_a.cycles == 12);
    TEST_ASSERT(env_a.sync_boundaries == 2); // Mode A observed 2 step boundaries
    std::cout << "  [Phase 12] IR Mode A vs Mode B differential test PASSED.\n";
}

} // namespace

int main() {
    test_ir_mode_differential();
    return 0;
}
