#include "tools/hybrid/m68k_00ad76_commit_step.hpp"
#include "tools/hybrid/m68k_00ad76_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00ad76;

M68k9D70StepResult step_00ad76_commit(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& active,
                                      M68k00AD76StepMetrics& metrics) noexcept {
    std::uint16_t opcode = 0U;
    switch (pc) {
    case 0x00AFFAU: { // suba.l #$00FF13CC, a1
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1);
        api.write_register(api.context, kRegA1, a1 - 0x00FF13CCU);
        if (!complete_step(api, 0x00B000U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00B000U: { // move.w a1, ($FF188C).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto a1 = api.read_register(api.context, kRegA1) & 0xFFFFU;
        write_be(api, 0x00FF188CU, 2, a1);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (a1 == 0U) new_ccr |= 0x04U;
        if ((a1 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00B006U, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00B006U: { // move.w d2, ($FF188A).l
        if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
        (void)api.fetch16(api.context); (void)api.fetch16(api.context);
        const auto d2 = api.read_register(api.context, kRegD2) & 0xFFFFU;
        write_be(api, 0x00FF188AU, 2, d2);
        std::uint16_t new_ccr = ccr(api) & 0x10U;
        if (d2 == 0U) new_ccr |= 0x04U;
        if ((d2 & 0x8000U) != 0U) new_ccr |= 0x08U;
        set_ccr(api, new_ccr);
        if (!complete_step(api, 0x00B00CU, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00B00CU: { // movem.l (sp)+, d7
        metrics.stepped_instructions++;
        return step_movem_pop_d7(api, pc, kExitRtsPc, resume_pc);
    }
    case kExitRtsPc: { // rts (0x00B010)
        metrics.stepped_instructions++;
        metrics.completed_calls++;
        active = false;
        return step_rts(api, pc, resume_pc);
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid
