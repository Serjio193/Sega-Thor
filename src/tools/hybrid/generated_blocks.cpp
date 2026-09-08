// GENERATED FILE: oasis_hybrid_recomp_generate; do not hand-edit.
#include "tools/hybrid/generated_block_runtime.hpp"
#include "tools/hybrid/generated_blocks.hpp"

namespace oasis::hybrid::generated {

void execute_0x002D66(BasicBlockApi& api) {
    // guest 0x002D66 opcode 0x48E7 48E7 0110 movem.l D7/A3,-(A7)
    const auto opcode_0x002D66 = fetch_checked(api, 0x48E7U);
    api.begin_instruction(opcode_0x002D66);
    (void)fetch_checked(api, 0x0110U);
    movem_l_predecrement(api, 0x0880U);
    api.finish_instruction(opcode_0x002D66);
    api.add_cycles(112);
    api.skip_bus_refresh();
    // guest 0x002D6A opcode 0x4247 4247 clr.w D7
    const auto opcode_0x002D6A = fetch_checked(api, 0x4247U);
    api.begin_instruction(opcode_0x002D6A);
    clear_w_data_register(api, 7U);
    api.finish_instruction(opcode_0x002D6A);
    // guest 0x002D6C opcode 0x1E1E 1E1E move.b (A6)+,D7
    const auto opcode_0x002D6C = fetch_checked(api, 0x1E1EU);
    api.begin_instruction(opcode_0x002D6C);
    move_b_postincrement_to_data_register(api, 6U, 7U);
    api.finish_instruction(opcode_0x002D6C);
    // guest 0x002D6E opcode 0x47F9 47F9 00FF 134C lea.l ($00FF134C).L,A3
    const auto opcode_0x002D6E = fetch_checked(api, 0x47F9U);
    api.begin_instruction(opcode_0x002D6E);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x134CU);
    lea_absolute_long(api, 0xFF134CU, 3U);
    api.finish_instruction(opcode_0x002D6E);
    // guest 0x002D74 opcode 0xD6C7 D6C7 adda.w D7,A3
    const auto opcode_0x002D74 = fetch_checked(api, 0xD6C7U);
    api.begin_instruction(opcode_0x002D74);
    adda_w_data_to_address(api, 7U, 3U);
    api.finish_instruction(opcode_0x002D74);
    // guest 0x002D76 opcode 0x1E1E 1E1E move.b (A6)+,D7
    const auto opcode_0x002D76 = fetch_checked(api, 0x1E1EU);
    api.begin_instruction(opcode_0x002D76);
    move_b_postincrement_to_data_register(api, 6U, 7U);
    api.finish_instruction(opcode_0x002D76);
    // guest 0x002D78 opcode 0x36DE 36DE move.w (A6)+,(A3)+
    const auto opcode_0x002D78 = fetch_checked(api, 0x36DEU);
    api.begin_instruction(opcode_0x002D78);
    move_w_postincrement_to_postincrement(api, 6U, 3U);
    api.finish_instruction(opcode_0x002D78);
}

void execute_0x0604BC(BasicBlockApi& api) {
    // guest 0x0604BC opcode 0x4DF9 4DF9 00FF 0628 lea.l ($00FF0628).L,A6
    const auto opcode_0x0604BC = fetch_checked(api, 0x4DF9U);
    api.begin_instruction(opcode_0x0604BC);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0628U);
    lea_absolute_long(api, 0xFF0628U, 6U);
    api.finish_instruction(opcode_0x0604BC);
}

void execute_0x061032(BasicBlockApi& api) {
    // guest 0x061032 opcode 0xD481 D481 add.l D1,D2
    const auto opcode_0x061032 = fetch_checked(api, 0xD481U);
    api.begin_instruction(opcode_0x061032);
    add_l_data_to_data(api, 1U, 2U);
    api.finish_instruction(opcode_0x061032);
}

void execute_0x03A85E(BasicBlockApi& api) {
    // guest 0x03A85E opcode 0x4A79 4A79 00FF 1654 tst.w ($00FF1654).L
    const auto opcode_0x03A85E = fetch_checked(api, 0x4A79U);
    api.begin_instruction(opcode_0x03A85E);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1654U);
    test_absolute_long(api, 0xFF1654U, 2U);
    api.finish_instruction(opcode_0x03A85E);
}

