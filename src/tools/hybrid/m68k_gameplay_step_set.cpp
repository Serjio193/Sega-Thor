#include "tools/hybrid/m68k_gameplay_step_set.hpp"

#include "tools/hybrid/external_library.hpp"
#include "tools/hybrid/m68k_613b2_instruction_step.hpp"
#include "tools/hybrid/m68k_61e48_instruction_step.hpp"
#include "tools/hybrid/m68k_61934_instruction_step.hpp"
#include "tools/hybrid/m68k_6147e_instruction_step.hpp"
#include "tools/hybrid/m68k_623ac_instruction_step.hpp"
#include "tools/hybrid/m68k_6273a_instruction_step.hpp"
#include "tools/hybrid/m68k_62788_instruction_step.hpp"
#include "tools/hybrid/m68k_60000_instruction_step.hpp"
#include "tools/hybrid/m68k_60286_instruction_step.hpp"
#include "tools/hybrid/m68k_62ae0_instruction_step.hpp"
#include "tools/hybrid/m68k_626bc_instruction_step.hpp"
#include "tools/hybrid/m68k_0027fc_instruction_step.hpp"
#include "tools/hybrid/m68k_002992_instruction_step.hpp"
#include "tools/hybrid/m68k_6138e_instruction_step.hpp"
#include "tools/hybrid/m68k_00e682_instruction_step.hpp"
#include "tools/hybrid/m68k_0613f8_instruction_step.hpp"
#include "tools/hybrid/m68k_009ad6_instruction_step.hpp"
#include "tools/hybrid/m68k_009bf2_instruction_step.hpp"
#include "tools/hybrid/m68k_0027a6_instruction_step.hpp"
#include "tools/hybrid/m68k_00b15e_instruction_step.hpp"
#include "tools/hybrid/m68k_00a8da_instruction_step.hpp"
#include "tools/hybrid/m68k_017d12_instruction_step.hpp"
#include "tools/hybrid/m68k_6281a_instruction_step.hpp"
#include "tools/hybrid/m68k_6282e_instruction_step.hpp"
#include "tools/hybrid/m68k_016ebc_instruction_step.hpp"
#include "tools/hybrid/m68k_01ac90_instruction_step.hpp"
#include "tools/hybrid/m68k_01b102_instruction_step.hpp"
#include "tools/hybrid/m68k_00350e_instruction_step.hpp"
#include "tools/hybrid/m68k_008e22_instruction_step.hpp"
#include "tools/hybrid/m68k_00a7e6_instruction_step.hpp"
#include "tools/hybrid/m68k_008e90_instruction_step.hpp"
#include "tools/hybrid/m68k_00f788_instruction_step.hpp"
#include "tools/hybrid/m68k_00a196_instruction_step.hpp"
#include "tools/hybrid/m68k_00a342_instruction_step.hpp"
#include "tools/hybrid/m68k_00b34c_instruction_step.hpp"
#include "tools/hybrid/m68k_00c980_instruction_step.hpp"
#include "tools/hybrid/m68k_00c998_instruction_step.hpp"
#include "tools/hybrid/m68k_008dd4_instruction_step.hpp"
#include "tools/hybrid/m68k_94a2_instruction_step.hpp"
#include "tools/hybrid/m68k_8ef8_instruction_step.hpp"
#include "tools/hybrid/m68k_9d_family_instruction_step.hpp"
#include "tools/hybrid/m68k_e268_instruction_step.hpp"
#include "tools/hybrid/m68k_9c40_instruction_step.hpp"
#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <array>
#include <cstdint>
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
std::array<bool, 46> g_enabled{};

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
    std::array<bool, 4> enabled_9d{g_enabled[0], g_enabled[1], g_enabled[2], g_enabled[3]};
    auto result = step_9d_family(pc, api, enabled_9d);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_94a2(pc, api, g_enabled[4]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_9c40(pc, api, g_enabled[5]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_8ef8(pc, api, g_enabled[6]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_e268(pc, api, g_enabled[7]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_613b2(pc, api, g_enabled[8]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_61e48(pc, api, g_enabled[9]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_61934(pc, api, g_enabled[10]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_6147e(pc, api, g_enabled[11]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_623ac(pc, api, g_enabled[12]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_6273a(pc, api, g_enabled[13]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_62788(pc, api, g_enabled[14]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_60000(pc, api, g_enabled[15]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_60286(pc, api, g_enabled[16]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_62ae0(pc, api, g_enabled[17]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_626bc(pc, api, g_enabled[18]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_0027fc(pc, api, g_enabled[19]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_002992(pc, api, g_enabled[20]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_6138e(pc, api, g_enabled[21]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_00e682(pc, api, g_enabled[22]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_0613f8(pc, api, g_enabled[23]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_016ebc(pc, api, g_enabled[24]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_009ad6(pc, api, g_enabled[25]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_009bf2(pc, api, g_enabled[26]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_0027a6(pc, api, g_enabled[27]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_00b15e(pc, api, g_enabled[28]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_00a8da(pc, api, g_enabled[29]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_017d12(pc, api, g_enabled[30]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_6281a(pc, api, g_enabled[31]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_6282e(pc, api, g_enabled[32]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_01ac90(pc, api, g_enabled[33]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_01b102(pc, api, g_enabled[34]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_00350e(pc, api, g_enabled[35]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_008e22(pc, api, g_enabled[36]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_00a7e6(pc, api, g_enabled[37]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_008e90(pc, api, g_enabled[38]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_00f788(pc, api, g_enabled[39]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_00a196(pc, api, g_enabled[40]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_00a342(pc, api, g_enabled[41]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_00b34c(pc, api, g_enabled[42]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_00c980(pc, api, g_enabled[43]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_00c998(pc, api, g_enabled[44]);
    if (result == M68k9D70StepResult::NotHandled) result = step_m68k_008dd4(pc, api, g_enabled[45]);
    if (result == M68k9D70StepResult::ContractFault) { if (g_fault) g_fault(pc); return 1; }
    return result == M68k9D70StepResult::InstructionStepped ? 1 : 0;
}

bool selected(std::string_view list, std::string_view id) {
    std::size_t pos{};
    while (pos < list.size()) {
        const auto end = list.find(',', pos);
        if (list.substr(pos, end == std::string_view::npos ? list.size() - pos : end - pos) == id) return true;
        if (end == std::string_view::npos) break;
        pos = end + 1U;
    }
    return false;
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
    g_enabled = {
        selected(enabled_helpers, "9d70"), selected(enabled_helpers, "9d4e"),
        selected(enabled_helpers, "9db4"), selected(enabled_helpers, "9d92"),
        selected(enabled_helpers, "94a2"), selected(enabled_helpers, "9c40"),
        selected(enabled_helpers, "8ef8"), selected(enabled_helpers, "e268"),
        selected(enabled_helpers, "613b2"), selected(enabled_helpers, "61e48"),
        selected(enabled_helpers, "61934"), selected(enabled_helpers, "6147e"),
        selected(enabled_helpers, "623ac"), selected(enabled_helpers, "6273a"),
        selected(enabled_helpers, "62788"), selected(enabled_helpers, "60000"),
        selected(enabled_helpers, "60286"), selected(enabled_helpers, "62ae0"),
        selected(enabled_helpers, "626bc"), selected(enabled_helpers, "0027fc"),
        selected(enabled_helpers, "002992"),
        (selected(enabled_helpers, "6138e") || selected(enabled_helpers, "06138e")),
        (selected(enabled_helpers, "e682") || selected(enabled_helpers, "00e682")),
        (selected(enabled_helpers, "613f8") || selected(enabled_helpers, "0613f8")),
        (selected(enabled_helpers, "16ebc") || selected(enabled_helpers, "016ebc")),
        (selected(enabled_helpers, "9ad6") || selected(enabled_helpers, "009ad6")),
        (selected(enabled_helpers, "9bf2") || selected(enabled_helpers, "009bf2")),
        (selected(enabled_helpers, "27a6") || selected(enabled_helpers, "0027a6")),
        (selected(enabled_helpers, "b15e") || selected(enabled_helpers, "00b15e")),
        (selected(enabled_helpers, "a8da") || selected(enabled_helpers, "00a8da")),
        (selected(enabled_helpers, "17d12") || selected(enabled_helpers, "017d12")),
        (selected(enabled_helpers, "6281a") || selected(enabled_helpers, "06281a")),
        (selected(enabled_helpers, "6282e") || selected(enabled_helpers, "06282e")),
        (selected(enabled_helpers, "01ac90") || selected(enabled_helpers, "1ac90")),
        (selected(enabled_helpers, "01b102") || selected(enabled_helpers, "1b102")),
        (selected(enabled_helpers, "00350e") || selected(enabled_helpers, "350e") ||
         selected(enabled_helpers, "00350E") || selected(enabled_helpers, "350E")),
        (selected(enabled_helpers, "008e22") || selected(enabled_helpers, "8e22") ||
         selected(enabled_helpers, "008E22") || selected(enabled_helpers, "8E22")),
        (selected(enabled_helpers, "00a7e6") || selected(enabled_helpers, "a7e6") ||
         selected(enabled_helpers, "00A7E6") || selected(enabled_helpers, "A7E6")),
        (selected(enabled_helpers, "008e90") || selected(enabled_helpers, "8e90") ||
         selected(enabled_helpers, "008E90") || selected(enabled_helpers, "8E90")),
        (selected(enabled_helpers, "00f788") || selected(enabled_helpers, "f788") ||
         selected(enabled_helpers, "00F788") || selected(enabled_helpers, "F788")),
        (selected(enabled_helpers, "00a196") || selected(enabled_helpers, "a196") ||
         selected(enabled_helpers, "00A196") || selected(enabled_helpers, "A196")),
        (selected(enabled_helpers, "00a342") || selected(enabled_helpers, "a342") ||
         selected(enabled_helpers, "00A342") || selected(enabled_helpers, "A342")),
        (selected(enabled_helpers, "00b34c") || selected(enabled_helpers, "b34c") ||
         selected(enabled_helpers, "00B34C") || selected(enabled_helpers, "B34C")),
        (selected(enabled_helpers, "00c980") || selected(enabled_helpers, "c980") ||
         selected(enabled_helpers, "00C980") || selected(enabled_helpers, "C980")),
        (selected(enabled_helpers, "00c998") || selected(enabled_helpers, "c998") ||
         selected(enabled_helpers, "00C998") || selected(enabled_helpers, "C998")),
        (selected(enabled_helpers, "008dd4") || selected(enabled_helpers, "8dd4") ||
         selected(enabled_helpers, "008DD4") || selected(enabled_helpers, "8DD4"))
    };
    bool any_enabled = false;
    for (bool b : g_enabled) any_enabled = any_enabled || b;
    if (!any_enabled || !g_observe || !g_fault)
        throw std::runtime_error("instruction step set is empty or lacks callbacks");
    core.get<InstallBlock>("retro_hybrid_install_block")(block_hook);
}

void write_m68k_gameplay_step_metrics(const std::filesystem::path& path) {
    write_e268_step_metrics(path);
    if (!path.empty()) {
        write_613b2_step_metrics(path.parent_path() / "613b2-step-metrics.json");
        write_61e48_step_metrics(path.parent_path() / "61e48-step-metrics.json");
        write_61934_step_metrics(path.parent_path() / "61934-step-metrics.json");
        write_6147e_step_metrics(path.parent_path() / "6147e-step-metrics.json");
        write_623ac_step_metrics(path.parent_path() / "623ac-step-metrics.json");
        write_6273a_step_metrics(path.parent_path() / "6273a-step-metrics.json");
        write_62788_step_metrics(path.parent_path() / "62788-step-metrics.json");
        write_60000_step_metrics(path.parent_path() / "60000-step-metrics.json");
        write_60286_step_metrics(path.parent_path() / "60286-step-metrics.json");
        write_62ae0_step_metrics(path.parent_path() / "62ae0-step-metrics.json");
        write_626bc_step_metrics(path.parent_path() / "626bc-step-metrics.json");
        write_0027fc_step_metrics(path.parent_path() / "0027fc-step-metrics.json");
        write_002992_step_metrics(path.parent_path() / "002992-step-metrics.json");
        write_6138e_step_metrics(path.parent_path() / "6138e-step-metrics.json");
        write_00e682_step_metrics(path.parent_path() / "00e682-step-metrics.json");
        write_0613f8_step_metrics(path.parent_path() / "0613f8-step-metrics.json");
        write_016ebc_step_metrics(path.parent_path() / "016ebc-step-metrics.json");
        write_009ad6_step_metrics(path.parent_path() / "009ad6-step-metrics.json");
        write_009bf2_step_metrics(path.parent_path() / "009bf2-step-metrics.json");
        write_0027a6_step_metrics(path.parent_path() / "0027a6-step-metrics.json");
        write_00b15e_step_metrics(path.parent_path() / "00b15e-step-metrics.json");
        write_00a8da_step_metrics(path.parent_path() / "00a8da-step-metrics.json");
        write_017d12_step_metrics(path.parent_path() / "017d12-step-metrics.json");
        write_6281a_step_metrics(path.parent_path() / "06281a-step-metrics.json");
        write_6282e_step_metrics(path.parent_path() / "06282e-step-metrics.json");
        write_01ac90_step_metrics(path.parent_path() / "01ac90-step-metrics.json");
        write_01b102_step_metrics(path.parent_path() / "01b102-step-metrics.json");
        write_00350e_step_metrics(path.parent_path() / "00350e-step-metrics.json");
        write_008e22_step_metrics(path.parent_path() / "008e22-step-metrics.json");
        write_00a7e6_step_metrics(path.parent_path() / "00a7e6-step-metrics.json");
        write_008e90_step_metrics(path.parent_path() / "008e90-step-metrics.json");
        write_00f788_step_metrics(path.parent_path() / "00f788-step-metrics.json");
        write_00a196_step_metrics(path.parent_path() / "00a196-step-metrics.json");
        write_00a342_step_metrics(path.parent_path() / "00a342-step-metrics.json");
        write_00b34c_step_metrics(path.parent_path() / "00b34c-step-metrics.json");
        write_00c980_step_metrics(path.parent_path() / "00c980-step-metrics.json");
        write_00c998_step_metrics(path.parent_path() / "00c998-step-metrics.json");
        write_008dd4_step_metrics(path.parent_path() / "008dd4-step-metrics.json");
    }
}

} // namespace oasis::hybrid
