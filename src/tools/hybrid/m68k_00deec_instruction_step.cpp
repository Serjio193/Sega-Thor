#include "tools/hybrid/m68k_00deec_instruction_step.hpp"
#include "tools/hybrid/m68k_00deec_step_ops.hpp"

#include <array>
#include <cstdio>
#include <fstream>

namespace oasis::hybrid {
namespace {

using namespace step_00deec;

inline constexpr unsigned kStart = 0x00DEECU;

constexpr std::array<std::uint16_t, 52> kRomWords = {
    0x0839U, 0x0001U, 0x00FFU, 0x17B8U, 0x6600U, 0x005CU, 0x4DF9U, 0x00FFU,
    0x1AA4U, 0x7E13U, 0x3C2EU, 0x0000U, 0x6F00U, 0x0044U, 0x0C46U, 0x0016U,
    0x6500U, 0x003CU, 0xDC46U, 0x207BU, 0x6014U, 0x48E7U, 0x0102U, 0x4E90U,
    0x4CDFU, 0x4080U, 0x4241U, 0x3401U, 0x122EU, 0x0048U, 0x142EU, 0x0049U,
    0x082EU, 0x0000U, 0x0017U, 0x6700U, 0x0004U, 0xC342U, 0x3D41U, 0x0042U,
    0x5242U, 0xE24AU, 0x3202U, 0xE24AU, 0xD441U, 0x3D42U, 0x0044U, 0x4DEEU,
    0x00BCU, 0x51CFU, 0xFFB0U, 0x4E75U,
};

struct ProvenCaller {
    unsigned caller_pc{};
    unsigned return_pc{};
    std::int16_t displacement{};
};

constexpr std::array<ProvenCaller, 1> kCallers = {{
    {0x008B32U, 0x008B36U, static_cast<std::int16_t>(0x53B8)}
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
M68k00DEECStepMetrics metrics{};

bool validate_rom(const M68k9D70StepApi& api) noexcept {
    for (std::size_t i = 0; i < kRomWords.size(); ++i) {
        unsigned word{};
        if (!read_be(api, kStart + static_cast<unsigned>(i) * 2U, 2, word) ||
            word != kRomWords[i]) return false;
    }
    return true;
}

const ProvenCaller* find_caller(const M68k9D70StepApi& api, unsigned sp) noexcept {
    unsigned return_pc{};
    if (!in_work_ram(sp, 4U) || sp < 0x00FF0004U || (sp & 1U) != 0U ||
        !read_be(api, sp, 4U, return_pc)) return nullptr;
    const unsigned source_pc = api.read_transfer_source_pc ?
        (api.read_transfer_source_pc(api.context) & kMask24) : 0U;
    for (const auto& caller : kCallers) {
        if (return_pc != caller.return_pc) continue;
        if (source_pc != 0U && source_pc != caller.caller_pc) continue;
        unsigned opcode{}, ext{};
        if (read_be(api, caller.caller_pc, 2U, opcode) && opcode == 0x6100U &&
            read_be(api, caller.caller_pc + 2U, 2U, ext) &&
            static_cast<std::int16_t>(ext) == caller.displacement) {
            return &caller;
        }
    }
    return nullptr;
}

} // namespace

M68k9D70StepResult step_m68k_00deec(unsigned pc, const M68k9D70StepApi& api,
                                    bool enabled) noexcept {
    if (!enabled) return M68k9D70StepResult::NotHandled;

    if (active && child_call_active) {
        const unsigned sp = api.read_register(api.context, 15U) & kMask24;
        if (sp == saved_child_sp && pc == 0x00DF1CU) {
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
        if (!validate_rom(api)) return M68k9D70StepResult::NotHandled;

        const unsigned sp = api.read_register(api.context, 15U) & kMask24;
        const auto* caller = find_caller(api, sp);
        if (!caller) {
            metrics.caller_other_calls++;
            metrics.rom_fallback_calls++;
            return M68k9D70StepResult::NotHandled;
        }

        if (caller->caller_pc == 0x008B32U) {
            metrics.caller_008b32_calls++;
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
    case 0x00DEECU:
        res = step_btst_imm_abs(api, pc, 1U, 0x00FF17B8U, 0x00DEF4U, resume_pc);
        break;
    case 0x00DEF4U:
        res = step_bne_w(api, pc, 0x00DF52U, 0x00DEF8U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped && resume_pc == 0x00DF52U) {
            inhibit_exit = true;
        }
        break;
    case 0x00DEF8U:
        res = step_lea_abs_reg(api, pc, 0x00FF1AA4U, 6U, 0x00DEFEU, resume_pc);
        break;
    case 0x00DEFEU:
        res = step_moveq(api, pc, 19, 7U, 0x00DF00U, resume_pc);
        break;
    case 0x00DF00U:
        res = step_move_w_disp_reg(api, pc, 6U, 0, 6U, 0x00DF04U, resume_pc);
        break;
    case 0x00DF04U:
        res = step_ble_w(api, pc, 0x00DF4AU, 0x00DF08U, resume_pc);
        break;
    case 0x00DF08U:
        res = step_cmpi_w_reg(api, pc, 22U, 6U, 0x00DF0CU, resume_pc);
        break;
    case 0x00DF0CU:
        res = step_bcs_w(api, pc, 0x00DF4AU, 0x00DF10U, resume_pc);
        break;
    case 0x00DF10U:
        res = step_add_w_reg_reg(api, pc, 6U, 6U, 0x00DF12U, resume_pc);
        break;
    case 0x00DF12U:
        res = step_movea_l_pc_index(api, pc, 0U, 0x00DF16U, resume_pc);
        break;
    case 0x00DF16U:
        res = step_movem_l_push_d7_a6(api, pc, 0x00DF1AU, resume_pc);
        break;
    case 0x00DF1AU:
        res = step_jsr_a0(api, pc, 0x00DF1CU, resume_pc, child_call_active, saved_child_sp);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.child_jsr_calls++;
        }
        break;
    case 0x00DF1CU:
        res = step_movem_l_pop_d7_a6(api, pc, 0x00DF20U, resume_pc);
        break;
    case 0x00DF20U:
        res = step_clr_w_reg(api, pc, 1U, 0x00DF22U, resume_pc);
        break;
    case 0x00DF22U:
        res = step_move_w_reg_reg(api, pc, 1U, 2U, 0x00DF24U, resume_pc);
        break;
    case 0x00DF24U:
        res = step_move_b_disp_reg(api, pc, 6U, 72, 1U, 0x00DF28U, resume_pc);
        break;
    case 0x00DF28U:
        res = step_move_b_disp_reg(api, pc, 6U, 73, 2U, 0x00DF2CU, resume_pc);
        break;
    case 0x00DF2CU:
        res = step_btst_imm_disp(api, pc, 0U, 6U, 23, 0x00DF32U, resume_pc);
        break;
    case 0x00DF32U:
        res = step_beq_w(api, pc, 0x00DF38U, 0x00DF36U, resume_pc);
        break;
    case 0x00DF36U:
        res = step_exg_d1_d2(api, pc, 0x00DF38U, resume_pc);
        break;
    case 0x00DF38U:
        res = step_move_w_reg_disp(api, pc, 1U, 6U, 66, 0x00DF3CU, resume_pc);
        break;
    case 0x00DF3CU:
        res = step_addq_w_reg_1(api, pc, 2U, 0x00DF3EU, resume_pc);
        break;
    case 0x00DF3EU:
        res = step_lsr_w_reg_1(api, pc, 2U, 0x00DF40U, resume_pc);
        break;
    case 0x00DF40U:
        res = step_move_w_reg_reg(api, pc, 2U, 1U, 0x00DF42U, resume_pc);
        break;
    case 0x00DF42U:
        res = step_lsr_w_reg_1(api, pc, 2U, 0x00DF44U, resume_pc);
        break;
    case 0x00DF44U:
        res = step_add_w_reg_reg(api, pc, 1U, 2U, 0x00DF46U, resume_pc);
        break;
    case 0x00DF46U:
        res = step_move_w_reg_disp(api, pc, 2U, 6U, 68, 0x00DF4AU, resume_pc);
        break;
    case 0x00DF4AU:
        res = step_lea_disp_reg(api, pc, 6U, 188, 0x00DF4EU, resume_pc);
        break;
    case 0x00DF4EU:
        res = step_dbf_d7(api, pc, 0x00DF00U, 0x00DF52U, resume_pc);
        break;
    case 0x00DF52U:
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

const M68k00DEECStepMetrics& m68k_00deec_metrics() noexcept {
    return metrics;
}

void write_00deec_step_metrics(const std::filesystem::path& path) {
    std::ofstream ofs(path);
    if (!ofs) return;
    ofs << "{\n"
        << "  \"target_entries\": " << metrics.target_entries << ",\n"
        << "  \"native_calls\": " << metrics.native_calls << ",\n"
        << "  \"rom_fallback_calls\": " << metrics.rom_fallback_calls << ",\n"
        << "  \"completed_calls\": " << metrics.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << metrics.stepped_instructions << ",\n"
        << "  \"caller_008b32_calls\": " << metrics.caller_008b32_calls << ",\n"
        << "  \"caller_other_calls\": " << metrics.caller_other_calls << ",\n"
        << "  \"active_at_entry\": " << metrics.active_at_entry << ",\n"
        << "  \"inhibit_exit_calls\": " << metrics.inhibit_exit_calls << ",\n"
        << "  \"loop_completed_calls\": " << metrics.loop_completed_calls << ",\n"
        << "  \"child_jsr_calls\": " << metrics.child_jsr_calls << "\n"
        << "}\n";
}

void reset_m68k_00deec_state() noexcept {
    active = false;
    interrupted = false;
    child_call_active = false;
    saved_child_sp = 0U;
    resume_pc = 0U;
    return_caller_pc = 0U;
    metrics = {};
}

} // namespace oasis::hybrid
