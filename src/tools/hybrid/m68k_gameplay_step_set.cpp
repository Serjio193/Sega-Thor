#include "tools/hybrid/m68k_gameplay_step_set.hpp"

#include "tools/hybrid/external_library.hpp"
#include "tools/hybrid/m68k_gameplay_step_registry.hpp"
#include "tools/hybrid/m68k_0099d6_instruction_step.hpp"
#include "tools/hybrid/m68k_009678_instruction_step.hpp"
#include "tools/hybrid/m68k_0095aa_instruction_step.hpp"
#include "tools/hybrid/m68k_009ad0_instruction_step.hpp"
#include "tools/hybrid/m68k_009d00_instruction_step.hpp"
#include "tools/hybrid/m68k_0103b0_instruction_step.hpp"
#include "tools/hybrid/m68k_010382_instruction_step.hpp"
#include "tools/hybrid/m68k_00e398_instruction_step.hpp"
#include "tools/hybrid/m68k_01ad14_instruction_step.hpp"
#include "tools/hybrid/m68k_00e2d4_instruction_step.hpp"
#include "tools/hybrid/m68k_06231e_instruction_step.hpp"
#include "tools/hybrid/m68k_0622d0_instruction_step.hpp"
#include "tools/hybrid/m68k_0622dc_instruction_step.hpp"
#include "tools/hybrid/m68k_06230e_instruction_step.hpp"
#include "tools/hybrid/m68k_0622f4_instruction_step.hpp"
#include "tools/hybrid/m68k_03b7c0_instruction_step.hpp"
#include "tools/hybrid/m68k_008d06_instruction_step.hpp"
#include "tools/hybrid/m68k_008910_instruction_step.hpp"
#include "tools/hybrid/m68k_00129a_instruction_step.hpp"
#include "tools/hybrid/m68k_0005ec_instruction_step.hpp"
#include "tools/hybrid/m68k_00d9be_instruction_step.hpp"
#include "tools/hybrid/m68k_008cac_instruction_step.hpp"
#include "tools/hybrid/m68k_008cd0_instruction_step.hpp"
#include "tools/hybrid/m68k_00d7b0_instruction_step.hpp"
#include "tools/hybrid/m68k_0101e4_instruction_step.hpp"
#include "tools/hybrid/m68k_007d8a_instruction_step.hpp"
#include "tools/hybrid/m68k_0085e2_instruction_step.hpp"
#include "tools/hybrid/m68k_0604bc_instruction_step.hpp"
#include "tools/hybrid/m68k_01a288_instruction_step.hpp"
#include "tools/hybrid/m68k_0062cc_instruction_step.hpp"
#include "tools/hybrid/m68k_0101d0_instruction_step.hpp"
#include "tools/hybrid/m68k_01b024_instruction_step.hpp"
#include "tools/hybrid/m68k_0085c4_instruction_step.hpp"
#include "tools/hybrid/m68k_01a21e_instruction_step.hpp"
#include "tools/hybrid/m68k_01ae7e_instruction_step.hpp"
#include "tools/hybrid/m68k_01af3a_instruction_step.hpp"
#include "tools/hybrid/m68k_00c90e_instruction_step.hpp"
#include "tools/hybrid/m68k_00cecc_instruction_step.hpp"
#include "tools/hybrid/m68k_010086_instruction_step.hpp"
#include "tools/hybrid/m68k_00f920_instruction_step.hpp"
#include "tools/hybrid/m68k_002d66_instruction_step.hpp"
#include "tools/hybrid/m68k_061032_instruction_step.hpp"
#include "tools/hybrid/m68k_0086b4_instruction_step.hpp"
#include "tools/hybrid/m68k_00f740_instruction_step.hpp"
#include "tools/hybrid/m68k_008950_instruction_step.hpp"
#include "tools/hybrid/m68k_00c012_instruction_step.hpp"
#include "tools/hybrid/m68k_0610c8_instruction_step.hpp"
#include "tools/hybrid/m68k_0008a2_instruction_step.hpp"
#include "tools/hybrid/m68k_0008b6_instruction_step.hpp"
#include "tools/hybrid/m68k_017ff2_instruction_step.hpp"
#include "tools/hybrid/m68k_061054_instruction_step.hpp"
#include "tools/hybrid/m68k_06121a_instruction_step.hpp"
#include "tools/hybrid/m68k_060fb2_instruction_step.hpp"
#include "tools/hybrid/m68k_002d58_instruction_step.hpp"
#include "tools/hybrid/m68k_0063da_instruction_step.hpp"
#include "tools/hybrid/m68k_00638c_instruction_step.hpp"
#include "tools/hybrid/m68k_00297c_instruction_step.hpp"
#include "tools/hybrid/m68k_01a2ac_instruction_step.hpp"
#include "tools/hybrid/m68k_00e4d0_instruction_step.hpp"
#include "tools/hybrid/m68k_0102be_instruction_step.hpp"
#include "tools/hybrid/m68k_01027c_instruction_step.hpp"
#include "tools/hybrid/m68k_00b9ec_instruction_step.hpp"
#include "tools/hybrid/m68k_0082ae_instruction_step.hpp"
#include "tools/hybrid/m68k_008cdc_instruction_step.hpp"
#include "tools/hybrid/m68k_008c5a_instruction_step.hpp"
#include "tools/hybrid/m68k_062156_instruction_step.hpp"
#include "tools/hybrid/m68k_00da72_instruction_step.hpp"
#include "tools/hybrid/m68k_001568_instruction_step.hpp"
#include "tools/hybrid/m68k_003064_instruction_step.hpp"
#include "tools/hybrid/m68k_02edca_instruction_step.hpp"
#include "tools/hybrid/m68k_02eea0_instruction_step.hpp"
#include "tools/hybrid/m68k_001d30_instruction_step.hpp"
#include "tools/hybrid/m68k_03ac92_instruction_step.hpp"
#include "tools/hybrid/m68k_016168_instruction_step.hpp"
#include "tools/hybrid/m68k_016a46_instruction_step.hpp"
#include "tools/hybrid/m68k_0042a6_instruction_step.hpp"
#include "tools/hybrid/m68k_03c454_instruction_step.hpp"
#include "tools/hybrid/m68k_03c1e4_instruction_step.hpp"
#include "tools/hybrid/m68k_03c1e6_instruction_step.hpp"
#include "tools/hybrid/m68k_0100ee_instruction_step.hpp"
#include "tools/hybrid/m68k_00e66e_instruction_step.hpp"
#include "tools/hybrid/m68k_015c4a_instruction_step.hpp"
#include "tools/hybrid/m68k_00f942_instruction_step.hpp"
#include "tools/hybrid/m68k_00e6ba_instruction_step.hpp"
#include "tools/hybrid/m68k_0040a6_instruction_step.hpp"
#include "tools/hybrid/m68k_00d266_instruction_step.hpp"
#include "tools/hybrid/m68k_02e30e_instruction_step.hpp"
#include "tools/hybrid/m68k_03c584_instruction_step.hpp"
#include "tools/hybrid/m68k_00b730_instruction_step.hpp"
#include "tools/hybrid/m68k_03b832_instruction_step.hpp"
#include "tools/hybrid/m68k_002b8a_instruction_step.hpp"
#include "tools/hybrid/m68k_002ba0_instruction_step.hpp"
#include "tools/hybrid/m68k_002b6e_instruction_step.hpp"
#include "tools/hybrid/m68k_03c956_instruction_step.hpp"
#include "tools/hybrid/m68k_0007c4_instruction_step.hpp"
#include "tools/hybrid/m68k_002cbc_instruction_step.hpp"
#include "tools/hybrid/m68k_00d962_instruction_step.hpp"
#include "tools/hybrid/m68k_007198_instruction_step.hpp"
#include "tools/hybrid/m68k_03ac6e_instruction_step.hpp"
#include "tools/hybrid/m68k_03ac16_instruction_step.hpp"
#include "tools/hybrid/m68k_03ab98_instruction_step.hpp"
#include "tools/hybrid/m68k_03abda_instruction_step.hpp"
#include "tools/hybrid/m68k_00e36e_instruction_step.hpp"
#include "tools/hybrid/m68k_03ace4_instruction_step.hpp"
#include "tools/hybrid/m68k_0062e4_instruction_step.hpp"
#include "tools/hybrid/m68k_00663e_instruction_step.hpp"
#include "tools/hybrid/m68k_006516_instruction_step.hpp"
#include "tools/hybrid/m68k_007074_instruction_step.hpp"
#include "tools/hybrid/m68k_006fe4_instruction_step.hpp"
#include "tools/hybrid/m68k_00701a_instruction_step.hpp"
#include "tools/hybrid/m68k_0070f8_instruction_step.hpp"
#include "tools/hybrid/m68k_00e2a2_instruction_step.hpp"
#include "tools/hybrid/m68k_0083f8_instruction_step.hpp"
#include "tools/hybrid/m68k_00b610_instruction_step.hpp"
#include "tools/hybrid/m68k_00b66a_instruction_step.hpp"
#include "tools/hybrid/m68k_00b45c_instruction_step.hpp"
#include "tools/hybrid/m68k_00302e_instruction_step.hpp"
#include "tools/hybrid/m68k_00f0d2_instruction_step.hpp"
#include "tools/hybrid/m68k_00d168_instruction_step.hpp"
#include "tools/hybrid/m68k_00dbca_instruction_step.hpp"
#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <array>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string_view>

