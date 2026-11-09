#include "tools/hybrid/generated/m68k_recompiled_00f0d2_part1.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {

M68k9D70StepResult step_m68k_00f0d2_part1(
    unsigned pc, const M68k9D70StepApi& api, unsigned& resume_pc,
    StepMetrics_00f0d2& metrics, bool& active, bool& interrupted) noexcept {
    switch (pc) {
    case 0x00F0DEU: {
        const std::uint16_t opcode = 0x2003U;
        prepare_step(api, pc, opcode);
        api.write_register(api.context, kRegD0, api.read_register(api.context, kRegD3));
        update_logic_ccr_l(api, api.read_register(api.context, kRegD3));
        complete_step(api, 0x00F0E0U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00F0E0U: {
        const std::uint16_t opcode = 0xD084U;
        prepare_step(api, pc, opcode);
        const auto src = api.read_register(api.context, kRegD4);
        const auto dst = api.read_register(api.context, kRegD0);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegD0, (res & 0xFFFFFFFFU));
        update_add_ccr_l(api, dst, src, res);
        complete_step(api, 0x00F0E2U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00F0E2U: {
        const std::uint16_t opcode = 0x6100U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const unsigned sp = (api.read_register(api.context, kRegA7) - 4U) & kMask24;
        api.write_register(api.context, kRegA7, sp);
        api.write(api.context, sp, 4, 0x00F0E6U);
        unsigned dummy_resume = 0;
        complete_step(api, 0x00E6BAU, opcode, 0, dummy_resume);
        resume_pc = 0x00F0E6U;
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00F0E6U: {
        const std::uint16_t opcode = 0x4CDFU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        auto sp = api.read_register(api.context, kRegA7) & kMask24;
        api.write_register(api.context, kRegD3, api.read(api.context, (sp + 0U) & kMask24, 4));
        api.write_register(api.context, kRegD4, api.read(api.context, (sp + 4U) & kMask24, 4));
        api.write_register(api.context, kRegA7, (sp + 8U) & kMask24);
        if (api.add_cycles) api.add_cycles(api.context, 112);
        complete_step(api, 0x00F0EAU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00F0EAU: {
        const auto res = step_rts(api, pc, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
        }
        return res;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid::generated
