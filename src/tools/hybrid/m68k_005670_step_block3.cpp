#include "tools/hybrid/m68k_005670_step_block3.hpp"
#include "tools/hybrid/m68k_005670_step_ops.hpp"

namespace oasis::hybrid::step_005670 {

M68k9D70StepResult step_005670_block3(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& child_call_active,
                                      unsigned& saved_child_sp) noexcept {
    switch (pc) {
    case 0x005B3CU:
        return step_btst_imm_disp(api, pc, 1U, 14U, 65, 0x005B42U, resume_pc);
    case 0x005B42U:
        return step_bcc_s(api, pc, 6U, 0x005B48U, 0x005B44U, resume_pc);
    case 0x005B44U:
        return step_moveq(api, pc, 0U, 5U, 0x005B46U, resume_pc);
    case 0x005B46U:
        return step_bra_s(api, pc, 0x005B4AU, resume_pc);
    case 0x005B48U:
        return step_moveq(api, pc, 2U, 5U, 0x005B4AU, resume_pc);
    case 0x005B4AU:
        return step_movea_l_disp_reg(api, pc, 14U, 104, 8U, 0x005B4EU, resume_pc);
    case 0x005B4EU:
        return step_cmpa_l_imm_reg(api, pc, 0x00FF19E8U, 8U, 0x005B54U, resume_pc);
    case 0x005B54U:
        return step_bcc_w(api, pc, 5U, 0x0059A8U, 0x005B58U, resume_pc);
    case 0x005B58U:
        return step_cmpa_l_imm_reg(api, pc, 0x00FF2954U, 8U, 0x005B5EU, resume_pc);
    case 0x005B5EU:
        return step_bcc_w(api, pc, 4U, 0x0059A8U, 0x005B62U, resume_pc);
    case 0x005B62U:
        return step_btst_imm_disp(api, pc, 7U, 8U, 56, 0x005B68U, resume_pc);
    case 0x005B68U:
        return step_bcc_w(api, pc, 7U, 0x0059A8U, 0x005B6CU, resume_pc);
    case 0x005B6CU:
        return step_movem_l_reg_predec(api, pc, 1U, 0x005B70U, resume_pc);
    case 0x005B70U:
        return step_bsr_w(api, pc, 0x0083D4U, 0x005B74U, resume_pc, child_call_active, saved_child_sp);
    case 0x005B74U:
        return step_move_b_abs_reg(api, pc, 0x00FF165EU, 7U, 0x005B7AU, resume_pc);
    case 0x005B7AU:
        return step_andi_w_reg(api, pc, 0x000FU, 7U, 0x005B7EU, resume_pc);
    case 0x005B7EU:
        return step_clr_w_reg(api, pc, 6U, 0x005B80U, resume_pc);
    case 0x005B80U:
        return step_bset_reg_reg(api, pc, 1U, 6U, 0x005B82U, resume_pc);
    case 0x005B82U:
        return step_movem_l_postinc_reg(api, pc, 1U, 0x005B86U, resume_pc);
    case 0x005B86U:
        return step_cmp_b_reg_reg(api, pc, 7U, 6U, 0x005B88U, resume_pc);
    case 0x005B88U:
        return step_bcc_w(api, pc, 6U, 0x0059A8U, 0x005B8CU, resume_pc);
    case 0x005B8CU:
        return step_move_w_reg_disp(api, pc, 5U, 14U, 22, 0x005B90U, resume_pc);
    case 0x005B90U:
        return step_move_w_imm_disp(api, pc, 48U, 14U, 4, 0x005B96U, resume_pc);
    case 0x005B96U:
        return step_move_w_imm_reg(api, pc, 54U, 0U, 0x005B9AU, resume_pc);
    case 0x005B9AU:
        return step_bsr_w(api, pc, 0x008EF8U, 0x005B9EU, resume_pc, child_call_active, saved_child_sp);
    case 0x005B9EU:
        return step_bra_w(api, pc, 0x0059A8U, resume_pc);
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid::step_005670
