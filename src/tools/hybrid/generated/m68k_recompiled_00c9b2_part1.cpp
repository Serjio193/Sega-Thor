#include "tools/hybrid/generated/m68k_recompiled_00c9b2_part1.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {

M68k9D70StepResult step_m68k_00c9b2_part1(
    unsigned pc, const M68k9D70StepApi& api, unsigned& resume_pc,
    StepMetrics_00c9b2& metrics, bool& active, bool& interrupted) noexcept {
    switch (pc) {
    case 0x00C9D6U: {
        const std::uint16_t opcode = 0x4A39U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, 0xFF1983U, 1));
        update_logic_ccr_b(api, val);
        complete_step(api, 0x00C9DCU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9DCU: {
        const std::uint16_t opcode = 0x660AU;
        prepare_step(api, pc, opcode);
        const bool taken = (get_ccr(api) & 0x04U) == 0U;
        const unsigned target = taken ? 0x00C9E8U : 0x00C9DEU;
        const int adj = taken ? 0 : (api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9DEU: {
        const std::uint16_t opcode = 0x4A39U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, 0xFF1982U, 1));
        update_logic_ccr_b(api, val);
        complete_step(api, 0x00C9E4U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9E4U: {
        const std::uint16_t opcode = 0x6602U;
        prepare_step(api, pc, opcode);
        const bool taken = (get_ccr(api) & 0x04U) == 0U;
        const unsigned target = taken ? 0x00C9E8U : 0x00C9E6U;
        const int adj = taken ? 0 : (api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9E6U: {
        const auto res = step_rts(api, pc, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
        }
        return res;
    }
    case 0x00C9E8U: {
        const std::uint16_t opcode = 0x588FU;
        prepare_step(api, pc, opcode);
        const auto src = 0x4U;
        const auto dst = api.read_register(api.context, kRegA7);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegA7, (res & 0xFFFFFFFFU));
        complete_step(api, 0x00C9EAU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9EAU: {
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
