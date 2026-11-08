#include "tools/hybrid/m68k_entity_cluster_target10.hpp"

#include <cmath>

namespace oasis::hybrid::entity_cluster {
namespace {

void update_ccr_cmp_w(const GuestExecutionContext& ctx, std::uint16_t src, std::uint16_t dst) noexcept {
    const std::uint32_t res = (static_cast<std::uint32_t>(dst) - src) & 0xFFFFU;
    const std::uint8_t sm = (src >> 15U) & 1U;
    const std::uint8_t dm = (dst >> 15U) & 1U;
    const std::uint8_t rm = (res >> 15U) & 1U;
    std::uint8_t ccr = ctx.ccr() & 0x10U; // preserve X
    if (res == 0U) ccr |= 0x04U;          // Z
    if (rm != 0U)  ccr |= 0x08U;          // N
    if ((!sm && dm && !rm) || (sm && !dm && rm)) ccr |= 0x02U; // V
    if ((sm && !dm) || (rm && !dm) || (sm && rm)) ccr |= 0x01U; // C
    ctx.set_ccr(ccr);
}

void update_ccr_move_w(const GuestExecutionContext& ctx, std::uint16_t val) noexcept {
    std::uint8_t ccr = ctx.ccr() & 0x10U;
    if (val == 0U) ccr |= 0x04U;
    if ((val & 0x8000U) != 0U) ccr |= 0x08U;
    ctx.set_ccr(ccr);
}

bool condition_bne(std::uint8_t ccr) noexcept {
    return (ccr & 0x04U) == 0U; // Z == 0
}

bool condition_bgt(std::uint8_t ccr) noexcept {
    const bool z = (ccr & 0x04U) != 0U;
    const bool n = (ccr & 0x08U) != 0U;
    const bool v = (ccr & 0x02U) != 0U;
    return !z && (n == v);
}

M68k9D70StepResult step_target10_rts(const GuestExecutionContext& ctx, ClusterTransfer& transfer) noexcept {
    std::uint16_t op_rts{};
    if (!ctx.begin_instruction(kTarget10ExitPc, op_rts)) return M68k9D70StepResult::ContractFault;
    const unsigned sp = ctx.a(7);
    std::uint32_t ret_pc{};
    if (!ctx.read_u32(sp, ret_pc)) return M68k9D70StepResult::ContractFault;
    ctx.set_a(7, sp + 4U);
    ctx.finish_instruction(ret_pc, op_rts, 0);
    transfer.kind = TransferKind::TerminalReturn;
    transfer.next_pc = ret_pc;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace

M68k9D70StepResult step_cluster_target10(
    const GuestExecutionContext& ctx, unsigned pc, ClusterTransfer& transfer,
    bool execute_child_inline) noexcept {

    // Continuation from child return at 0x025822:
    if (pc == 0x025822U) {
        // Step 11: 0x025822 - CMPI.W #$0030, D0 (opcode 0x0C40, imm 0x0030)
        std::uint16_t op11{};
        if (!ctx.begin_instruction(0x025822U, op11)) return M68k9D70StepResult::ContractFault;
        const std::uint16_t d0_w = static_cast<std::uint16_t>(ctx.d(0));
        update_ccr_cmp_w(ctx, 0x0030U, d0_w);
        ctx.finish_instruction(0x025826U, op11, 0);

        // Step 12: 0x025826 - BGT.W $02583C (opcode 0x6E00, disp 0x0014)
        std::uint16_t op12{};
        if (!ctx.begin_instruction(0x025826U, op12)) return M68k9D70StepResult::ContractFault;
        if (condition_bgt(ctx.ccr())) {
            ctx.finish_instruction(kTarget10ExitPc, op12, 0);
            return step_target10_rts(ctx, transfer);
        }
        const int not_taken = ctx.api().word_not_taken_adjustment ?
            ctx.api().word_not_taken_adjustment(ctx.api().context) : 0;
        ctx.finish_instruction(0x02582AU, op12, not_taken);

        // Step 13: 0x02582A - BCLR.B #1, 54(A6)
        std::uint16_t op13{};
        if (!ctx.begin_instruction(0x02582AU, op13)) return M68k9D70StepResult::ContractFault;
        const unsigned a6 = ctx.a(6);
        std::uint8_t byte54{};
        if (!ctx.read_u8(a6 + 54U, byte54)) return M68k9D70StepResult::ContractFault;
        const bool old_bit1 = (byte54 & 0x02U) != 0U;
        if (!ctx.write_u8(a6 + 54U, static_cast<std::uint8_t>(byte54 & ~0x02U))) return M68k9D70StepResult::ContractFault;
        std::uint8_t c13 = ctx.ccr() & 0xFBU; // clear Z
        if (!old_bit1) c13 |= 0x04U;          // set Z if zero
        ctx.set_ccr(c13);
        ctx.finish_instruction(0x025830U, op13, 0);

        // Step 14: 0x025830 - BCLR.B #0, 55(A6)
        std::uint16_t op14{};
        if (!ctx.begin_instruction(0x025830U, op14)) return M68k9D70StepResult::ContractFault;
        std::uint8_t byte55{};
        if (!ctx.read_u8(a6 + 55U, byte55)) return M68k9D70StepResult::ContractFault;
        const bool old_bit0 = (byte55 & 0x01U) != 0U;
        if (!ctx.write_u8(a6 + 55U, static_cast<std::uint8_t>(byte55 & ~0x01U))) return M68k9D70StepResult::ContractFault;
        std::uint8_t c14 = ctx.ccr() & 0xFBU;
        if (!old_bit0) c14 |= 0x04U;
        ctx.set_ccr(c14);
        ctx.finish_instruction(0x025836U, op14, 0);

        // Step 15: 0x025836 - MOVE.W #$0016, 4(A6)
        std::uint16_t op15{};
        if (!ctx.begin_instruction(0x025836U, op15)) return M68k9D70StepResult::ContractFault;
        if (!ctx.write_u16(a6 + 4U, 0x0016U)) return M68k9D70StepResult::ContractFault;
        update_ccr_move_w(ctx, 0x0016U);
        ctx.finish_instruction(kTarget10ExitPc, op15, 0);

        // Step 16: RTS
        return step_target10_rts(ctx, transfer);
    }

    if (pc != kTarget10EntryPc) {
        return M68k9D70StepResult::NotHandled;
    }

    // Step 1: 0x0257EE - BTST.B #3, ($FF1A20).L (opcode 0x0839)
    std::uint16_t op1{};
    if (!ctx.begin_instruction(0x0257EEU, op1)) return M68k9D70StepResult::ContractFault;
    std::uint8_t ram1a20{};
    if (!ctx.read_u8(0xFF1A20U, ram1a20)) return M68k9D70StepResult::ContractFault;
    const bool bit3 = (ram1a20 & 0x08U) != 0U;
    std::uint8_t ccr1 = ctx.ccr() & 0xFBU;
    if (!bit3) ccr1 |= 0x04U;
    ctx.set_ccr(ccr1);
    ctx.finish_instruction(0x0257F6U, op1, 0);

    // Step 2: 0x0257F6 - BNE.W $02583C (opcode 0x6600, disp 0x0044)
    std::uint16_t op2{};
    if (!ctx.begin_instruction(0x0257F6U, op2)) return M68k9D70StepResult::ContractFault;
    if (condition_bne(ctx.ccr())) {
        ctx.finish_instruction(kTarget10ExitPc, op2, 0);
        return step_target10_rts(ctx, transfer);
    }
    const int not_taken2 = ctx.api().word_not_taken_adjustment ?
        ctx.api().word_not_taken_adjustment(ctx.api().context) : 0;
    ctx.finish_instruction(0x0257FAU, op2, not_taken2);

    // Step 3: 0x0257FA - MOVE.W ($FF19FC).L, D0 (opcode 0x3039)
    std::uint16_t op3{};
    if (!ctx.begin_instruction(0x0257FAU, op3)) return M68k9D70StepResult::ContractFault;
    std::uint16_t target_id{};
    if (!ctx.read_u16(0xFF19FCU, target_id)) return M68k9D70StepResult::ContractFault;
    ctx.set_d(0, (ctx.d(0) & 0xFFFF0000U) | target_id);
    update_ccr_move_w(ctx, target_id);
    ctx.finish_instruction(0x025800U, op3, 0);

    // Step 4: 0x025800 - CMP.W 20(A6), D0 (opcode 0xB06E, ext 0x0014)
    std::uint16_t op4{};
    if (!ctx.begin_instruction(0x025800U, op4)) return M68k9D70StepResult::ContractFault;
    const unsigned a6 = ctx.a(6);
    std::uint16_t entity_id{};
    if (!ctx.read_u16(a6 + 20U, entity_id)) return M68k9D70StepResult::ContractFault;
    update_ccr_cmp_w(ctx, entity_id, target_id);
    ctx.finish_instruction(0x025804U, op4, 0);

    // Step 5: 0x025804 - BNE.W $02583C
    std::uint16_t op5{};
    if (!ctx.begin_instruction(0x025804U, op5)) return M68k9D70StepResult::ContractFault;
    if (condition_bne(ctx.ccr())) {
        ctx.finish_instruction(kTarget10ExitPc, op5, 0);
        return step_target10_rts(ctx, transfer);
    }
    const int not_taken5 = ctx.api().word_not_taken_adjustment ?
        ctx.api().word_not_taken_adjustment(ctx.api().context) : 0;
    ctx.finish_instruction(0x025808U, op5, not_taken5);

    // Steps 6..9: Coordinates load
    std::uint16_t op6{}, op7{}, op8{}, op9{};
    if (!ctx.begin_instruction(0x025808U, op6)) return M68k9D70StepResult::ContractFault;
    std::uint16_t px{}, py{}, ex{}, ey{};
    if (!ctx.read_u16(0xFF19F0U, px)) return M68k9D70StepResult::ContractFault;
    ctx.set_d(1, (ctx.d(1) & 0xFFFF0000U) | px);
    update_ccr_move_w(ctx, px);
    ctx.finish_instruction(0x02580EU, op6, 0);

    if (!ctx.begin_instruction(0x02580EU, op7)) return M68k9D70StepResult::ContractFault;
    if (!ctx.read_u16(0xFF19F4U, py)) return M68k9D70StepResult::ContractFault;
    ctx.set_d(2, (ctx.d(2) & 0xFFFF0000U) | py);
    update_ccr_move_w(ctx, py);
    ctx.finish_instruction(0x025814U, op7, 0);

    if (!ctx.begin_instruction(0x025814U, op8)) return M68k9D70StepResult::ContractFault;
    if (!ctx.read_u16(a6 + 8U, ex)) return M68k9D70StepResult::ContractFault;
    ctx.set_d(3, (ctx.d(3) & 0xFFFF0000U) | ex);
    update_ccr_move_w(ctx, ex);
    ctx.finish_instruction(0x025818U, op8, 0);

    if (!ctx.begin_instruction(0x025818U, op9)) return M68k9D70StepResult::ContractFault;
    if (!ctx.read_u16(a6 + 12U, ey)) return M68k9D70StepResult::ContractFault;
    ctx.set_d(4, (ctx.d(4) & 0xFFFF0000U) | ey);
    update_ccr_move_w(ctx, ey);
    ctx.finish_instruction(0x02581CU, op9, 0);

    // Step 10: 0x02581C - JSR ($00F0D2).L
    std::uint16_t op10{};
    if (!ctx.begin_instruction(0x02581CU, op10)) return M68k9D70StepResult::ContractFault;

    if (execute_child_inline) {
        // Inline math equivalent of 0x00F0D2
        const std::int32_t dx = static_cast<std::int16_t>(px) - static_cast<std::int16_t>(ex);
        const std::int32_t dy = static_cast<std::int16_t>(py) - static_cast<std::int16_t>(ey);
        const std::uint32_t dist = static_cast<std::uint32_t>(std::sqrt(dx * dx + dy * dy));
        ctx.set_d(0, (ctx.d(0) & 0xFFFF0000U) | (dist & 0xFFFFU));
        ctx.finish_instruction(0x025822U, op10, 0);
        return step_cluster_target10(ctx, 0x025822U, transfer, false);
    }

    // Push return PC 0x025822 onto stack
    const unsigned sp = ctx.a(7) - 4U;
    if (!ctx.write_u32(sp, 0x025822U)) return M68k9D70StepResult::ContractFault;
    ctx.set_a(7, sp);
    ctx.finish_instruction(kChild00F0D2Pc, op10, 0);

    transfer.kind = TransferKind::ExternalRomChild;
    transfer.next_pc = kChild00F0D2Pc;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid::entity_cluster
