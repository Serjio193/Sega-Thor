#include "tools/hybrid/generated/m68k_recompiled_00e682_part1.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {

M68k9D70StepResult step_m68k_00e682_part1(
    unsigned pc, const M68k9D70StepApi& api, unsigned& resume_pc,
    StepMetrics_00e682& metrics, bool& active, bool& interrupted) noexcept {
    switch (pc) {
    case 0x00E69AU: {
        const std::uint16_t opcode = 0x526EU;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        const auto src = 0x1U;
        const auto dst = static_cast<std::uint16_t>(api.read(api.context, (api.read_register(api.context, kRegA6) + 0x2EU) & kMask24, 2));
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write(api.context, (api.read_register(api.context, kRegA6) + 0x2EU) & kMask24, 2, (res & 0xFFFFU));
        update_add_ccr_w(api, dst, src, res);
        complete_step(api, 0x00E69EU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E69EU: {
        const std::uint16_t opcode = 0x5340U;
        prepare_step(api, pc, opcode);
        const auto src = 0x1U;
        const auto dst = (api.read_register(api.context, kRegD0) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) - static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegD0, (api.read_register(api.context, kRegD0) & 0xFFFF0000U) | ((res & 0xFFFFU) & 0xFFFFU));
        update_sub_ccr_w(api, dst, src, res);
        complete_step(api, 0x00E6A0U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E6A0U: {
        const std::uint16_t opcode = 0x1D40U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        api.write(api.context, (api.read_register(api.context, kRegA6) + 0x82U) & kMask24, 1, (api.read_register(api.context, kRegD0) & 0xFFU));
        update_logic_ccr_b(api, (api.read_register(api.context, kRegD0) & 0xFFU));
        complete_step(api, 0x00E6A4U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E6A4U: {
        const std::uint16_t opcode = 0x6702U;
        prepare_step(api, pc, opcode);
        const bool taken = (get_ccr(api) & 0x04U) != 0U;
        const unsigned target = taken ? 0x00E6A8U : 0x00E6A6U;
        const int adj = taken ? 0 : (api.byte_not_taken_adjustment ? api.byte_not_taken_adjustment(api.context) : 0);
        complete_step(api, target, opcode, adj, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E6A6U: {
        const std::uint16_t opcode = 0x588FU;
        prepare_step(api, pc, opcode);
        const auto src = 0x4U;
        const auto dst = api.read_register(api.context, kRegA7);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegA7, (res & 0xFFFFFFFFU));
        complete_step(api, 0x00E6A8U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E6A8U: {
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
