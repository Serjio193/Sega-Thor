#include "tools/hybrid/m68k_00e0f4_instruction_step.hpp"
#include "tools/hybrid/m68k_00e0f4_step_ops.hpp"

#include <array>
#include <fstream>

namespace oasis::hybrid {
namespace {

using namespace step_00e0f4;

inline constexpr unsigned kStart = 0x00E0F4U;
constexpr unsigned kMask24 = 0x00FFFFFFU;

bool read_be(const M68k9D70StepApi& api, unsigned addr, int width, unsigned& val) noexcept {
    val = 0U;
    for (int i = 0; i < width; ++i) {
        const int byte_val = api.peek(api.context, (addr + static_cast<unsigned>(i)) & kMask24);
        if (byte_val < 0) return false;
        val = (val << 8U) | static_cast<unsigned>(byte_val & 0xFF);
    }
    return true;
}

struct ProvenCaller {
    unsigned caller_pc{};
    unsigned return_pc{};
    std::int16_t displacement{};
};

constexpr std::array<ProvenCaller, 1> kCallers = {{
    {0x008B3AU, 0x008B3EU, static_cast<std::int16_t>(0x55B8)}
}};

bool in_work_ram(unsigned addr, unsigned size) noexcept {
    return addr >= 0x00FF0000U && (addr + size) <= 0x01000000U;
}

bool active = false;
bool interrupted = false;
bool child_call_active = false;
bool inhibit_exit = false;
unsigned saved_child_sp = 0U;
unsigned resume_pc = 0U;
unsigned return_caller_pc = 0U;
M68k00E0F4StepMetrics metrics{};

const ProvenCaller* find_caller(const M68k9D70StepApi& api, unsigned sp) noexcept {
    unsigned return_pc{};
    if (!in_work_ram(sp, 4U) || sp < 0x00FF0004U || (sp & 1U) != 0U ||
        !read_be(api, sp, 4, return_pc)) return nullptr;
    const unsigned source_pc = api.read_transfer_source_pc ?
        (api.read_transfer_source_pc(api.context) & kMask24) : 0U;
    for (const auto& caller : kCallers) {
        if (return_pc != caller.return_pc) continue;
        if (source_pc != 0U && source_pc != caller.caller_pc) continue;
        unsigned opcode{}, ext{};
        if (read_be(api, caller.caller_pc, 2, opcode) && opcode == 0x6100U &&
            read_be(api, caller.caller_pc + 2U, 2, ext) &&
            static_cast<std::int16_t>(ext) == caller.displacement) {
            return &caller;
        }
    }
    return nullptr;
}

} // namespace

M68k9D70StepResult step_m68k_00e0f4(unsigned pc, const M68k9D70StepApi& api,
                                    bool enabled) noexcept {
    if (!enabled) return M68k9D70StepResult::NotHandled;

    if (active && child_call_active) {
        const unsigned sp = api.read_register(api.context, 15U) & kMask24;
        if (sp == saved_child_sp && pc == 0x00E132U) {
            child_call_active = false;
            resume_pc = pc;
        } else {
            return M68k9D70StepResult::NotHandled;
        }
    }

    if (active && pc != resume_pc) {
        interrupted = true;
        return M68k9D70StepResult::NotHandled;
    }
    if (interrupted && pc == resume_pc) {
        interrupted = false;
    }

    if (pc == kStart) {
        if (active) {
            metrics.active_at_entry++;
            active = false;
            interrupted = false;
            child_call_active = false;
            saved_child_sp = 0U;
            return M68k9D70StepResult::NotHandled;
        }
        if (!verify_rom_words(api, kStart)) return M68k9D70StepResult::NotHandled;

        const unsigned sp = api.read_register(api.context, 15U) & kMask24;
        const auto* caller = find_caller(api, sp);
        if (!caller) {
            metrics.caller_other_calls++;
            metrics.rom_fallback_calls++;
            return M68k9D70StepResult::NotHandled;
        }

        if (caller->caller_pc == 0x008B3AU) {
            metrics.caller_008b3a_calls++;
        }

        metrics.target_entries++;
        metrics.native_calls++;
        active = true;
        interrupted = false;
        child_call_active = false;
        inhibit_exit = false;
        saved_child_sp = 0U;
        return_caller_pc = caller->return_pc;
    }

    if (!active) return M68k9D70StepResult::NotHandled;

    M68k9D70StepResult res = M68k9D70StepResult::NotHandled;

    switch (pc) {
    case 0x00E0F4U:
        res = step_move_w_abs_reg(api, pc, 0x00FF1856U, 6U, 0x00E0FAU, resume_pc);
        break;
    case 0x00E0FAU:
        res = step_beq_w(api, pc, 0x00E106U, 0x00E0FEU, resume_pc);
        break;
    case 0x00E0FEU:
        res = step_subq_w_1_reg(api, pc, 6U, 0x00E100U, resume_pc);
        break;
    case 0x00E100U:
        res = step_move_w_reg_abs(api, pc, 6U, 0x00FF1856U, 0x00E106U, resume_pc);
        break;
    case 0x00E106U:
        res = step_btst_imm_abs(api, pc, 1U, 0x00FF17B8U, 0x00E10EU, resume_pc);
        break;
    case 0x00E10EU:
        res = step_bne_w(api, pc, 0x00E13EU, 0x00E112U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped && resume_pc == 0x00E13EU) {
            inhibit_exit = true;
        }
        break;
    case 0x00E112U:
        res = step_lea_abs_reg(api, pc, 0x00FF2ABCU, 6U, 0x00E118U, resume_pc);
        break;
    case 0x00E118U:
        res = step_moveq(api, pc, 7, 7U, 0x00E11AU, resume_pc);
        break;
    case 0x00E11AU:
        res = step_move_w_disp_reg(api, pc, 6U, 0, 6U, 0x00E11EU, resume_pc);
        break;
    case 0x00E11EU:
        res = step_ble_w(api, pc, 0x00E136U, 0x00E122U, resume_pc);
        break;
    case 0x00E122U:
        res = step_move_w_disp_reg(api, pc, 6U, 4, 6U, 0x00E126U, resume_pc);
        break;
    case 0x00E126U:
        res = step_lea_pc_index_reg(api, pc, 6U, 0x00E140U, 0U, 0x00E12AU, resume_pc);
        break;
    case 0x00E12AU:
        res = step_adda_w_ind_reg(api, pc, 0U, 0U, 0x00E12CU, resume_pc);
        break;
    case 0x00E12CU:
        res = step_movem_l_push_d7_a6(api, pc, 0x00E130U, resume_pc);
        break;
    case 0x00E130U:
        res = step_jsr_a0(api, pc, 0x00E132U, resume_pc, child_call_active, saved_child_sp);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.child_jsr_calls++;
        }
        break;
    case 0x00E132U:
        res = step_movem_l_pop_d7_a6(api, pc, 0x00E136U, resume_pc);
        break;
    case 0x00E136U:
        res = step_lea_disp_reg(api, pc, 6U, 90, 0x00E13AU, resume_pc);
        break;
    case 0x00E13AU:
        res = step_dbf_d7(api, pc, 0x00E11AU, 0x00E13EU, resume_pc);
        break;
    case 0x00E13EU:
        res = step_rts(api, pc, return_caller_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.completed_calls++;
            if (inhibit_exit) {
                metrics.inhibit_exit_calls++;
            } else {
                metrics.loop_completed_calls++;
            }
            active = false;
            interrupted = false;
            child_call_active = false;
            inhibit_exit = false;
            saved_child_sp = 0U;
        }
        break;
    default:
        return M68k9D70StepResult::NotHandled;
    }

    if (res == M68k9D70StepResult::InstructionStepped) {
        metrics.stepped_instructions++;
    }
    return res;
}

const M68k00E0F4StepMetrics& m68k_00e0f4_metrics() noexcept {
    return metrics;
}

void write_00e0f4_step_metrics(const std::filesystem::path& path) {
    std::ofstream ofs(path);
    if (!ofs) return;
    ofs << "{\n"
        << "  \"target_entries\": " << metrics.target_entries << ",\n"
        << "  \"native_calls\": " << metrics.native_calls << ",\n"
        << "  \"rom_fallback_calls\": " << metrics.rom_fallback_calls << ",\n"
        << "  \"completed_calls\": " << metrics.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << metrics.stepped_instructions << ",\n"
        << "  \"caller_008b3a_calls\": " << metrics.caller_008b3a_calls << ",\n"
        << "  \"caller_other_calls\": " << metrics.caller_other_calls << ",\n"
        << "  \"active_at_entry\": " << metrics.active_at_entry << ",\n"
        << "  \"inhibit_exit_calls\": " << metrics.inhibit_exit_calls << ",\n"
        << "  \"loop_completed_calls\": " << metrics.loop_completed_calls << ",\n"
        << "  \"child_jsr_calls\": " << metrics.child_jsr_calls << "\n"
        << "}\n";
}

void reset_m68k_00e0f4_state() noexcept {
    active = false;
    interrupted = false;
    child_call_active = false;
    saved_child_sp = 0U;
    resume_pc = 0U;
    return_caller_pc = 0U;
    metrics = {};
}

} // namespace oasis::hybrid
