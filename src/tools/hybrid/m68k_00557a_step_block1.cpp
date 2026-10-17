#include "tools/hybrid/m68k_00557a_step_block1.hpp"
#include "tools/hybrid/m68k_00557a_step_ops.hpp"
#include "tools/hybrid/m68k_00f788_step_ops.hpp"
#include "tools/hybrid/m68k_623ac_step_ops.hpp"

namespace oasis::hybrid::step_00557a {
using namespace step_623ac;
using namespace step_00f788;

M68k9D70StepResult step_00557a_block1(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& child_call_active,
                                      unsigned& saved_child_sp) noexcept {
    switch (pc) {
    case 0x00557AU:
        return step_sf_abs(api, pc, 0x00FF1986U, 0x005580U, resume_pc);
    case 0x005580U:
        return step_lea_abs(api, pc, 0x00FF19E8U, 14U, 0x005586U, resume_pc);
    case 0x005586U:
        return step_bsr_w(api, pc, 0x005670U, 0x00558AU, resume_pc, child_call_active, saved_child_sp);
    case 0x00558AU:
        return step_lea_abs(api, pc, 0x00FF19E8U, 14U, 0x005590U, resume_pc);
    case 0x005590U:
        return step_clr_w_reg(api, pc, 1U, 0x005592U, resume_pc);
    case 0x005592U:
        return step_move_w_reg_reg(api, pc, 1U, 2U, 0x005594U, resume_pc);
    case 0x005594U:
        return step_move_b_disp_reg(api, pc, 14U, 72, 1U, 0x005598U, resume_pc);
    case 0x005598U:
        return step_move_b_disp_reg(api, pc, 14U, 73, 2U, 0x00559CU, resume_pc);
    case 0x00559CU:
        return step_btst_imm_disp(api, pc, 0U, 14U, 23, 0x0055A2U, resume_pc);
    case 0x0055A2U:
        return step_bcc_w(api, pc, 7U, 0x0055A8U, 0x0055A6U, resume_pc);
    case 0x0055A6U:
        return step_exg_reg_reg(api, pc, 1U, 2U, 0x0055A8U, resume_pc);
    case 0x0055A8U:
        return step_move_w_reg_disp(api, pc, 1U, 14U, 66, 0x0055ACU, resume_pc);
    case 0x0055ACU:
        return step_move_w_reg_disp(api, pc, 2U, 14U, 68, 0x0055B0U, resume_pc);
    case 0x0055B0U:
        return step_move_b_abs_reg(api, pc, 0x00FF1993U, 0U, 0x0055B6U, resume_pc);
    case 0x0055B6U:
        return step_bcc_w(api, pc, 7U, 0x0055C6U, 0x0055BAU, resume_pc);
    case 0x0055BAU:
        return step_subq_b_reg(api, pc, 1U, 0U, 0x0055BCU, resume_pc);
    case 0x0055BCU:
        return step_move_b_reg_abs(api, pc, 0U, 0x00FF1993U, 0x0055C2U, resume_pc);
    case 0x0055C2U:
        return step_bra_w(api, pc, 0x0055EEU, resume_pc);
    case 0x0055C6U:
        return step_move_w_disp_reg(api, pc, 14U, 138, 0U, 0x0055CAU, resume_pc);
    case 0x0055CAU:
        return step_bcc_w(api, pc, 7U, 0x0055EEU, 0x0055CEU, resume_pc);
    case 0x0055CEU:
        return step_cmpi_w_reg(api, pc, 64U, 0U, 0x0055D2U, resume_pc);
    case 0x0055D2U:
        return step_bcc_w(api, pc, 2U, 0x0055EEU, 0x0055D6U, resume_pc);
    case 0x0055D6U:
        return step_lsr_w_reg_1(api, pc, 0U, 0x0055D8U, resume_pc);
    case 0x0055D8U:
        return step_addi_w_reg(api, pc, 10U, 0U, 0x0055DCU, resume_pc);
    case 0x0055DCU:
        return step_move_b_reg_abs(api, pc, 0U, 0x00FF1993U, 0x0055E2U, resume_pc);
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid::step_00557a
