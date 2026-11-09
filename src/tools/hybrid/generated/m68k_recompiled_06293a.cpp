#include "tools/hybrid/generated/m68k_recompiled_06293a.hpp"
#include "tools/hybrid/generated/m68k_recompiled_06293a_p0.hpp"
#include "tools/hybrid/generated/m68k_recompiled_06293a_p1.hpp"

namespace oasis::hybrid::generated {

M68k9D70StepResult m68k_recompiled_06293a(const M68k9D70StepApi& api, unsigned& resume_pc) noexcept {
    unsigned block_pc = 0x06293AU;
    unsigned step_count = 0;
    while (block_pc != 0U && step_count < 2000U) {
        ++step_count;
        const unsigned current_pc = block_pc;
        block_pc = m68k_recompiled_06293a_p0(api, current_pc, resume_pc);
        if (block_pc != 0U && block_pc != current_pc) continue;
        block_pc = m68k_recompiled_06293a_p1(api, current_pc, resume_pc);
        if (block_pc != 0U && block_pc != current_pc) continue;
        if (block_pc == current_pc) break; // Unhandled block
    }
    return M68k9D70StepResult::InstructionStepped;
}
} // namespace oasis::hybrid::generated
