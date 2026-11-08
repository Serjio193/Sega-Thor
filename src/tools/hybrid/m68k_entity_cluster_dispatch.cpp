#include "tools/hybrid/m68k_entity_cluster_dispatch.hpp"

#include <array>

namespace oasis::hybrid::entity_cluster {
namespace {

constexpr std::array<std::int16_t, 13> kCanonicalJumpOffsets{{
    0x012E, 0x0136, 0x01D4, 0x01FA, 0x023C, 0x025C, 0x02FE, 0x03AC, // 0..14
    0x000A, 0x0058, 0x0072, 0x00C0, 0x0100                           // 16..24
}};

void update_ccr_move_w(const GuestExecutionContext& ctx, std::uint16_t val) noexcept {
    std::uint8_t c = ctx.ccr() & 0x10U; // keep X flag unchanged
    if (val == 0U) {
        c |= 0x04U; // Z flag
    } else if ((val & 0x8000U) != 0U) {
        c |= 0x08U; // N flag
    }
    // V and C are cleared
    ctx.set_ccr(c);
}

} // namespace

JumpTableResolution calculate_jump_target(
    const GuestExecutionContext& ctx, std::uint16_t d0_val) noexcept {
    JumpTableResolution res{};
    res.d0_index = d0_val;
    if ((d0_val & 1U) != 0U || d0_val > kMaxValidTableIndex) {
        res.is_valid = false;
        return res;
    }
    const unsigned slot_index = d0_val >> 1U;
    res.table_address = kJumpTableBasePc + d0_val;
    std::int16_t rom_offset{};
    if (ctx.read_s16(res.table_address, rom_offset)) {
        res.offset_word = rom_offset;
    } else {
        res.offset_word = kCanonicalJumpOffsets[slot_index];
    }
    res.target_pc = (res.table_address + static_cast<std::int32_t>(res.offset_word)) & 0xFFFFFFU;
    res.is_valid = true;
    return res;
}

M68k9D70StepResult step_cluster_dispatcher(
    const GuestExecutionContext& ctx, unsigned pc, ClusterTransfer& transfer) noexcept {
    if (pc != kClusterDispatcherPc) {
        return M68k9D70StepResult::NotHandled;
    }

    // Step 1: 0x02575C - MOVE.W 4(A6), D0 (opcode 0x302E, ext 0x0004)
    std::uint16_t op1{};
    if (!ctx.begin_instruction(0x02575CU, op1)) return M68k9D70StepResult::ContractFault;
    const unsigned a6 = ctx.a(6);
    std::uint16_t state_w{};
    if (!ctx.read_u16(a6 + 4U, state_w)) return M68k9D70StepResult::ContractFault;
    const std::uint32_t old_d0 = ctx.d(0);
    ctx.set_d(0, (old_d0 & 0xFFFF0000U) | state_w);
    update_ccr_move_w(ctx, state_w);
    ctx.finish_instruction(0x025760U, op1, 0);

    // Step 2: 0x025760 - LEA 6(PC, D0.W), A0 (opcode 0x41FB, ext 0x0006)
    std::uint16_t op2{};
    if (!ctx.begin_instruction(0x025760U, op2)) return M68k9D70StepResult::ContractFault;
    const auto resolution = calculate_jump_target(ctx, state_w);
    if (!resolution.is_valid) {
        transfer.kind = TransferKind::ContractFault;
        return M68k9D70StepResult::ContractFault;
    }
    ctx.set_a(0, resolution.table_address);
    ctx.finish_instruction(0x025764U, op2, 0);

    // Step 3: 0x025764 - ADDA.W (A0), A0 (opcode 0xD0D0)
    std::uint16_t op3{};
    if (!ctx.begin_instruction(0x025764U, op3)) return M68k9D70StepResult::ContractFault;
    ctx.set_a(0, resolution.target_pc);
    ctx.finish_instruction(0x025766U, op3, 0);

    // Step 4: 0x025766 - JMP (A0) (opcode 0x4ED0)
    std::uint16_t op4{};
    if (!ctx.begin_instruction(0x025766U, op4)) return M68k9D70StepResult::ContractFault;
    ctx.finish_instruction(resolution.target_pc, op4, 0);

    transfer.kind = TransferKind::ContinueInsideCluster;
    transfer.next_pc = resolution.target_pc;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid::entity_cluster
