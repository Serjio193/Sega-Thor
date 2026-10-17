#include "tools/hybrid/m68k_00557a_step_block2.hpp"
#include "tools/hybrid/m68k_00557a_step_ops.hpp"
#include "tools/hybrid/m68k_00f788_step_ops.hpp"
#include "tools/hybrid/m68k_623ac_step_ops.hpp"

namespace oasis::hybrid::step_00557a {
using namespace step_623ac;
using namespace step_00f788;

M68k9D70StepResult step_00557a_block2(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& child_call_active,
                                      unsigned& saved_child_sp) noexcept {
    switch (pc) {
    case 0x0055E2U:
        return step_move_l_imm_reg(api, pc, 0x00A0038CU, 0U, 0x0055E8U, resume_pc);
    case 0x0055E8U:
        return step_jsr_abs(api, pc, 0x060004U, 0x0055EEU, resume_pc, child_call_active, saved_child_sp);
    case 0x0055EEU:
        return step_move_b_abs_reg(api, pc, 0x00FF1994U, 0U, 0x0055F4U, resume_pc);
    case 0x0055F4U:
        return step_bcc_w(api, pc, 7U, 0x005604U, 0x0055F8U, resume_pc);
    case 0x0055F8U:
        return step_subq_b_reg(api, pc, 1U, 0U, 0x0055FAU, resume_pc);
    case 0x0055FAU:
        return step_move_b_reg_abs(api, pc, 0U, 0x00FF1994U, 0x005600U, resume_pc);
    case 0x005600U:
        return step_bra_w(api, pc, 0x005638U, resume_pc);
    case 0x005604U:
        return step_move_w_abs_reg(api, pc, 0x00FF1AA4U, 0U, 0x00560AU, resume_pc);
    case 0x00560AU:
        return step_bcc_w(api, pc, 7U, 0x005638U, 0x00560EU, resume_pc);
    case 0x00560EU:
        return step_cmpi_w_reg(api, pc, 22U, 0U, 0x005612U, resume_pc);
    case 0x005612U:
        return step_bcc_w(api, pc, 7U, 0x005638U, 0x005616U, resume_pc);
    case 0x005616U:
        return step_move_w_disp_reg(api, pc, 14U, 142, 0U, 0x00561AU, resume_pc);
    case 0x00561AU:
        return step_cmpi_w_reg(api, pc, 24U, 0U, 0x00561EU, resume_pc);
    case 0x00561EU:
        return step_bcc_w(api, pc, 2U, 0x005638U, 0x005622U, resume_pc);
    case 0x005622U:
        return step_addi_w_reg(api, pc, 12U, 0U, 0x005626U, resume_pc);
    case 0x005626U:
        return step_move_b_reg_abs(api, pc, 0U, 0x00FF1994U, 0x00562CU, resume_pc);
    case 0x00562CU:
        return step_move_l_imm_reg(api, pc, 0x00A004DDU, 0U, 0x005632U, resume_pc);
    case 0x005632U:
        return step_jsr_abs(api, pc, 0x060004U, 0x005638U, resume_pc, child_call_active, saved_child_sp);
    case 0x005638U:
        return step_btst_imm_disp(api, pc, 7U, 14U, 111, 0x00563EU, resume_pc);
    case 0x00563EU:
        return step_bcc_w(api, pc, 6U, 0x00565EU, 0x005642U, resume_pc);
    case 0x005642U:
        return step_move_w_disp_reg(api, pc, 14U, 4, 1U, 0x005646U, resume_pc);
    case 0x005646U:
        return step_cmpi_w_reg(api, pc, 0U, 1U, 0x00564AU, resume_pc);
    case 0x00564AU:
        return step_bcc_w(api, pc, 7U, 0x005660U, 0x00564EU, resume_pc);
    case 0x00564EU:
        return step_cmpi_w_reg(api, pc, 2U, 1U, 0x005652U, resume_pc);
    case 0x005652U:
        return step_bcc_w(api, pc, 7U, 0x005660U, 0x005656U, resume_pc);
    case 0x005656U:
        return step_cmpi_w_reg(api, pc, 4U, 1U, 0x00565AU, resume_pc);
    case 0x00565AU:
        return step_bcc_w(api, pc, 7U, 0x005660U, 0x00565EU, resume_pc);
    case 0x00565EU:
        return step_rts(api, pc, resume_pc);
    case 0x005660U:
        return step_lea_abs(api, pc, 0x00FF1972U, 8U, 0x005666U, resume_pc);
    case 0x005666U:
        return step_move_w_disp_postinc(api, pc, 14U, 8, 8U, 0x00566AU, resume_pc);
    case 0x00566AU:
        return step_move_w_disp_ind(api, pc, 14U, 12, 8U, 0x00566EU, resume_pc);
    case 0x00566EU:
        return step_rts(api, pc, resume_pc);
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid::step_00557a
