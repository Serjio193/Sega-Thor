#include "tools/hybrid/generated/m68k_recompiled_015ae2_part0.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {

M68k9D70StepResult step_m68k_015ae2_part0(
    unsigned pc, const M68k9D70StepApi& api, unsigned& resume_pc,
    StepMetrics_015ae2& metrics, bool& active, bool& interrupted) noexcept {
    switch (pc) {
    case 0x015AE2U: {
        const std::uint16_t opcode = 0x102EU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD0, (api.read_register(api.context, kRegD0) & 0xFFFFFF00U) | (static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x66U) & kMask24, 1)) & 0xFFU));
        update_logic_ccr_b(api, static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x66U) & kMask24, 1)));
        complete_step(api, 0x015AE6U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x015AE6U: {
        const std::uint16_t opcode = 0x6700U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) != 0U;
        const unsigned target = taken ? 0x016294U : 0x015AEAU;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x015AEAU: {
        const std::uint16_t opcode = 0x5300U;
        prepare_step(api, pc, opcode);
        const auto src = 0x1U;
        const auto dst = (api.read_register(api.context, kRegD0) & 0xFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegD0, (api.read_register(api.context, kRegD0) & 0xFFFFFF00U) | ((res & 0xFFU) & 0xFFU));
        update_sub_ccr_b(api, dst, src, res);
        complete_step(api, 0x015AECU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x015AECU: {
        const std::uint16_t opcode = 0x6700U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) != 0U;
        const unsigned target = taken ? 0x015AFEU : 0x015AF0U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x015AF0U: {
        const std::uint16_t opcode = 0x5300U;
        prepare_step(api, pc, opcode);
        const auto src = 0x1U;
        const auto dst = (api.read_register(api.context, kRegD0) & 0xFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegD0, (api.read_register(api.context, kRegD0) & 0xFFFFFF00U) | ((res & 0xFFU) & 0xFFU));
        update_sub_ccr_b(api, dst, src, res);
        complete_step(api, 0x015AF2U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid::generated
