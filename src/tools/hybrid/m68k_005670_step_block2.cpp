#include "tools/hybrid/m68k_005670_step_block2.hpp"
#include "tools/hybrid/m68k_005670_step_ops.hpp"

namespace oasis::hybrid::step_005670 {

M68k9D70StepResult step_005670_block2(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& child_call_active,
                                      unsigned& saved_child_sp) noexcept {
    switch (pc) {
    case 0x0057D6U:
        return step_tst_b_abs(api, pc, 0x00FF198BU, 0x0057DCU, resume_pc);
    case 0x0057DCU:
        return step_bcc_w(api, pc, 7U, 0x0057E6U, 0x0057E0U, resume_pc);
    case 0x0057E0U:
        return step_subq_b_abs(api, pc, 1U, 0x00FF198BU, 0x0057E6U, resume_pc);
    case 0x0057E6U:
        return step_move_w_imm_disp(api, pc, 40U, 14U, 74, 0x0057ECU, resume_pc);
    case 0x0057ECU:
        return step_move_b_disp_reg(api, pc, 14U, 108, 0U, 0x0057F0U, resume_pc);
    case 0x0057F0U:
        return step_bcc_w(api, pc, 7U, 0x0057FAU, 0x0057F4U, resume_pc);
    case 0x0057F4U:
        return step_subq_b_reg(api, pc, 1U, 0U, 0x0057F6U, resume_pc);
    case 0x0057F6U:
        return step_move_b_reg_disp(api, pc, 0U, 14U, 108, 0x0057FAU, resume_pc);
    case 0x0057FAU:
        return step_move_w_disp_reg(api, pc, 14U, 134, 0U, 0x0057FEU, resume_pc);
    case 0x0057FEU:
        return step_bcc_w(api, pc, 7U, 0x005942U, 0x005802U, resume_pc);
    case 0x005802U:
        return step_clr_w_disp(api, pc, 14U, 134, 0x005806U, resume_pc);
    case 0x005806U:
        return step_tst_b_abs(api, pc, 0x00FF1989U, 0x00580CU, resume_pc);
    case 0x00580CU:
        return step_bcc_s(api, pc, 7U, 0x00586CU, 0x00580EU, resume_pc);
    case 0x00580EU:
        return step_tst_l_disp(api, pc, 14U, 16, 0x005812U, resume_pc);
    case 0x005812U:
        return step_bcc_s(api, pc, 6U, 0x00586CU, 0x005814U, resume_pc);
    case 0x005814U:
        return step_lea_disp_reg(api, pc, 14U, 188, 8U, 0x005818U, resume_pc);
    case 0x005818U:
        return step_cmpi_w_ind(api, pc, 28U, 8U, 0x00581EU, resume_pc);
    case 0x00581EU:
        return step_bcc_s(api, pc, 6U, 0x005866U, 0x005820U, resume_pc);
    case 0x005820U:
        return step_move_w_reg_disp(api, pc, 0U, 8U, 134, 0x005824U, resume_pc);
    case 0x005824U:
        return step_move_l_disp_disp(api, pc, 14U, 126, 8U, 126, 0x00582AU, resume_pc);
    case 0x00582AU:
        return step_move_b_disp_disp(api, pc, 14U, 130, 8U, 130, 0x005830U, resume_pc);
    case 0x005830U:
        return step_move_b_disp_disp(api, pc, 14U, 131, 8U, 131, 0x005836U, resume_pc);
    case 0x005836U:
        return step_move_w_disp_disp(api, pc, 14U, 132, 8U, 132, 0x00583CU, resume_pc);
    case 0x00583CU:
        return step_sf_disp(api, pc, 8U, 157, 0x005840U, resume_pc);
    case 0x005840U:
        return step_clr_l_disp(api, pc, 14U, 126, 0x005844U, resume_pc);
    case 0x005844U:
        return step_clr_b_disp(api, pc, 14U, 130, 0x005848U, resume_pc);
    case 0x005848U:
        return step_clr_b_disp(api, pc, 14U, 131, 0x00584CU, resume_pc);
    case 0x00584CU:
        return step_clr_w_disp(api, pc, 14U, 132, 0x005850U, resume_pc);
    case 0x005850U:
        return step_bsr_w(api, pc, 0x0062CCU, 0x005854U, resume_pc, child_call_active, saved_child_sp);
    case 0x005854U:
        return step_move_b_imm_abs(api, pc, 7U, 0x00FF198EU, 0x00585CU, resume_pc);
    case 0x00585CU:
        return step_bset_imm_disp(api, pc, 3U, 14U, 56, 0x005862U, resume_pc);
    case 0x005862U:
        return step_bra_w(api, pc, 0x005942U, resume_pc);
    case 0x005866U:
        return step_sf_abs(api, pc, 0x00FF1989U, 0x00586CU, resume_pc);
    case 0x00586CU:
        return step_move_w_reg_reg(api, pc, 0U, 7U, 0x00586EU, resume_pc);
    case 0x00586EU:
        return step_andi_w_reg(api, pc, 0x0FFFU, 0U, 0x005872U, resume_pc);
    case 0x005872U:
        return step_sub_w_disp_reg(api, pc, 14U, 146, 0U, 0x005876U, resume_pc);
    case 0x005876U:
        return step_bcc_s(api, pc, 2U, 0x00587AU, 0x005878U, resume_pc);
    case 0x005878U:
        return step_moveq(api, pc, 1U, 0U, 0x00587AU, resume_pc);
    case 0x00587AU:
        return step_move_w_disp_reg(api, pc, 14U, 138, 1U, 0x00587EU, resume_pc);
    case 0x00587EU:
        return step_cmp_w_reg_reg(api, pc, 0U, 1U, 0x005880U, resume_pc);
    case 0x005880U:
        return step_bcc_s(api, pc, 4U, 0x005884U, 0x005882U, resume_pc);
    case 0x005882U:
        return step_move_w_reg_reg(api, pc, 1U, 0U, 0x005884U, resume_pc);
    case 0x005884U:
        return step_sub_w_reg_reg(api, pc, 0U, 1U, 0x005886U, resume_pc);
    case 0x005886U:
        return step_move_w_reg_disp(api, pc, 1U, 14U, 138, 0x00588AU, resume_pc);
    case 0x00588AU:
        return step_add_w_reg_abs(api, pc, 0U, 0x00FF0E3CU, 0x005890U, resume_pc);
    case 0x005890U:
        return step_bsr_w(api, pc, 0x0085C4U, 0x005894U, resume_pc, child_call_active, saved_child_sp);
    case 0x005894U:
        return step_tst_w_disp(api, pc, 14U, 138, 0x005898U, resume_pc);
    case 0x005898U:
        return step_bcc_s(api, pc, 6U, 0x0058C2U, 0x00589AU, resume_pc);
    case 0x00589AU:
        return step_jsr_abs(api, pc, 0x00E4D0U, 0x0058A0U, resume_pc, child_call_active, saved_child_sp);
    case 0x0058A0U:
        return step_ori_w_reg(api, pc, 0xC000U, 7U, 0x0058A4U, resume_pc);
    case 0x0058A4U:
        return step_st_abs(api, pc, 0x00FF1987U, 0x0058AAU, resume_pc);
    case 0x0058AAU:
        return step_clr_w_disp(api, pc, 14U, 142, 0x0058AEU, resume_pc);
    case 0x0058AEU:
        return step_tst_w_abs(api, pc, 0x00FF1AA4U, 0x0058B4U, resume_pc);
    case 0x0058B4U:
        return step_bcc_s(api, pc, 7U, 0x0058C2U, 0x0058B6U, resume_pc);
    case 0x0058B6U:
        return step_move_w_imm_disp(api, pc, 1U, 14U, 142, 0x0058BCU, resume_pc);
    case 0x0058BCU:
        return step_move_w_imm_disp(api, pc, 26U, 14U, 330, 0x0058C2U, resume_pc);
    case 0x0058C2U:
        return step_movea_l_disp_reg(api, pc, 14U, 126, 8U, 0x0058C6U, resume_pc);
    case 0x0058C6U:
        return step_btst_imm_reg(api, pc, 13U, 7U, 0x0058CAU, resume_pc);
    case 0x0058CAU:
        return step_bcc_s(api, pc, 7U, 0x0058D2U, 0x0058CCU, resume_pc);
    case 0x0058CCU:
        return step_bset_imm_disp(api, pc, 3U, 14U, 57, 0x0058D2U, resume_pc);
    case 0x0058D2U:
        return step_andi_w_reg(api, pc, 0xC000U, 7U, 0x0058D6U, resume_pc);
    case 0x0058D6U:
        return step_bcc_s(api, pc, 6U, 0x005900U, 0x0058D8U, resume_pc);
    case 0x0058D8U:
        return step_move_w_imm_disp(api, pc, 24U, 14U, 4, 0x0058DEU, resume_pc);
    case 0x0058DEU:
        return step_move_w_disp_reg(api, pc, 14U, 132, 0U, 0x0058E2U, resume_pc);
    case 0x0058E2U:
        return step_cmpi_w_disp(api, pc, 3U, 14U, 22, 0x0058E8U, resume_pc);
    case 0x0058E8U:
        return step_bcc_s(api, pc, 6U, 0x0058F4U, 0x0058EAU, resume_pc);
    case 0x0058EAU:
        return step_btst_imm_reg(api, pc, 0U, 0U, 0x0058EEU, resume_pc);
    case 0x0058EEU:
        return step_bcc_s(api, pc, 7U, 0x0058F4U, 0x0058F0U, resume_pc);
    case 0x0058F0U:
        return step_bchg_imm_reg(api, pc, 1U, 0U, 0x0058F4U, resume_pc);
    case 0x0058F4U:
        return step_add_w_reg_reg(api, pc, 0U, 0U, 0x0058F6U, resume_pc);
    case 0x0058F6U:
        return step_addi_w_reg(api, pc, 24U, 0U, 0x0058FAU, resume_pc);
    case 0x0058FAU:
        return step_bsr_w(api, pc, 0x008EF8U, 0x0058FEU, resume_pc, child_call_active, saved_child_sp);
    case 0x0058FEU:
        return step_bra_s(api, pc, 0x005942U, resume_pc);
    case 0x005900U:
        return step_move_w_disp_reg(api, pc, 14U, 132, 1U, 0x005904U, resume_pc);
    case 0x005904U:
        return step_cmp_w_disp_reg(api, pc, 14U, 22, 1U, 0x005908U, resume_pc);
    case 0x005908U:
        return step_bcc_s(api, pc, 7U, 0x00591EU, 0x00590AU, resume_pc);
    case 0x00590AU:
        return step_bchg_imm_reg(api, pc, 1U, 1U, 0x00590EU, resume_pc);
    case 0x00590EU:
        return step_move_w_reg_disp(api, pc, 1U, 14U, 22, 0x005912U, resume_pc);
    case 0x005912U:
        return step_move_w_imm_disp(api, pc, 26U, 14U, 4, 0x005918U, resume_pc);
    case 0x005918U:
        return step_move_w_imm_reg(api, pc, 38U, 0U, 0x00591CU, resume_pc);
    case 0x00591CU:
        return step_bra_s(api, pc, 0x005928U, resume_pc);
    case 0x00591EU:
        return step_move_w_imm_disp(api, pc, 32U, 14U, 4, 0x005924U, resume_pc);
    case 0x005924U:
        return step_move_w_imm_reg(api, pc, 44U, 0U, 0x005928U, resume_pc);
    case 0x005928U:
        return step_bsr_w(api, pc, 0x008EF8U, 0x00592CU, resume_pc, child_call_active, saved_child_sp);
    case 0x00592CU:
        return step_addi_w_disp(api, pc, 16U, 14U, 16, 0x005932U, resume_pc);
    case 0x005932U:
        return step_sf_disp(api, pc, 14U, 131, 0x005936U, resume_pc);
    case 0x005936U:
        return step_bset_imm_disp(api, pc, 3U, 14U, 56, 0x00593CU, resume_pc);
    case 0x00593CU:
        return step_sf_abs(api, pc, 0x00FF198EU, 0x005942U, resume_pc);
    case 0x005942U:
        return step_move_b_disp_reg(api, pc, 14U, 64, 0U, 0x005946U, resume_pc);
    case 0x005946U:
        return step_move_b_disp_reg(api, pc, 14U, 65, 1U, 0x00594AU, resume_pc);
    case 0x00594AU:
        return step_move_w_disp_reg(api, pc, 14U, 4, 2U, 0x00594EU, resume_pc);
    case 0x00594EU:
        return step_btst_imm_disp(api, pc, 7U, 14U, 111, 0x005954U, resume_pc);
    case 0x005954U:
        return step_bcc_w(api, pc, 6U, 0x005A72U, 0x005958U, resume_pc);
    case 0x005958U:
        return step_bclr_imm_disp(api, pc, 3U, 14U, 54, 0x00595EU, resume_pc);
    case 0x00595EU:
        return step_tst_b_abs(api, pc, 0x00FF1985U, 0x005964U, resume_pc);
    case 0x005964U:
        return step_bcc_w(api, pc, 6U, 0x0059A8U, 0x005968U, resume_pc);
    case 0x005968U:
        return step_cmpi_w_reg(api, pc, 2U, 2U, 0x00596CU, resume_pc);
    case 0x00596CU:
        return step_bcc_w(api, pc, 7U, 0x005978U, 0x005970U, resume_pc);
    case 0x005970U:
        return step_cmpi_w_reg(api, pc, 4U, 2U, 0x005974U, resume_pc);
    case 0x005974U:
        return step_bcc_w(api, pc, 6U, 0x0059A8U, 0x005978U, resume_pc);
    case 0x005978U:
        return step_btst_imm_reg(api, pc, 5U, 0U, 0x00597CU, resume_pc);
    case 0x00597CU:
        return step_bcc_w(api, pc, 6U, 0x005A30U, 0x005980U, resume_pc);
    case 0x005980U:
        return step_btst_imm_reg(api, pc, 5U, 1U, 0x005984U, resume_pc);
    case 0x005984U:
        return step_bcc_w(api, pc, 6U, 0x005A48U, 0x005988U, resume_pc);
    case 0x005988U:
        return step_btst_imm_reg(api, pc, 6U, 0U, 0x00598CU, resume_pc);
    case 0x00598CU:
        return step_bcc_w(api, pc, 6U, 0x0059A8U, 0x005990U, resume_pc);
    case 0x005990U:
        return step_btst_imm_reg(api, pc, 6U, 1U, 0x005994U, resume_pc);
    case 0x005994U:
        return step_bcc_w(api, pc, 6U, 0x0059A8U, 0x005998U, resume_pc);
    case 0x005998U:
        return step_btst_imm_reg(api, pc, 7U, 0U, 0x00599CU, resume_pc);
    case 0x00599CU:
        return step_bcc_w(api, pc, 6U, 0x005B2CU, 0x0059A0U, resume_pc);
    case 0x0059A0U:
        return step_btst_imm_reg(api, pc, 7U, 1U, 0x0059A4U, resume_pc);
    case 0x0059A4U:
        return step_bcc_w(api, pc, 6U, 0x005B3CU, 0x0059A8U, resume_pc);
    case 0x0059A8U:
        return step_move_w_abs_reg(api, pc, 0x00FF165EU, 7U, 0x0059AEU, resume_pc);
    case 0x0059AEU:
        return step_move_w_disp_reg(api, pc, 14U, 4, 0U, 0x0059B2U, resume_pc);
    case 0x0059B2U:
        return step_lea_pc_disp_reg(api, pc, 0U, 8U, 0x0059B6U, resume_pc);
    case 0x0059B6U:
        return step_adda_w_ind_reg(api, pc, 8U, 0x0059B8U, resume_pc);
    case 0x0059B8U:
        return step_jmp_ind(api, pc, 8U, resume_pc);
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid::step_005670
