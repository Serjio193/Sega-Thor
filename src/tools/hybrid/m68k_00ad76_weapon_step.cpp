#include "tools/hybrid/m68k_00ad76_weapon_step.hpp"
#include "tools/hybrid/m68k_00ad76_step_ops.hpp"

namespace oasis::hybrid {
using namespace step_00ad76;

M68k9D70StepResult step_00ad76_weapon(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& child_call_active,
                                      unsigned& saved_child_sp,
                                      M68k00AD76StepMetrics& metrics) noexcept {
    switch (pc) {
    case 0x00B012U: { // bsr.w $00B34C (weapon piece descriptor hook)
        metrics.stepped_instructions++;
        return step_bsr_w(api, pc, 0x00B34CU, 0x00B016U, resume_pc, child_call_active, saved_child_sp);
    }
    case 0x00B016U: { // bra.w $00AEE6 (resume piece loop)
        metrics.stepped_instructions++;
        return step_bra_w(api, pc, 0x00AEE6U, resume_pc);
    }
    case 0x00B01AU: { // bsr.w $00B3B2 (weapon shadow hook)
        metrics.stepped_instructions++;
        return step_bsr_w(api, pc, 0x00B3B2U, 0x00B01EU, resume_pc, child_call_active, saved_child_sp);
    }
    case 0x00B01EU: { // bra.w $00AF9C (resume shadow pipeline)
        metrics.stepped_instructions++;
        return step_bra_w(api, pc, 0x00AF9CU, resume_pc);
    }
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid
