#include "tools/hybrid/generated/m68k_recompiled_002512.hpp"
#include "tools/hybrid/generated/m68k_recompiled_002512_p0.hpp"
#include "tools/hybrid/generated/m68k_recompiled_002512_p1.hpp"
#include "tools/hybrid/generated/m68k_recompiled_002512_p2.hpp"
#include "tools/hybrid/generated/m68k_recompiled_002512_p3.hpp"
#include "tools/hybrid/generated/m68k_recompiled_002512_p4.hpp"

namespace oasis::hybrid::generated {

M68k9D70StepResult m68k_recompiled_002512(const M68k9D70StepApi& api, unsigned& resume_pc) noexcept {
    unsigned block_pc = 0x002512U;
    unsigned step_count = 0;
    while (block_pc != 0U && step_count < 2000U) {
        ++step_count;
        const unsigned current_pc = block_pc;
        block_pc = m68k_recompiled_002512_p0(api, current_pc, resume_pc);
        if (block_pc != 0U && block_pc != current_pc) continue;
        block_pc = m68k_recompiled_002512_p1(api, current_pc, resume_pc);
        if (block_pc != 0U && block_pc != current_pc) continue;
        block_pc = m68k_recompiled_002512_p2(api, current_pc, resume_pc);
        if (block_pc != 0U && block_pc != current_pc) continue;
        block_pc = m68k_recompiled_002512_p3(api, current_pc, resume_pc);
        if (block_pc != 0U && block_pc != current_pc) continue;
        block_pc = m68k_recompiled_002512_p4(api, current_pc, resume_pc);
        if (block_pc != 0U && block_pc != current_pc) continue;
        if (block_pc == current_pc) break; // Unhandled block
    }
    return M68k9D70StepResult::InstructionStepped;
}
} // namespace oasis::hybrid::generated