namespace oasis::hybrid {
namespace {
using ReadRegister = std::uint32_t (*)(unsigned);
using WriteRegister = void (*)(unsigned, unsigned);
using PeekMemory = int (*)(unsigned);
using ReadMemory = std::uint32_t (*)(unsigned, int);
using WriteMemory = void (*)(unsigned, int, unsigned);
using Fetch16 = std::uint32_t (*)();
using SetReturn = void (*)(unsigned, unsigned, unsigned);
using Instruction = void (*)(unsigned);
using AddCycles = void (*)(int);
using BranchAdjustment = int (*)();
using InstallBlock = void (*)(int (*)(unsigned));
using SkipBusRefresh = void (*)();
using SchedulerField = int (*)(unsigned);
using TransferSourcePc = unsigned (*)();

ReadRegister g_read_register{};
WriteRegister g_write_register{};
PeekMemory g_peek{};
ReadMemory g_read{};
WriteMemory g_write{};
Fetch16 g_fetch16{};
SetReturn g_set_return{};
Instruction g_begin{};
Instruction g_finish{};
AddCycles g_add_cycles{};
BranchAdjustment g_branch_adjustment{};
BranchAdjustment g_byte_branch_adjustment{};
SkipBusRefresh g_skip_bus_refresh{};
SchedulerField g_scheduler_field{};
TransferSourcePc g_transfer_source_pc{};
void (*g_observe)(unsigned, unsigned){};
void (*g_fault)(unsigned){};
std::array<bool, 222> g_enabled{};

std::uint32_t rr(void*, unsigned index) { return g_read_register(index); }
void wr(void*, unsigned index, std::uint32_t value) { g_write_register(index, value); }
int peek(void*, unsigned address) { return g_peek(address); }
std::uint32_t read(void*, unsigned address, int width) { return g_read(address, width); }
void write(void*, unsigned address, int width, std::uint32_t value) { g_write(address, width, value); }
std::uint32_t fetch16(void*) { return g_fetch16(); }
void set_return(void*, unsigned pc, unsigned pref_addr, unsigned pref_data) {
    g_set_return(pc, pref_addr, pref_data);
}
void begin(void*, unsigned opcode) { g_begin(opcode); }
void finish(void*, unsigned opcode) { g_finish(opcode); }
void add_cycles(void*, int cycles) { g_add_cycles(cycles); }
void skip_bus_refresh(void*) { g_skip_bus_refresh(); }
int scheduler_field(void*, unsigned field) { return g_scheduler_field(field); }
unsigned transfer_source_pc(void*) { return g_transfer_source_pc ? g_transfer_source_pc() : 0U; }
int branch_adjust(void*) { return g_branch_adjustment(); }
int byte_branch_adjust(void*) { return g_byte_branch_adjustment(); }
void observe(void*, unsigned pc, unsigned opcode) { g_observe(pc, opcode); }

int block_hook(unsigned pc) {
    const M68k9D70StepApi api{nullptr, rr, wr, peek, read, write, fetch16, set_return,
        begin, finish, add_cycles, branch_adjust, byte_branch_adjust, observe,
        skip_bus_refresh, scheduler_field, transfer_source_pc};
    auto result = dispatch_m68k_step_slots_legacy(pc, api, g_enabled.data());
    if (result == M68k9D70StepResult::ContractFault) {
        std::cerr << "CONTRACT_FAULT in legacy table at pc=0x" << std::hex << pc << std::dec << '\n';
    }
#define CHK(fn, idx) if (result == M68k9D70StepResult::NotHandled) { \
        result = fn(pc, api, g_enabled[idx]); \
        if (result == M68k9D70StepResult::ContractFault) { \
            std::cerr << "CONTRACT_FAULT in " #fn " at pc=0x" << std::hex << pc << std::dec << '\n'; \
        } \
    }
    CHK(step_m68k_0099d6, 101);
    CHK(step_m68k_009678, 102);
    CHK(step_m68k_0095aa, 103);
    CHK(step_m68k_009ad0, 104);
    CHK(step_m68k_009d00, 105);
    CHK(step_m68k_0103b0, 106);
    CHK(step_m68k_010382, 107);
    CHK(step_m68k_00e398, 108);
    CHK(step_m68k_01ad14, 109);
    CHK(step_m68k_00e2d4, 110);
    CHK(step_m68k_06231e, 111);
    CHK(step_m68k_0622d0, 112);
    CHK(step_m68k_0622dc, 113);
    CHK(step_m68k_06230e, 114);
    CHK(step_m68k_0622f4, 115);
    CHK(step_m68k_03b7c0, 116);
    CHK(step_m68k_008d06, 117);
    CHK(step_m68k_008910, 119);
    CHK(step_m68k_00129a, 120);
    CHK(step_m68k_0005ec, 121);
    CHK(step_m68k_00d9be, 122);
    CHK(step_m68k_008cac, 123);
    CHK(step_m68k_008cd0, 124);
    CHK(step_m68k_00d7b0, 125);
    CHK(step_m68k_0101e4, 126);
    CHK(step_m68k_007d8a, 127);
    CHK(step_m68k_0085e2, 128);
    CHK(step_m68k_0604bc, 129);
    CHK(step_m68k_01a288, 130);
    CHK(step_m68k_0062cc, 131);
    CHK(step_m68k_0101d0, 132);
    CHK(step_m68k_01b024, 133);
    CHK(step_m68k_0085c4, 134);
    CHK(step_m68k_01a21e, 135);
    CHK(step_m68k_01ae7e, 136);
    CHK(step_m68k_01af3a, 137);
    CHK(step_m68k_00c90e, 138);
    CHK(step_m68k_00cecc, 139);
    CHK(step_m68k_010086, 140);
    CHK(step_m68k_00f920, 141);
    CHK(step_m68k_002d66, 142);
    CHK(step_m68k_061032, 143);
    CHK(step_m68k_0086b4, 144);
    CHK(step_m68k_00f740, 145);
    CHK(step_m68k_008950, 146);
    CHK(step_m68k_00c012, 147);
    CHK(step_m68k_0610c8, 148);
    CHK(step_m68k_0008a2, 149);
    CHK(step_m68k_0008b6, 150);
    CHK(step_m68k_017ff2, 151);
    CHK(step_m68k_061054, 152);
    CHK(step_m68k_06121a, 153);
    CHK(step_m68k_060fb2, 154);
    CHK(step_m68k_002d58, 155);
    CHK(step_m68k_0063da, 156);
    CHK(step_m68k_00638c, 157);
    CHK(step_m68k_00297c, 158);
    CHK(step_m68k_01a2ac, 159);
    CHK(step_m68k_00e4d0, 160);
    CHK(step_m68k_0102be, 161);
    CHK(step_m68k_01027c, 162);
    CHK(step_m68k_00b9ec, 163);
    CHK(step_m68k_0082ae, 164);
    CHK(step_m68k_008cdc, 165);
    CHK(step_m68k_008c5a, 166);
    CHK(step_m68k_062156, 167);
    CHK(step_m68k_00da72, 168);
    CHK(step_m68k_001568, 169);
    CHK(step_m68k_003064, 170);
    CHK(step_m68k_02edca, 171);
    CHK(step_m68k_02eea0, 172);
    CHK(step_m68k_001d30, 173);
    CHK(step_m68k_03ac92, 174);
    CHK(step_m68k_016168, 175);
    CHK(step_m68k_016a46, 176);
    CHK(step_m68k_03c454, 178);
    CHK(step_m68k_03c1e4, 179);
    CHK(step_m68k_03c1e6, 180);
    CHK(step_m68k_0100ee, 181);
    CHK(step_m68k_00e66e, 182);
    CHK(step_m68k_015c4a, 183);
    CHK(step_m68k_00f942, 184);
    CHK(step_m68k_00e6ba, 185);
    CHK(step_m68k_0040a6, 186);
    CHK(step_m68k_00d266, 187);
    CHK(step_m68k_02e30e, 188);
    CHK(step_m68k_03c584, 189);
    CHK(step_m68k_00b730, 190);
    CHK(step_m68k_03b832, 191);
    CHK(step_m68k_002b8a, 192);
    CHK(step_m68k_002ba0, 193);
    CHK(step_m68k_002b6e, 194);
    CHK(step_m68k_03c956, 195);
    CHK(step_m68k_0007c4, 196);
    CHK(step_m68k_002cbc, 197);
    CHK(step_m68k_00d962, 198);
    CHK(step_m68k_007198, 199);
    CHK(step_m68k_03ac6e, 200);
    CHK(step_m68k_03ac16, 201);
    CHK(step_m68k_03ab98, 202);
    CHK(step_m68k_03abda, 203);
    CHK(step_m68k_00e36e, 204);
    CHK(step_m68k_03ace4, 205);
    CHK(step_m68k_0062e4, 206);
    CHK(step_m68k_00663e, 207);
    CHK(step_m68k_006516, 208);
    CHK(step_m68k_007074, 209);
    CHK(step_m68k_006fe4, 210);
    CHK(step_m68k_00701a, 211);
    CHK(step_m68k_0070f8, 212);
    CHK(step_m68k_00e2a2, 213);
    CHK(step_m68k_0083f8, 214);
    CHK(step_m68k_00b610, 215);
    CHK(step_m68k_00b66a, 216);
    CHK(step_m68k_00b45c, 217);
    CHK(step_m68k_00302e, 218);
    CHK(step_m68k_00f0d2, 219);
    CHK(step_m68k_00d168, 220);
    CHK(step_m68k_00dbca, 221);
#undef CHK
    if (result == M68k9D70StepResult::ContractFault) { if (g_fault) g_fault(pc); return 1; }
    return result == M68k9D70StepResult::InstructionStepped ? 1 : 0;
}
} // namespace

