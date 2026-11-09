#include "tools/hybrid/generated/m68k_recompiled_00e682_part0.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {

M68k9D70StepResult step_m68k_00e682_part0(
    unsigned pc, const M68k9D70StepApi& api, unsigned& resume_pc,
    StepMetrics_00e682& metrics, bool& active, bool& interrupted) noexcept {
    switch (pc) {
    case 0x00E682U: {
        const std::uint16_t opcode = 0x08AEU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        (void)api.fetch16(api.context);
        const auto val = static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x36U) & kMask24, 1));
        const bool is_zero = (val & (1U << (6U & 7U))) == 0U;
        update_btst_ccr(api, is_zero);
        const auto new_val = static_cast<std::uint8_t>(val & ~(1U << (6U & 7U)));
        api.write(api.context, (api.read_register(api.context, kRegA6) + 0x36U) & kMask24, 1, new_val);
        complete_step(api, 0x00E688U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E688U: {
        const std::uint16_t opcode = 0x102EU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD0, (api.read_register(api.context, kRegD0) & 0xFFFFFF00U) | (static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x9DU) & kMask24, 1)) & 0xFFU));
        update_logic_ccr_b(api, static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x9DU) & kMask24, 1)));
        complete_step(api, 0x00E68CU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E68CU: {
        const std::uint16_t opcode = 0x6600U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) == 0U;
        const unsigned target = taken ? 0x00E6AAU : 0x00E690U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E690U: {
        const std::uint16_t opcode = 0x102EU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write_register(api.context, kRegD0, (api.read_register(api.context, kRegD0) & 0xFFFFFF00U) | (static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x82U) & kMask24, 1)) & 0xFFU));
        update_logic_ccr_b(api, static_cast<std::uint8_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x82U) & kMask24, 1)));
        complete_step(api, 0x00E694U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E694U: {
        const std::uint16_t opcode = 0x6600U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const bool taken = (get_ccr(api) & 0x04U) == 0U;
        const unsigned target = taken ? 0x00E69AU : 0x00E698U;
        const int adj = taken ? 0 : (api.word_not_taken_adjustment ? api.word_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E698U: {
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
