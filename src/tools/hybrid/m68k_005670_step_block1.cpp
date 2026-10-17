#include "tools/hybrid/m68k_005670_step_block1.hpp"
#include "tools/hybrid/m68k_005670_step_ops.hpp"

namespace oasis::hybrid::step_005670 {

M68k9D70StepResult step_005670_block1(const M68k9D70StepApi& api, unsigned pc,
                                      unsigned& resume_pc, bool& child_call_active,
                                      unsigned& saved_child_sp) noexcept {
    switch (pc) {
    case 0x005670U:
        return step_move_l_reg_abs(api, pc, 15U, 0x00FF1934U, 0x005676U, resume_pc);
    case 0x005676U:
        return step_move_b_abs_reg(api, pc, 0x00FF1985U, 0U, 0x00567CU, resume_pc);
    case 0x00567CU:
        return step_bcc_s(api, pc, 7U, 0x005686U, 0x00567EU, resume_pc);
    case 0x00567EU:
        return step_clr_l_disp(api, pc, 14U, 114, 0x005682U, resume_pc);
    case 0x005682U:
        return step_clr_l_disp(api, pc, 14U, 118, 0x005686U, resume_pc);
    case 0x005686U:
        return step_tst_b_reg(api, pc, 0U, 0x005688U, resume_pc);
    case 0x005688U:
        return step_bcc_w(api, pc, 6U, 0x0056AEU, 0x00568CU, resume_pc);
    case 0x00568CU:
        return step_move_b_abs_reg(api, pc, 0x00FF198EU, 0U, 0x005692U, resume_pc);
    case 0x005692U:
        return step_bcc_w(api, pc, 7U, 0x0056AEU, 0x005696U, resume_pc);
    case 0x005696U:
        return step_bset_imm_disp(api, pc, 3U, 14U, 56, 0x00569CU, resume_pc);
    case 0x00569CU:
        return step_subq_b_reg(api, pc, 1U, 0U, 0x00569EU, resume_pc);
    case 0x00569EU:
        return step_move_b_reg_abs(api, pc, 0U, 0x00FF198EU, 0x0056A4U, resume_pc);
    case 0x0056A4U:
        return step_bcc_w(api, pc, 6U, 0x0056AEU, 0x0056A8U, resume_pc);
    case 0x0056A8U:
        return step_bclr_imm_disp(api, pc, 3U, 14U, 56, 0x0056AEU, resume_pc);
    case 0x0056AEU:
        return step_tst_b_abs(api, pc, 0x00FF198FU, 0x0056B4U, resume_pc);
    case 0x0056B4U:
        return step_bcc_w(api, pc, 7U, 0x0056D8U, 0x0056B8U, resume_pc);
    case 0x0056B8U:
        return step_cmpi_w_abs(api, pc, 58U, 0x00FF1B60U, 0x0056C0U, resume_pc);
    case 0x0056C0U:
        return step_bcc_w(api, pc, 7U, 0x0056D8U, 0x0056C4U, resume_pc);
    case 0x0056C4U:
        return step_move_w_abs_disp(api, pc, 0x00FF0D7EU, 14U, 6, 0x0056CCU, resume_pc);
    case 0x0056CCU:
        return step_move_w_imm_disp(api, pc, 0xFFFFU, 14U, 92, 0x0056D2U, resume_pc);
    case 0x0056D2U:
        return step_sf_abs(api, pc, 0x00FF198FU, 0x0056D8U, resume_pc);
    case 0x0056D8U:
        return step_tst_b_abs(api, pc, 0x00FF1983U, 0x0056DEU, resume_pc);
    case 0x0056DEU:
        return step_bcc_w(api, pc, 7U, 0x0056FCU, 0x0056E2U, resume_pc);
    case 0x0056E2U:
        return step_clr_w_abs(api, pc, 0x00FF165EU, 0x0056E8U, resume_pc);
    case 0x0056E8U:
        return step_tst_b_abs(api, pc, 0x00FF1985U, 0x0056EEU, resume_pc);
    case 0x0056EEU:
        return step_bcc_w(api, pc, 7U, 0x0056FCU, 0x0056F2U, resume_pc);
    case 0x0056F2U:
        return step_move_b_imm_abs(api, pc, 1U, 0x00FF1985U, 0x0056FAU, resume_pc);
    case 0x0056FAU:
        return step_rts(api, pc, resume_pc);
    case 0x0056FCU:
        return step_tst_b_abs(api, pc, 0x00FF1987U, 0x005702U, resume_pc);
    case 0x005702U:
        return step_bcc_w(api, pc, 6U, 0x0057B2U, 0x005706U, resume_pc);
    case 0x005706U:
        return step_tst_b_abs(api, pc, 0x00FF1985U, 0x00570CU, resume_pc);
    case 0x00570CU:
        return step_bcc_w(api, pc, 6U, 0x0057B2U, 0x005710U, resume_pc);
    case 0x005710U:
        return step_tst_w_disp(api, pc, 14U, 4, 0x005714U, resume_pc);
    case 0x005714U:
        return step_bcc_w(api, pc, 6U, 0x005762U, 0x005718U, resume_pc);
    case 0x005718U:
        return step_btst_imm_abs(api, pc, 0U, 0x00FF0DBAU, 0x005720U, resume_pc);
    case 0x005720U:
        return step_bcc_w(api, pc, 7U, 0x005762U, 0x005724U, resume_pc);
    case 0x005724U:
        return step_btst_imm_abs(api, pc, 5U, 0x00FF16F0U, 0x00572CU, resume_pc);
    case 0x00572CU:
        return step_bcc_w(api, pc, 7U, 0x005762U, 0x005730U, resume_pc);
    case 0x005730U:
        return step_moveq(api, pc, 15U, 1U, 0x005732U, resume_pc);
    case 0x005732U:
        return step_btst_imm_abs(api, pc, 3U, 0x00FF0DBAU, 0x00573AU, resume_pc);
    case 0x00573AU:
        return step_bcc_s(api, pc, 7U, 0x00573EU, 0x00573CU, resume_pc);
    case 0x00573CU:
        return step_moveq(api, pc, 7U, 1U, 0x00573EU, resume_pc);
    case 0x00573EU:
        return step_move_b_abs_reg(api, pc, 0x00FF1971U, 0U, 0x005744U, resume_pc);
    case 0x005744U:
        return step_and_b_reg_reg(api, pc, 1U, 0U, 0x005746U, resume_pc);
    case 0x005746U:
        return step_bcc_w(api, pc, 6U, 0x005762U, 0x00574AU, resume_pc);
    case 0x00574AU:
        return step_move_w_disp_reg(api, pc, 14U, 138, 0U, 0x00574EU, resume_pc);
    case 0x00574EU:
        return step_bcc_w(api, pc, 7U, 0x005762U, 0x005752U, resume_pc);
    case 0x005752U:
        return step_move_w_disp_reg(api, pc, 14U, 136, 1U, 0x005756U, resume_pc);
    case 0x005756U:
        return step_cmp_w_reg_reg(api, pc, 1U, 0U, 0x005758U, resume_pc);
    case 0x005758U:
        return step_bcc_w(api, pc, 4U, 0x005762U, 0x00575CU, resume_pc);
    case 0x00575CU:
        return step_addq_w_reg(api, pc, 1U, 0U, 0x00575EU, resume_pc);
    case 0x00575EU:
        return step_move_w_reg_disp(api, pc, 0U, 14U, 138, 0x005762U, resume_pc);
    case 0x005762U:
        return step_tst_w_abs(api, pc, 0x00FF1AA4U, 0x005768U, resume_pc);
    case 0x005768U:
        return step_bcc_w(api, pc, 6U, 0x0057B2U, 0x00576CU, resume_pc);
    case 0x00576CU:
        return step_btst_imm_abs(api, pc, 5U, 0x00FF16F0U, 0x005774U, resume_pc);
    case 0x005774U:
        return step_bcc_w(api, pc, 6U, 0x005784U, 0x005778U, resume_pc);
    case 0x005778U:
        return step_btst_imm_abs(api, pc, 2U, 0x00FF0DBAU, 0x005780U, resume_pc);
    case 0x005780U:
        return step_bcc_w(api, pc, 7U, 0x0057B2U, 0x005784U, resume_pc);
    case 0x005784U:
        return step_moveq(api, pc, 7U, 1U, 0x005786U, resume_pc);
    case 0x005786U:
        return step_btst_imm_abs(api, pc, 3U, 0x00FF0DBAU, 0x00578EU, resume_pc);
    case 0x00578EU:
        return step_bcc_s(api, pc, 7U, 0x005792U, 0x005790U, resume_pc);
    case 0x005790U:
        return step_moveq(api, pc, 3U, 1U, 0x005792U, resume_pc);
    case 0x005792U:
        return step_move_b_abs_reg(api, pc, 0x00FF1971U, 0U, 0x005798U, resume_pc);
    case 0x005798U:
        return step_and_b_reg_reg(api, pc, 1U, 0U, 0x00579AU, resume_pc);
    case 0x00579AU:
        return step_bcc_w(api, pc, 6U, 0x0057B2U, 0x00579EU, resume_pc);
    case 0x00579EU:
        return step_move_w_disp_reg(api, pc, 14U, 140, 1U, 0x0057A2U, resume_pc);
    case 0x0057A2U:
        return step_move_w_disp_reg(api, pc, 14U, 142, 0U, 0x0057A6U, resume_pc);
    case 0x0057A6U:
        return step_cmp_w_reg_reg(api, pc, 1U, 0U, 0x0057A8U, resume_pc);
    case 0x0057A8U:
        return step_bcc_w(api, pc, 4U, 0x0057B2U, 0x0057ACU, resume_pc);
    case 0x0057ACU:
        return step_addq_w_reg(api, pc, 1U, 0U, 0x0057AEU, resume_pc);
    case 0x0057AEU:
        return step_move_w_reg_disp(api, pc, 0U, 14U, 142, 0x0057B2U, resume_pc);
    case 0x0057B2U:
        return step_tst_b_abs(api, pc, 0x00FF1982U, 0x0057B8U, resume_pc);
    case 0x0057B8U:
        return step_bcc_w(api, pc, 7U, 0x0057D0U, 0x0057BCU, resume_pc);
    case 0x0057BCU:
        return step_tst_b_abs(api, pc, 0x00FF1985U, 0x0057C2U, resume_pc);
    case 0x0057C2U:
        return step_bcc_w(api, pc, 7U, 0x0057CEU, 0x0057C6U, resume_pc);
    case 0x0057C6U:
        return step_move_b_imm_abs(api, pc, 1U, 0x00FF1985U, 0x0057CCU, resume_pc);
    case 0x0057CEU:
        return step_rts(api, pc, resume_pc);
    case 0x0057D0U:
        return step_jsr_abs(api, pc, 0x00E682U, 0x0057D6U, resume_pc, child_call_active, saved_child_sp);
    default:
        return M68k9D70StepResult::NotHandled;
    }
}

} // namespace oasis::hybrid::step_005670