void execute_0x03A8BA(BasicBlockApi& api) {
    // guest 0x03A8BA opcode 0x4A79 4A79 00FF 1654 tst.w ($00FF1654).L
    const auto opcode_0x03A8BA = fetch_checked(api, 0x4A79U);
    api.begin_instruction(opcode_0x03A8BA);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x1654U);
    test_absolute_long(api, 0xFF1654U, 2U);
    api.finish_instruction(opcode_0x03A8BA);
}

void execute_0x03A88C(BasicBlockApi& api) {
    // guest 0x03A88C opcode 0x4A39 4A39 00FF 0BFD tst.b ($00FF0BFD).L
    const auto opcode_0x03A88C = fetch_checked(api, 0x4A39U);
    api.begin_instruction(opcode_0x03A88C);
    (void)fetch_checked(api, 0x00FFU);
    (void)fetch_checked(api, 0x0BFDU);
    test_absolute_long(api, 0xFF0BFDU, 1U);
    api.finish_instruction(opcode_0x03A88C);
}

void execute_0x0003A0(BasicBlockApi& api) {
    // guest 0x0003A0 opcode 0x51CA 51CA FFDE dbf D2,loc_000380
    const auto opcode_0x0003A0 = fetch_checked(api, 0x51CAU);
    api.begin_instruction(opcode_0x0003A0);
    dbcc(api, 1U, 2U, 0x000380U, 0xFFDEU);
    api.finish_instruction(opcode_0x0003A0);
}

void execute_0x03A8AC(BasicBlockApi& api) {
    // guest 0x03A8AC opcode 0x6600 6600 000C bne.w loc_03A8BA
    const auto opcode_0x03A8AC = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x03A8AC);
    branch_condition(api, 6U, 0x03A8BAU, 14, 0x000CU);
    api.finish_instruction(opcode_0x03A8AC);
}

void execute_0x060312(BasicBlockApi& api) {
    // guest 0x060312 opcode 0x51C8 51C8 FFFC dbf D0,loc_060310
    const auto opcode_0x060312 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x060312);
    dbcc(api, 1U, 0U, 0x060310U, 0xFFFCU);
    api.finish_instruction(opcode_0x060312);
}

void execute_0x002230(BasicBlockApi& api) {
    // guest 0x002230 opcode 0x51C8 51C8 FFFE dbf D0,loc_002230
    const auto opcode_0x002230 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x002230);
    dbcc(api, 1U, 0U, 0x002230U, 0xFFFEU);
    api.finish_instruction(opcode_0x002230);
}

void execute_0x061360(BasicBlockApi& api) {
    // guest 0x061360 opcode 0x51C8 51C8 FFFC dbf D0,loc_06135E
    const auto opcode_0x061360 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x061360);
    dbcc(api, 1U, 0U, 0x06135EU, 0xFFFCU);
    api.finish_instruction(opcode_0x061360);
}

void execute_0x003A0E(BasicBlockApi& api) {
    // guest 0x003A0E opcode 0x51CA 51CA FFFC dbf D2,loc_003A0C
    const auto opcode_0x003A0E = fetch_checked(api, 0x51CAU);
    api.begin_instruction(opcode_0x003A0E);
    dbcc(api, 1U, 2U, 0x003A0CU, 0xFFFCU);
    api.finish_instruction(opcode_0x003A0E);
}

void execute_0x0038A0(BasicBlockApi& api) {
    // guest 0x0038A0 opcode 0x51C8 51C8 FFFC dbf D0,loc_00389E
    const auto opcode_0x0038A0 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x0038A0);
    dbcc(api, 1U, 0U, 0x00389EU, 0xFFFCU);
    api.finish_instruction(opcode_0x0038A0);
}

void execute_0x0003F2(BasicBlockApi& api) {
    // guest 0x0003F2 opcode 0x51C8 51C8 FFFC dbf D0,loc_0003F0
    const auto opcode_0x0003F2 = fetch_checked(api, 0x51C8U);
    api.begin_instruction(opcode_0x0003F2);
    dbcc(api, 1U, 0U, 0x0003F0U, 0xFFFCU);
    api.finish_instruction(opcode_0x0003F2);
}

void execute_0x003818(BasicBlockApi& api) {
    // guest 0x003818 opcode 0x66F6 66F6 bne.s loc_003810
    const auto opcode_0x003818 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x003818);
    branch_condition(api, 6U, 0x003810U, -14);
    api.finish_instruction(opcode_0x003818);
}

