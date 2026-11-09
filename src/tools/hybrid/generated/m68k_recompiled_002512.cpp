#include "tools/hybrid/generated/m68k_recompiled_002512.hpp"
#include "tools/hybrid/generated/m68k_recompiled_002512_right.hpp"
#include "tools/hybrid/generated/m68k_recompiled_002512_left.hpp"
#include "tools/hybrid/generated/m68k_recompiled_002512_upload.hpp"

namespace oasis::hybrid::generated {

M68k9D70StepResult m68k_recompiled_002512(const M68k9D70StepApi& api, unsigned& resume_pc) noexcept {
    unsigned block_pc = 0x002512U;
    unsigned step_count = 0;
    while (block_pc != 0U && step_count < 2000U) {
        ++step_count;
        if (block_pc < 0x002596U) {
            block_pc = m68k_recompiled_002512_right(api, block_pc, resume_pc);
        } else if (block_pc < 0x0025E2U) {
            block_pc = m68k_recompiled_002512_left(api, block_pc, resume_pc);
        } else {
            block_pc = m68k_recompiled_002512_upload(api, block_pc, resume_pc);
        }
    }
    return M68k9D70StepResult::InstructionStepped;
}
} // namespace oasis::hybrid::generated
