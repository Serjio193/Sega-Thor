#include "tools/hybrid/m68k_02575c_root_step.hpp"
#include "tools/hybrid/m68k_02575c_step_ops.hpp"

namespace oasis::hybrid::step_02575c {
namespace {

using step_623ac::read_be;
using step_623ac::in_work_ram;
using step_623ac::prepare_step;
using step_623ac::complete_step;
using step_623ac::branch_to;
using step_623ac::ccr;
using step_623ac::set_ccr;

constexpr unsigned kRootEntryPc = 0x0254EAU;
constexpr unsigned kRootChildCallPc = 0x0256B8U;
constexpr unsigned kRootChildRetPc = 0x0256BEU;
constexpr unsigned kChild00E682Pc = 0x00E682U;

} // namespace

M68k9D70StepResult step_root(
    unsigned pc, const M68k9D70StepApi& api, bool enabled,
    bool& root_active, bool& root_child_active,
    unsigned& root_resume_pc, unsigned& saved_root_child_sp,
    M68k02575CStepMetrics& metrics) noexcept {
    pc &= kMask24;

    if (root_child_active) {
        const auto sp = api.read_register(api.context, 15U) & kMask24;
        if (pc == root_resume_pc && sp == saved_root_child_sp) {
            root_child_active = false;
            metrics.root_child_returns++;
            metrics.rom_child_return_completions++;
        } else if (sp >= saved_root_child_sp + 4U) {
            root_child_active = false;
            root_active = false;
            return M68k9D70StepResult::NotHandled;
        } else {
            return M68k9D70StepResult::NotHandled;
        }
    }

    if (!root_active) {
        if (pc != kRootEntryPc) return M68k9D70StepResult::NotHandled;
        metrics.root_entries++;
        if (!enabled) {
            metrics.root_rom_entries++;
            return M68k9D70StepResult::NotHandled;
        }
        const auto a6 = api.read_register(api.context, 14U) & kMask24;
        if (!in_work_ram(a6, 0x100U)) {
            metrics.root_rom_entries++;
            return M68k9D70StepResult::NotHandled;
        }
        unsigned mode_byte = 0U;
        if (!read_be(api, a6 + 102U, 1, mode_byte)) {
            metrics.contract_faults++;
            return M68k9D70StepResult::ContractFault;
        }
        if (mode_byte != 0U) {
            // Modes 1..5: decline fail-closed to original ROM without mutations
            metrics.root_rom_entries++;
            return M68k9D70StepResult::NotHandled;
        }
        unsigned b157 = 0U, b130 = 0U, w134 = 0U;
        if (!read_be(api, a6 + 157U, 1, b157) || !read_be(api, a6 + 130U, 1, b130) || !read_be(api, a6 + 134U, 2, w134)) {
            metrics.contract_faults++;
            return M68k9D70StepResult::ContractFault;
        }
        if (b157 != 0U || b130 > 1U || w134 != 0U) {
            // Motion child stack-pop or non-zero 134 path: decline fail-closed to ROM
            metrics.root_rom_entries++;
            return M68k9D70StepResult::NotHandled;
        }
        root_active = true;
        root_resume_pc = kRootEntryPc;
        metrics.root_native_entries++;
    }

    if (root_active && pc != root_resume_pc) return M68k9D70StepResult::NotHandled;

    std::uint16_t opcode = 0U;
    const auto a6 = api.read_register(api.context, 14U) & kMask24;

    switch (pc) {
    case 0x0254EAU: { // move.b 102(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned byte_val = 0U;
        if (!read_be(api, a6 + 102U, 1, byte_val)) return M68k9D70StepResult::ContractFault;
        const auto old_d0 = api.read_register(api.context, 0U);
        api.write_register(api.context, 0U, (old_d0 & 0xFFFFFF00U) | (byte_val & 0xFFU));
        const auto old_ccr = ccr(api);
        set_ccr(api, (old_ccr & 0x10U) | ((byte_val == 0U) ? 0x04U : 0U));
        if (!complete_step(api, 0x0254EEU, opcode, 0, root_resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.root_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0254EEU: { // beq.w $0256B8
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) != 0U) {
            if (!complete_step(api, kRootChildCallPc, opcode, 0, root_resume_pc)) return M68k9D70StepResult::ContractFault;
        } else {
            root_active = false;
            return M68k9D70StepResult::NotHandled;
        }
        metrics.stepped_instructions++; metrics.root_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0256B8U: { // jsr ($00E682).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const unsigned old_sp = api.read_register(api.context, 15U) & kMask24;
        const unsigned sp = (old_sp - 4U) & kMask24;
        if (!in_work_ram(sp, 4U)) return M68k9D70StepResult::ContractFault;
        api.write(api.context, sp, 4U, kRootChildRetPc);
        api.write_register(api.context, 15U, sp);
        unsigned target_word = 0U;
        if (!read_be(api, kChild00E682Pc, 2U, target_word)) return M68k9D70StepResult::ContractFault;
        api.set_return_state(api.context, kChild00E682Pc, kChild00E682Pc + 2U, target_word);
        api.finish_instruction(api.context, opcode);
        root_child_active = true;
        saved_root_child_sp = old_sp;
        root_resume_pc = kRootChildRetPc;
        metrics.stepped_instructions++; metrics.root_steps++; metrics.root_child_calls++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0256BEU: { // move.w 134(a6), d0
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        unsigned val = 0U;
        if (!read_be(api, a6 + 134U, 2, val)) return M68k9D70StepResult::ContractFault;
        const auto old_d0 = api.read_register(api.context, 0U);
        api.write_register(api.context, 0U, (old_d0 & 0xFFFF0000U) | (val & 0xFFFFU));
        update_ccr_move_w(api, static_cast<std::uint16_t>(val));
        if (!complete_step(api, 0x0256C2U, opcode, 0, root_resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++; metrics.root_steps++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0256C2U: { // beq.w $02575C
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        if ((ccr(api) & 0x04U) != 0U) {
            root_active = false; // Root sequence complete, transition to dispatcher
            if (!complete_step(api, kStartPc, opcode, 0, root_resume_pc)) return M68k9D70StepResult::ContractFault;
            metrics.stepped_instructions++; metrics.root_steps++;
            return M68k9D70StepResult::InstructionStepped;
        } else {
            root_active = false;
            metrics.contract_faults++;
            return M68k9D70StepResult::ContractFault;
        }
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid::step_02575c
