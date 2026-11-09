#include "tools/hybrid/generated/m68k_recompiled_00e6ba_part1.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"

namespace oasis::hybrid::generated {

M68k9D70StepResult step_m68k_00e6ba_part1(
    unsigned pc, const M68k9D70StepApi& api, unsigned& resume_pc,
    StepMetrics_00e6ba& metrics, bool& active, bool& interrupted) noexcept {
    switch (pc) {
    case 0x00E6D2U: {
        const std::uint16_t opcode = 0x6000U;
        prepare_step(api, pc, opcode);
        (void)api.fetch16(api.context);
        complete_step(api, 0x00E6D8U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E6D6U: {
        const std::uint16_t opcode = 0x3400U;
        prepare_step(api, pc, opcode);
        api.write_register(api.context, kRegD2, (api.read_register(api.context, kRegD2) & 0xFFFF0000U) | ((api.read_register(api.context, kRegD0) & 0xFFFFU) & 0xFFFFU));
        update_logic_ccr_w(api, (api.read_register(api.context, kRegD0) & 0xFFFFU));
        complete_step(api, 0x00E6D8U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E6D8U: {
        const std::uint16_t opcode = 0x3202U;
        prepare_step(api, pc, opcode);
        api.write_register(api.context, kRegD1, (api.read_register(api.context, kRegD1) & 0xFFFF0000U) | ((api.read_register(api.context, kRegD2) & 0xFFFFU) & 0xFFFFU));
        update_logic_ccr_w(api, (api.read_register(api.context, kRegD2) & 0xFFFFU));
        complete_step(api, 0x00E6DAU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E6DAU: {
        const std::uint16_t opcode = 0x2400U;
        prepare_step(api, pc, opcode);
        api.write_register(api.context, kRegD2, api.read_register(api.context, kRegD0));
        update_logic_ccr_l(api, api.read_register(api.context, kRegD0));
        complete_step(api, 0x00E6DCU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E6DCU: {
        const std::uint16_t opcode = 0x84C1U;
        prepare_step(api, pc, opcode);
        const auto divisor = static_cast<std::uint16_t>((api.read_register(api.context, kRegD1) & 0xFFFFU));
        const auto dividend = api.read_register(api.context, kRegD2);
        if (divisor != 0) {
            const auto quot = dividend / divisor;
            const auto rem = dividend % divisor;
            api.write_register(api.context, kRegD2, ((rem & 0xFFFFU) << 16U) | (quot & 0xFFFFU));
            std::uint16_t ccr = 0;
            if ((quot & 0xFFFFU) == 0) ccr |= 0x04U;
            if (quot & 0x8000U) ccr |= 0x08U;
            if (quot > 0xFFFFU) ccr |= 0x02U;
            set_ccr(api, (get_ccr(api) & 0xFFE0U) | ccr);
        }
        complete_step(api, 0x00E6DEU, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    case 0x00E6DEU: {
        const std::uint16_t opcode = 0xD441U;
        prepare_step(api, pc, opcode);
        const auto src = (api.read_register(api.context, kRegD1) & 0xFFFFU);
        const auto dst = (api.read_register(api.context, kRegD2) & 0xFFFFU);
        const auto res = static_cast<std::uint32_t>(dst) + static_cast<std::uint32_t>(src);
        api.write_register(api.context, kRegD2, (api.read_register(api.context, kRegD2) & 0xFFFF0000U) | ((res & 0xFFFFU) & 0xFFFFU));
        update_add_ccr_w(api, dst, src, res);
        complete_step(api, 0x00E6E0U, opcode, 0, resume_pc);
        metrics.stepped_instructions++;
        return M68k9D70StepResult::InstructionStepped;
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid::generated
