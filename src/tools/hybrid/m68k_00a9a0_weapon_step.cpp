#include "tools/hybrid/m68k_00a9a0_weapon_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00a9a0;

M68k9D70StepResult step_00a9a0_weapon(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& child_call_active,
                                      unsigned& saved_child_sp,
                                      M68k00A9A0StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AD42U: { // bsr.w $00B34C (weapon piece hook)
        metrics.stepped_instructions++;
        metrics.weapon_00b34c_calls++;
        return step_bsr_w(api, pc, 0x00B34CU, 0x00AD46U, resume_pc, child_call_active, saved_child_sp);
    }
    case 0x00AD46U: { // bra.w $00AB6A
        metrics.stepped_instructions++;
        return step_bra_w(api, pc, 0x00AB6AU, resume_pc);
    }
    case 0x00AD4AU: { // movea.l 4(sp), a6
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context);
        const auto sp = api.read_register(api.context, kRegSP) & kMask24;
        unsigned mem_val = 0U;
        if (!read_be(api, sp + 4U, 4, mem_val)) return M68k9D70StepResult::ContractFault;
        api.write_register(api.context, kRegA6, mem_val);
        if (!complete_step(api, 0x00AD4EU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00AD4EU: { // bsr.w $00B3B2 (weapon shadow hook)
        metrics.stepped_instructions++;
        metrics.weapon_00b3b2_calls++;
        return step_bsr_w(api, pc, 0x00B3B2U, 0x00AD52U, resume_pc, child_call_active, saved_child_sp);
    }
    case 0x00AD52U: { // bra.w $00AC84
        metrics.stepped_instructions++;
        return step_bra_w(api, pc, 0x00AC84U, resume_pc);
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}
} // namespace oasis::hybrid
