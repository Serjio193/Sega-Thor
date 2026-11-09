#include "tools/hybrid/generated/m68k_recompiled_00c9b2_part0.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {

M68k9D70StepResult step_m68k_00c9b2_part0(
    unsigned pc, const M68k9D70StepApi& api, unsigned& resume_pc,
    StepMetrics_00c9b2& metrics, bool& active, bool& interrupted) noexcept {
    switch (pc) {
    case 0x00C9B2U: {
        const std::uint16_t opcode = 0x4A39U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, 0xFF17B9U, 1));
        update_logic_ccr_b(api, val);
        complete_step(api, 0x00C9B8U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9B8U: {
        const std::uint16_t opcode = 0x662EU;
        prepare_step(api, pc, opcode);
        const bool taken = (get_ccr(api) & 0x04U) == 0U;
        const unsigned target = taken ? 0x00C9E8U : 0x00C9BAU;
        const int adj = taken ? 0 : (api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9BAU: {
        const std::uint16_t opcode = 0x4A39U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, 0xFF1987U, 1));
        update_logic_ccr_b(api, val);
        complete_step(api, 0x00C9C0U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9C0U: {
        const std::uint16_t opcode = 0x6600U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) == 0U;
        const unsigned target = taken ? 0x00C9E8U : 0x00C9C4U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9C4U: {
        const std::uint16_t opcode = 0x4A39U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, 0xFF1985U, 1));
        update_logic_ccr_b(api, val);
        complete_step(api, 0x00C9CAU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9CAU: {
        const std::uint16_t opcode = 0x661CU;
        prepare_step(api, pc, opcode);
        const bool taken = (get_ccr(api) & 0x04U) == 0U;
        const unsigned target = taken ? 0x00C9E8U : 0x00C9CCU;
        const int adj = taken ? 0 : (api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9CCU: {
        const std::uint16_t opcode = 0x0839U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, 0xFF17B8U, 1));
        const bool is_zero = (val & (1U << (2U & 7U))) == 0U;
        update_btst_ccr(api, is_zero);
        complete_step(api, 0x00C9D4U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00C9D4U: {
        const std::uint16_t opcode = 0x6612U;
        prepare_step(api, pc, opcode);
        const bool taken = (get_ccr(api) & 0x04U) == 0U;
        const unsigned target = taken ? 0x00C9E8U : 0x00C9D6U;
        const int adj = taken ? 0 : (api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid::generated
