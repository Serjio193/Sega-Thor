#include "tools/hybrid/m68k_entity_cluster_target11_12.hpp"

namespace oasis::hybrid::entity_cluster {
namespace {

void update_ccr_sub_l(const GuestExecutionContext& ctx, std::uint32_t src, std::uint32_t dst, std::uint32_t res) noexcept {
    const std::uint8_t sm = (src >> 31U) & 1U;
    const std::uint8_t dm = (dst >> 31U) & 1U;
    const std::uint8_t rm = (res >> 31U) & 1U;
    std::uint8_t ccr = 0U;
    if (res == 0U) ccr |= 0x04U;          // Z
    if (rm != 0U)  ccr |= 0x08U;          // N
    if ((!sm && dm && !rm) || (sm && !dm && rm)) ccr |= 0x02U; // V
    if ((sm && !dm) || (rm && !dm) || (sm && rm)) ccr |= 0x11U; // C and X
    ctx.set_ccr(ccr);
}

void update_ccr_move_l(const GuestExecutionContext& ctx, std::uint32_t val) noexcept {
    std::uint8_t ccr = ctx.ccr() & 0x10U;
    if (val == 0U) ccr |= 0x04U;
    if ((val & 0x80000000U) != 0U) ccr |= 0x08U;
    ctx.set_ccr(ccr);
}

M68k9D70StepResult step_rts_helper(const GuestExecutionContext& ctx, unsigned pc, ClusterTransfer& transfer) noexcept {
    std::uint16_t op{};
    if (!ctx.begin_instruction(pc, op)) return M68k9D70StepResult::ContractFault;
    const unsigned sp = ctx.a(7);
    std::uint32_t ret_pc{};
    if (!ctx.read_u32(sp, ret_pc)) return M68k9D70StepResult::ContractFault;
    ctx.set_a(7, sp + 4U);
    ctx.finish_instruction(ret_pc, op, 0);
    transfer.kind = TransferKind::TerminalReturn;
    transfer.next_pc = ret_pc;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace

M68k9D70StepResult step_cluster_target11(
    const GuestExecutionContext& ctx, unsigned pc, ClusterTransfer& transfer) noexcept {
    if (pc != kTarget11EntryPc) return M68k9D70StepResult::NotHandled;

    // Step 1: 0x02583E - MOVE.L 86(A6), D0 (opcode 0x202E, ext 0x0056)
    std::uint16_t op1{};
    if (!ctx.begin_instruction(0x02583EU, op1)) return M68k9D70StepResult::ContractFault;
    const unsigned a6 = ctx.a(6);
    std::uint32_t delta_vel{};
    if (!ctx.read_u32(a6 + 86U, delta_vel)) return M68k9D70StepResult::ContractFault;
    ctx.set_d(0, delta_vel);
    update_ccr_move_l(ctx, delta_vel);
    ctx.finish_instruction(0x025842U, op1, 0);

    // Step 2: 0x025842 - SUB.L D0, 16(A6) (opcode 0x91AE, ext 0x0010)
    std::uint16_t op2{};
    if (!ctx.begin_instruction(0x025842U, op2)) return M68k9D70StepResult::ContractFault;
    std::uint32_t height{};
    if (!ctx.read_u32(a6 + 16U, height)) return M68k9D70StepResult::ContractFault;
    const std::uint32_t next_height = height - delta_vel;
    if (!ctx.write_u32(a6 + 16U, next_height)) return M68k9D70StepResult::ContractFault;
    update_ccr_sub_l(ctx, delta_vel, height, next_height);
    ctx.finish_instruction(0x025846U, op2, 0);

    // Step 3: 0x025846 - BMI.W $025856 (opcode 0x6B00, disp 0x000E)
    std::uint16_t op3{};
    if (!ctx.begin_instruction(0x025846U, op3)) return M68k9D70StepResult::ContractFault;
    const bool is_negative = (ctx.ccr() & 0x08U) != 0U; // N flag
    if (is_negative) {
        ctx.finish_instruction(0x025856U, op3, 0);

        // Branch body at 0x025856:
        // CLR.L 16(A6)
        std::uint16_t op4{};
        if (!ctx.begin_instruction(0x025856U, op4)) return M68k9D70StepResult::ContractFault;
        if (!ctx.write_u32(a6 + 16U, 0U)) return M68k9D70StepResult::ContractFault;
        ctx.set_ccr((ctx.ccr() & 0x10U) | 0x04U); // Z=1
        ctx.finish_instruction(0x02585AU, op4, 0);

        // CLR.W 46(A6)
        std::uint16_t op5{};
        if (!ctx.begin_instruction(0x02585AU, op5)) return M68k9D70StepResult::ContractFault;
        if (!ctx.write_u16(a6 + 46U, 0U)) return M68k9D70StepResult::ContractFault;
        ctx.set_ccr((ctx.ccr() & 0x10U) | 0x04U);
        ctx.finish_instruction(0x02585EU, op5, 0);

        // MOVE.W #$0018, 4(A6)
        std::uint16_t op6{};
        if (!ctx.begin_instruction(0x02585EU, op6)) return M68k9D70StepResult::ContractFault;
        if (!ctx.write_u16(a6 + 4U, 0x0018U)) return M68k9D70StepResult::ContractFault;
        ctx.set_ccr(ctx.ccr() & 0x10U);
        ctx.finish_instruction(0x025864U, op6, 0);

        // MOVE.L #$00500342, D0
        std::uint16_t op7{};
        if (!ctx.begin_instruction(0x025864U, op7)) return M68k9D70StepResult::ContractFault;
        ctx.set_d(0, 0x00500342U);
        update_ccr_move_l(ctx, 0x00500342U);
        ctx.finish_instruction(0x02586AU, op7, 0);

        // BTST.B #1, 57(A6)
        std::uint16_t op8{};
        if (!ctx.begin_instruction(0x02586AU, op8)) return M68k9D70StepResult::ContractFault;
        std::uint8_t byte57{};
        if (!ctx.read_u8(a6 + 57U, byte57)) return M68k9D70StepResult::ContractFault;
        std::uint8_t c8 = ctx.ccr() & 0xFBU;
        if ((byte57 & 0x02U) == 0U) c8 |= 0x04U; // Z=1 if bit 1 is 0
        ctx.set_ccr(c8);
        ctx.finish_instruction(0x025870U, op8, 0);

        // BEQ.W $02587A (opcode 0x6700, disp 0x0008)
        std::uint16_t op9{};
        if (!ctx.begin_instruction(0x025870U, op9)) return M68k9D70StepResult::ContractFault;
        // JMP ($00E7E8).L
        ctx.finish_instruction(0x02587AU, op9, 0);
        std::uint16_t op10{};
        if (!ctx.begin_instruction(0x02587AU, op10)) return M68k9D70StepResult::ContractFault;
        ctx.finish_instruction(kTailJump00E7E8Pc, op10, 0);

        transfer.kind = TransferKind::ExternalRomChild;
        transfer.next_pc = kTailJump00E7E8Pc;
        return M68k9D70StepResult::InstructionStepped;
    }

    // Normal non-negative path:
    const int not_taken3 = ctx.api().word_not_taken_adjustment ?
        ctx.api().word_not_taken_adjustment(ctx.api().context) : 0;
    ctx.finish_instruction(0x02584AU, op3, not_taken3);

    // ADDI.L #$0000C000, D0
    std::uint16_t op4{};
    if (!ctx.begin_instruction(0x02584AU, op4)) return M68k9D70StepResult::ContractFault;
    const std::uint32_t add_res = delta_vel + 0x0000C000U;
    ctx.set_d(0, add_res);
    update_ccr_move_l(ctx, add_res);
    ctx.finish_instruction(0x025850U, op4, 0);

    // MOVE.L D0, 86(A6)
    std::uint16_t op5{};
    if (!ctx.begin_instruction(0x025850U, op5)) return M68k9D70StepResult::ContractFault;
    if (!ctx.write_u32(a6 + 86U, add_res)) return M68k9D70StepResult::ContractFault;
    update_ccr_move_l(ctx, add_res);
    ctx.finish_instruction(0x025854U, op5, 0);

    // RTS at 0x025854
    return step_rts_helper(ctx, 0x025854U, transfer);
}

M68k9D70StepResult step_cluster_target12(
    const GuestExecutionContext& ctx, unsigned pc, ClusterTransfer& transfer) noexcept {
    if (pc != kTarget12EntryPc) return M68k9D70StepResult::NotHandled;

    // Step 1: 0x025880 - BTST.B #1, 55(A6) (opcode 0x082E, ext 0x0001 0x0037)
    std::uint16_t op1{};
    if (!ctx.begin_instruction(0x025880U, op1)) return M68k9D70StepResult::ContractFault;
    const unsigned a6 = ctx.a(6);
    std::uint8_t byte55{};
    if (!ctx.read_u8(a6 + 55U, byte55)) return M68k9D70StepResult::ContractFault;
    std::uint8_t c1 = ctx.ccr() & 0xFBU;
    if ((byte55 & 0x02U) == 0U) c1 |= 0x04U; // Z=1 if bit 1 is 0
    ctx.set_ccr(c1);
    ctx.finish_instruction(0x025886U, op1, 0);

    // Step 2: 0x025886 - BNE.W $02588C (opcode 0x6600, disp 0x0004)
    std::uint16_t op2{};
    if (!ctx.begin_instruction(0x025886U, op2)) return M68k9D70StepResult::ContractFault;
    const bool is_stunned = (ctx.ccr() & 0x04U) == 0U; // Z==0 (bit 1 was set)
    if (is_stunned) {
        ctx.finish_instruction(0x02588CU, op2, 0);

        // Step 3: 0x02588C - ANDI.B #$F3, 56(A6)
        std::uint16_t op3{};
        if (!ctx.begin_instruction(0x02588CU, op3)) return M68k9D70StepResult::ContractFault;
        std::uint8_t byte56{};
        if (!ctx.read_u8(a6 + 56U, byte56)) return M68k9D70StepResult::ContractFault;
        const std::uint8_t next_b56 = static_cast<std::uint8_t>(byte56 & 0xF3U);
        if (!ctx.write_u8(a6 + 56U, next_b56)) return M68k9D70StepResult::ContractFault;
        std::uint8_t c3 = ctx.ccr() & 0x10U;
        if (next_b56 == 0U) c3 |= 0x04U;
        if ((next_b56 & 0x80U) != 0U) c3 |= 0x08U;
        ctx.set_ccr(c3);
        ctx.finish_instruction(0x025892U, op3, 0);

        // Step 4: 0x025892 - BRA.W $0255D8
        std::uint16_t op4{};
        if (!ctx.begin_instruction(0x025892U, op4)) return M68k9D70StepResult::ContractFault;
        ctx.finish_instruction(kCommonJoinPc, op4, 0);

        transfer.kind = TransferKind::LoopbackJoin;
        transfer.next_pc = kCommonJoinPc;
        return M68k9D70StepResult::InstructionStepped;
    }

    // Normal non-stunned path: RTS at 0x02588A
    const int not_taken = ctx.api().word_not_taken_adjustment ?
        ctx.api().word_not_taken_adjustment(ctx.api().context) : 0;
    ctx.finish_instruction(0x02588AU, op2, not_taken);
    return step_rts_helper(ctx, 0x02588AU, transfer);
}

} // namespace oasis::hybrid::entity_cluster
