#include "tools/hybrid/m68k_008b8e_step_ops.hpp"

namespace oasis::hybrid::step_008b8e {
namespace {
constexpr unsigned kMask24 = 0xFFFFFFU;
} // namespace

M68k9D70StepResult step_bclr_imm_abs(const M68k9D70StepApi& api, unsigned pc, unsigned bit,
                                    unsigned addr, unsigned next_pc, unsigned& resume_pc) noexcept {
    std::uint16_t opcode{};
    if (!prepare_step(api, pc, opcode)) return M68k9D70StepResult::ContractFault;
    (void)api.fetch16(api.context); // imm word (0x0007)
    (void)api.fetch16(api.context); // addr hi (0x00FF)
    (void)api.fetch16(api.context); // addr lo (0x1864)
    const auto val = static_cast<std::uint8_t>(api.read(api.context, addr & kMask24, 1));
    const auto old_ccr = ccr(api);
    const bool bit_val = (val & (1U << (bit & 7U))) != 0;
    const auto new_ccr = (old_ccr & ~0x04U) | (bit_val ? 0U : 0x04U); // Z is 1 if bit was 0
    set_ccr(api, new_ccr);
    const auto new_val = static_cast<std::uint8_t>(val & ~(1U << (bit & 7U)));
    api.write(api.context, addr & kMask24, 1, new_val);
    if (!complete_step(api, next_pc, opcode, 0, resume_pc)) return M68k9D70StepResult::ContractFault;
    return M68k9D70StepResult::InstructionStepped;
}

} // namespace oasis::hybrid::step_008b8e