void install_m68k_gameplay_step_set(ExternalLibrary& core, std::string_view enabled_helpers,
                                    void (*observer)(unsigned, unsigned),
                                    void (*fault_handler)(unsigned),
                                    unsigned (*source_pc_reader)()) {
    g_read_register = core.get<ReadRegister>("retro_9c40_capture_register");
    g_write_register = core.get<WriteRegister>("retro_9c40_capture_set_register");
    g_peek = core.get<PeekMemory>("retro_9c40_capture_peek");
    g_read = core.get<ReadMemory>("retro_hybrid_read");
    g_write = core.get<WriteMemory>("retro_hybrid_write");
    g_fetch16 = core.get<Fetch16>("retro_hybrid_fetch16");
    g_set_return = core.get<SetReturn>("retro_hybrid_set_return_state");
    g_begin = core.get<Instruction>("retro_hybrid_begin_instruction");
    g_finish = core.get<Instruction>("retro_hybrid_finish_instruction");
    g_add_cycles = core.get<AddCycles>("retro_hybrid_add_cycles");
    g_branch_adjustment = core.get<BranchAdjustment>("retro_hybrid_bcc_not_taken_word_adjustment");
    g_byte_branch_adjustment = core.get<BranchAdjustment>("retro_hybrid_bcc_not_taken_adjustment");
    g_skip_bus_refresh = core.get<SkipBusRefresh>("retro_hybrid_skip_bus_refresh");
    g_scheduler_field = core.get<SchedulerField>("retro_hybrid_scheduler_field");
    g_transfer_source_pc = source_pc_reader;
    g_observe = observer; g_fault = fault_handler;
    g_enabled.fill(false);
    populate_m68k_step_slots_legacy(enabled_helpers, g_enabled.data());
    const std::array<bool, 121> tail_vals{{
        step_helper_selected(enabled_helpers, "0099d6"), step_helper_selected(enabled_helpers, "009678"), step_helper_selected(enabled_helpers, "0095aa"),
        step_helper_selected(enabled_helpers, "009ad0"), step_helper_selected(enabled_helpers, "009d00"), step_helper_selected(enabled_helpers, "0103b0"), step_helper_selected(enabled_helpers, "010382"),
        step_helper_selected(enabled_helpers, "00e398"), step_helper_selected(enabled_helpers, "01ad14"), step_helper_selected(enabled_helpers, "00e2d4"), step_helper_selected(enabled_helpers, "06231e"),
        step_helper_selected(enabled_helpers, "0622d0"), step_helper_selected(enabled_helpers, "0622dc"), step_helper_selected(enabled_helpers, "06230e"), step_helper_selected(enabled_helpers, "0622f4"),
        step_helper_selected(enabled_helpers, "03b7c0"), step_helper_selected(enabled_helpers, "008d06"), false /* 00da2a timing rejected */, step_helper_selected(enabled_helpers, "008910"),
        step_helper_selected(enabled_helpers, "00129a"), step_helper_selected(enabled_helpers, "0005ec"), step_helper_selected(enabled_helpers, "00d9be"), step_helper_selected(enabled_helpers, "008cac"),
        step_helper_selected(enabled_helpers, "008cd0"), step_helper_selected(enabled_helpers, "00d7b0"), step_helper_selected(enabled_helpers, "0101e4"), step_helper_selected(enabled_helpers, "007d8a"),
        step_helper_selected(enabled_helpers, "0085e2"), step_helper_selected(enabled_helpers, "0604bc"), step_helper_selected(enabled_helpers, "01a288"), step_helper_selected(enabled_helpers, "0062cc"),
        step_helper_selected(enabled_helpers, "0101d0"), step_helper_selected(enabled_helpers, "01b024"), step_helper_selected(enabled_helpers, "0085c4"), step_helper_selected(enabled_helpers, "01a21e"),
        step_helper_selected(enabled_helpers, "01ae7e"), step_helper_selected(enabled_helpers, "01af3a"), step_helper_selected(enabled_helpers, "00c90e"), step_helper_selected(enabled_helpers, "00cecc"),
        step_helper_selected(enabled_helpers, "010086"), step_helper_selected(enabled_helpers, "00f920"), step_helper_selected(enabled_helpers, "002d66"), step_helper_selected(enabled_helpers, "061032"),
        step_helper_selected(enabled_helpers, "0086b4"), step_helper_selected(enabled_helpers, "00f740"), step_helper_selected(enabled_helpers, "008950"), false /* 00c012 timing rejected */,
        step_helper_selected(enabled_helpers, "0610c8"), step_helper_selected(enabled_helpers, "0008a2"), step_helper_selected(enabled_helpers, "0008b6"), false /* 017ff2 timing rejected */,
        step_helper_selected(enabled_helpers, "061054"),
        step_helper_selected(enabled_helpers, "06121a"), step_helper_selected(enabled_helpers, "060fb2"), step_helper_selected(enabled_helpers, "002d58"), step_helper_selected(enabled_helpers, "0063da"),
        step_helper_selected(enabled_helpers, "00638c"), step_helper_selected(enabled_helpers, "00297c"),
        step_helper_selected(enabled_helpers, "01a2ac"), step_helper_selected(enabled_helpers, "00e4d0"),
        step_helper_selected(enabled_helpers, "0102be"), step_helper_selected(enabled_helpers, "01027c"),
        step_helper_selected(enabled_helpers, "00b9ec"), step_helper_selected(enabled_helpers, "0082ae"),
        step_helper_selected(enabled_helpers, "008cdc"), step_helper_selected(enabled_helpers, "008c5a"),
        step_helper_selected(enabled_helpers, "062156"),
        step_helper_selected(enabled_helpers, "00da72"), step_helper_selected(enabled_helpers, "001568"),
        step_helper_selected(enabled_helpers, "003064"), step_helper_selected(enabled_helpers, "02edca"),
        step_helper_selected(enabled_helpers, "02eea0"), step_helper_selected(enabled_helpers, "001d30"),
        step_helper_selected(enabled_helpers, "03ac92"),
        step_helper_selected(enabled_helpers, "016168"),
        step_helper_selected(enabled_helpers, "016a46"),
        false /* 0042a6 timing deferred */,
        step_helper_selected(enabled_helpers, "03c454"),
        step_helper_selected(enabled_helpers, "03c1e4"),
        step_helper_selected(enabled_helpers, "03c1e6"),
        step_helper_selected(enabled_helpers, "0100ee"),
        step_helper_selected(enabled_helpers, "00e66e"),
        step_helper_selected(enabled_helpers, "015c4a"),
        step_helper_selected(enabled_helpers, "00f942"),
        false /* 00e6ba audit candidate */,
        false /* 0040a6 audit candidate */,
        false /* 00d266 audit candidate */,
        step_helper_selected(enabled_helpers, "02e30e"),
        step_helper_selected(enabled_helpers, "03c584"),
        step_helper_selected(enabled_helpers, "00b730"),
        step_helper_selected(enabled_helpers, "03b832"),
        step_helper_selected(enabled_helpers, "002b8a"),
        step_helper_selected(enabled_helpers, "002ba0"),
        step_helper_selected(enabled_helpers, "002b6e"),
        step_helper_selected(enabled_helpers, "03c956"),
        false /* 0007c4 timing/SR deferred */,
        false /* 002cbc hardware conflict deferred */,
        step_helper_selected(enabled_helpers, "00d962"),
        step_helper_selected(enabled_helpers, "007198"),
        step_helper_selected(enabled_helpers, "03ac6e"),
        step_helper_selected(enabled_helpers, "03ac16"),
        step_helper_selected(enabled_helpers, "03ab98"),
        step_helper_selected(enabled_helpers, "03abda"),
        step_helper_selected(enabled_helpers, "00e36e"),
        step_helper_selected(enabled_helpers, "03ace4"),
        step_helper_selected(enabled_helpers, "0062e4"),
        step_helper_selected(enabled_helpers, "00663e"),
        step_helper_selected(enabled_helpers, "006516"),
        step_helper_selected(enabled_helpers, "007074"),
        step_helper_selected(enabled_helpers, "006fe4"),
        step_helper_selected(enabled_helpers, "00701a"),
        step_helper_selected(enabled_helpers, "0070f8"),
        step_helper_selected(enabled_helpers, "00e2a2"),
        false /* 0083f8 timing cycle sensitive hold */,
        step_helper_selected(enabled_helpers, "00b610"),
        step_helper_selected(enabled_helpers, "00b66a"),
        false /* 00b45c experimental - awaiting oracle acceptance authorization */,
        step_helper_selected(enabled_helpers, "00302e"),
        step_helper_selected(enabled_helpers, "00f0d2"),
        step_helper_selected(enabled_helpers, "00d168"),
        step_helper_selected(enabled_helpers, "00dbca")
    }};
    for (std::size_t i = 0; i < tail_vals.size(); ++i) g_enabled[101U + i] = tail_vals[i];
    bool any_enabled = false;
    for (bool b : g_enabled) any_enabled = any_enabled || b;
    if (!any_enabled || !g_observe || !g_fault)
        throw std::runtime_error("instruction step set is empty or lacks callbacks");
    core.get<InstallBlock>("retro_hybrid_install_block")(block_hook);
}

} // namespace oasis::hybrid
