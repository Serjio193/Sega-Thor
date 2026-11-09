#include "tools/hybrid/generated/m68k_recompiled_016294_part1.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {

M68k9D70StepResult step_m68k_016294_part1(
    unsigned pc, const M68k9D70StepApi& api, unsigned& resume_pc,
    StepMetrics_016294& metrics, bool& active, bool& interrupted) noexcept {
    switch (pc) {
    case 0x0162B2U: {
        const std::uint16_t opcode = 0x0C40U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto src = 0x7BU;
        const auto dst = (api.read_register(api.context, kRegD0) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        update_cmp_ccr_w(api, dst, src, res);
        complete_step(api, 0x0162B6U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0162B6U: {
        const std::uint16_t opcode = 0x6500U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x01U) != 0U;
        const unsigned target = taken ? 0x016306U : 0x0162BAU;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0162BAU: {
        const std::uint16_t opcode = 0x0C40U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto src = 0x96U;
        const auto dst = (api.read_register(api.context, kRegD0) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        update_cmp_ccr_w(api, dst, src, res);
        complete_step(api, 0x0162BEU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0162BEU: {
        const std::uint16_t opcode = 0x6500U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x01U) != 0U;
        const unsigned target = taken ? 0x015AFCU : 0x0162C2U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0162C2U: {
        const std::uint16_t opcode = 0x0C40U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto src = 0xB7U;
        const auto dst = (api.read_register(api.context, kRegD0) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        update_cmp_ccr_w(api, dst, src, res);
        complete_step(api, 0x0162C6U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0162C6U: {
        const std::uint16_t opcode = 0x6500U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x01U) != 0U;
        const unsigned target = taken ? 0x01634AU : 0x0162CAU;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x0162CAU: {
        const std::uint16_t opcode = 0x0C40U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto src = 0xD3U;
        const auto dst = (api.read_register(api.context, kRegD0) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        update_cmp_ccr_w(api, dst, src, res);
        complete_step(api, 0x0162CEU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid::generated
