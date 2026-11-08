#include "tools/hybrid/m68k_entity_cluster_state0.hpp"

namespace oasis::hybrid::entity_cluster {
namespace {

void update_ccr_subq_w_1(const GuestExecutionContext& ctx, std::uint16_t orig, std::uint16_t res) noexcept {
    const std::uint8_t dm = (orig >> 15U) & 1U;
    const std::uint8_t rm = (res >> 15U) & 1U;
    std::uint8_t ccr = 0U;
    if (res == 0U) ccr |= 0x04U;          // Z
    if (rm != 0U)  ccr |= 0x08U;          // N
    if (dm == 1U && rm == 0U) ccr |= 0x02U; // V (overflow 0x8000 - 1 = 0x7FFF)
    if (orig == 0U) ccr |= 0x11U;         // C and X (borrow 0 - 1 = 0xFFFF)
    ctx.set_ccr(ccr);
}

bool condition_ble(std::uint8_t ccr) noexcept {
    const bool z = (ccr & 0x04U) != 0U;
    const bool n = (ccr & 0x08U) != 0U;
    const bool v = (ccr & 0x02U) != 0U;
    return z || (n != v);
}

} // namespace

M68k9D70StepResult step_cluster_state0(
    const GuestExecutionContext& ctx, unsigned pc, ClusterTransfer& transfer) noexcept {
    if (pc != kState0EntryPc) {
        return M68k9D70StepResult::NotHandled;
    }

    // Step 1: 0x025896 - SUBQ.W #1, 2(A6) (opcode 0x536E, ext 0x0002)
    std::uint16_t op1{};
    if (!ctx.begin_instruction(0x025896U, op1)) return M68k9D70StepResult::ContractFault;
    const unsigned a6 = ctx.a(6);
    std::uint16_t timer{};
    if (!ctx.read_u16(a6 + 2U, timer)) return M68k9D70StepResult::ContractFault;
    const std::uint16_t next_timer = static_cast<std::uint16_t>(timer - 1U);
    if (!ctx.write_u16(a6 + 2U, next_timer)) return M68k9D70StepResult::ContractFault;
    update_ccr_subq_w_1(ctx, timer, next_timer);
    ctx.finish_instruction(0x02589AU, op1, 0);

    // Step 2: 0x02589A - BLE.W $0255D8 (opcode 0x6F00, disp 0xFD3C = -0x02C4)
    std::uint16_t op2{};
    if (!ctx.begin_instruction(0x02589AU, op2)) return M68k9D70StepResult::ContractFault;
    const bool taken = condition_ble(ctx.ccr());
    if (taken) {
        ctx.finish_instruction(kCommonJoinPc, op2, 0);
        transfer.kind = TransferKind::LoopbackJoin;
        transfer.next_pc = kCommonJoinPc;
        return M68k9D70StepResult::InstructionStepped;
    }

    // Not taken: word branch not taken adjustment
    const int not_taken_adjust = ctx.api().word_not_taken_adjustment ?
        ctx.api().word_not_taken_adjustment(ctx.api().context) : 0;
    ctx.finish_instruction(0x02589EU, op2, not_taken_adjust);

    // Step 3: 0x02589E - RTS (opcode 0x4E75)
    std::uint16_t op3{};
    if (!ctx.begin_instruction(0x02589EU, op3)) return M68k9D70StepResult::ContractFault;
    const unsigned sp = ctx.a(7);
    std::uint32_t ret_pc{};
    if (!ctx.read_u32(sp, ret_pc)) return M68k9D70StepResult::ContractFault;
    ctx.set_a(7, sp + 4U);
    ctx.finish_instruction(ret_pc, op3, 0);

    transfer.kind = TransferKind::TerminalReturn;
    transfer.next_pc = ret_pc;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid::entity_cluster