void execute_0x0030BE(BasicBlockApi& api) {
    // guest 0x0030BE opcode 0x66F6 66F6 bne.s loc_0030B6
    const auto opcode_0x0030BE = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x0030BE);
    branch_condition(api, 6U, 0x0030B6U, -14);
    api.finish_instruction(opcode_0x0030BE);
}

void execute_0x03A758(BasicBlockApi& api) {
    // guest 0x03A758 opcode 0x66F6 66F6 bne.s loc_03A750
    const auto opcode_0x03A758 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x03A758);
    branch_condition(api, 6U, 0x03A750U, -14);
    api.finish_instruction(opcode_0x03A758);
}

void execute_0x00D994(BasicBlockApi& api) {
    // guest 0x00D994 opcode 0x51CB 51CB FFFA dbf D3,loc_00D990
    const auto opcode_0x00D994 = fetch_checked(api, 0x51CBU);
    api.begin_instruction(opcode_0x00D994);
    dbcc(api, 1U, 3U, 0x00D990U, 0xFFFAU);
    api.finish_instruction(opcode_0x00D994);
}

void execute_0x003186(BasicBlockApi& api) {
    // guest 0x003186 opcode 0x66F6 66F6 bne.s loc_00317E
    const auto opcode_0x003186 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x003186);
    branch_condition(api, 6U, 0x00317EU, -14);
    api.finish_instruction(opcode_0x003186);
}

void execute_0x003250(BasicBlockApi& api) {
    // guest 0x003250 opcode 0x66F6 66F6 bne.s loc_003248
    const auto opcode_0x003250 = fetch_checked(api, 0x66F6U);
    api.begin_instruction(opcode_0x003250);
    branch_condition(api, 6U, 0x003248U, -14);
    api.finish_instruction(opcode_0x003250);
}

void execute_0x061938(BasicBlockApi& api) {
    // guest 0x061938 opcode 0x6600 6600 000C bne.w loc_061946
    const auto opcode_0x061938 = fetch_checked(api, 0x6600U);
    api.begin_instruction(opcode_0x061938);
    branch_condition(api, 6U, 0x061946U, 14, 0x000CU);
    api.finish_instruction(opcode_0x061938);
}

void execute_0x002C18(BasicBlockApi& api) {
    // guest 0x002C18 opcode 0x66F8 66F8 bne.s loc_002C12
    const auto opcode_0x002C18 = fetch_checked(api, 0x66F8U);
    api.begin_instruction(opcode_0x002C18);
    branch_condition(api, 6U, 0x002C12U, -14);
    api.finish_instruction(opcode_0x002C18);
}

const GeneratedBlockSpec kBlocks[] = {
    {0x002D66U, 0x002D7AU, 7U, execute_0x002D66},
    {0x0604BCU, 0x0604C2U, 1U, execute_0x0604BC},
    {0x061032U, 0x061034U, 1U, execute_0x061032},
    {0x03A85EU, 0x03A864U, 1U, execute_0x03A85E},
    {0x03A8BAU, 0x03A8C0U, 1U, execute_0x03A8BA},
    {0x03A88CU, 0x03A892U, 1U, execute_0x03A88C},
    {0x0003A0U, 0x0003A4U, 1U, execute_0x0003A0},
    {0x03A8ACU, 0x03A8B0U, 1U, execute_0x03A8AC},
    {0x060312U, 0x060316U, 1U, execute_0x060312},
    {0x002230U, 0x002234U, 1U, execute_0x002230},
    {0x061360U, 0x061364U, 1U, execute_0x061360},
    {0x003A0EU, 0x003A12U, 1U, execute_0x003A0E},
    {0x0038A0U, 0x0038A4U, 1U, execute_0x0038A0},
    {0x0003F2U, 0x0003F6U, 1U, execute_0x0003F2},
    {0x003818U, 0x00381AU, 1U, execute_0x003818},
    {0x0030BEU, 0x0030C0U, 1U, execute_0x0030BE},
    {0x03A758U, 0x03A75AU, 1U, execute_0x03A758},
    {0x00D994U, 0x00D998U, 1U, execute_0x00D994},
    {0x003186U, 0x003188U, 1U, execute_0x003186},
    {0x003250U, 0x003252U, 1U, execute_0x003250},
    {0x061938U, 0x06193CU, 1U, execute_0x061938},
    {0x002C18U, 0x002C1AU, 1U, execute_0x002C18},
};

std::span<const GeneratedBlockSpec> blocks() { return kBlocks; }

} // namespace oasis::hybrid::generated
