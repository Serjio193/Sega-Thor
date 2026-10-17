#include "tools/hybrid/m68k_00e0ba_instruction_step.hpp"
#include "tools/hybrid/m68k_00e0ba_step_ops.hpp"

#include <array>
#include <fstream>

namespace oasis::hybrid {
namespace {

using namespace step_00e0ba;

inline constexpr unsigned kStart = 0x00E0BAU;
constexpr unsigned kMask24 = 0x00FFFFFFU;

bool read_be(const M68k9D70StepApi& api, unsigned addr, unsigned bytes, unsigned& out) noexcept {
    out = 0;
    for (unsigned i = 0; i < bytes; ++i) {
        out = (out << 8U) | static_cast<std::uint8_t>(api.read(api.context, (addr + i) & kMask24, 1));
    }
    return true;
}

struct ProvenCaller {
    unsigned caller_pc{};
    unsigned return_pc{};
    std::int16_t displacement{};
};

constexpr std::array<ProvenCaller, 1> kCallers = {{
    {0x008B36U, 0x008B3AU, static_cast<std::int16_t>(0x5582)}
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
M68k00E0BAStepMetrics metrics{};

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

M68k9D70StepResult step_m68k_00e0ba(unsigned pc, const M68k9D70StepApi& api,
                                    bool enabled) noexcept {
    if (!enabled) return M68k9D70StepResult::NotHandled;

    if (active && child_call_active) {
        const unsigned sp = api.read_register(api.context, 15U) & kMask24;
        if (sp == saved_child_sp && pc == 0x00E0E6U) {
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

        if (caller->caller_pc == 0x008B36U) {
            metrics.caller_008b36_calls++;
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
    case 0x00E0BAU:
        res = step_btst_imm_abs(api, pc, 1U, 0x00FF17B8U, 0x00E0C2U, resume_pc);
        break;
    case 0x00E0C2U:
        res = step_bne_w(api, pc, 0x00E0F2U, 0x00E0C6U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped && resume_pc == 0x00E0F2U) {
            inhibit_exit = true;
        }
        break;
    case 0x00E0C6U:
        res = step_lea_abs_reg(api, pc, 0x00FF2D8CU, 6U, 0x00E0CCU, resume_pc);
        break;
    case 0x00E0CCU:
        res = step_moveq(api, pc, 5, 7U, 0x00E0CEU, resume_pc);
        break;
    case 0x00E0CEU:
        res = step_move_w_disp_reg(api, pc, 6U, 0, 6U, 0x00E0D2U, resume_pc);
        break;
    case 0x00E0D2U:
        res = step_beq_w(api, pc, 0x00E0EAU, 0x00E0D6U, resume_pc);
        break;
    case 0x00E0D6U:
        res = step_move_l_disp_reg(api, pc, 6U, 86, 6U, 0x00E0DAU, resume_pc);
        break;
    case 0x00E0DAU:
        res = step_beq_w(api, pc, 0x00E0EAU, 0x00E0DEU, resume_pc);
        break;
    case 0x00E0DEU:
        res = step_movea_l_reg_reg(api, pc, 6U, 0U, 0x00E0E0U, resume_pc);
        break;
    case 0x00E0E0U:
        res = step_movem_l_push_d7_a6(api, pc, 0x00E0E4U, resume_pc);
        break;
    case 0x00E0E4U:
        res = step_jsr_a0(api, pc, 0x00E0E6U, resume_pc, child_call_active, saved_child_sp);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.child_jsr_calls++;
        }
        break;
    case 0x00E0E6U:
        res = step_movem_l_pop_d7_a6(api, pc, 0x00E0EAU, resume_pc);
        break;
    case 0x00E0EAU:
        res = step_lea_disp_reg(api, pc, 6U, 90, 0x00E0EEU, resume_pc);
        break;
    case 0x00E0EEU:
        res = step_dbf_d7(api, pc, 0x00E0CEU, 0x00E0F2U, resume_pc);
        break;
    case 0x00E0F2U:
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

const M68k00E0BAStepMetrics& m68k_00e0ba_metrics() noexcept {
    return metrics;
}

void write_00e0ba_step_metrics(const std::filesystem::path& path) {
    std::ofstream ofs(path);
    if (!ofs) return;
    ofs << "{\n"
        << "  \"target_entries\": " << metrics.target_entries << ",\n"
        << "  \"native_calls\": " << metrics.native_calls << ",\n"
        << "  \"rom_fallback_calls\": " << metrics.rom_fallback_calls << ",\n"
        << "  \"completed_calls\": " << metrics.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << metrics.stepped_instructions << ",\n"
        << "  \"caller_008b36_calls\": " << metrics.caller_008b36_calls << ",\n"
        << "  \"caller_other_calls\": " << metrics.caller_other_calls << ",\n"
        << "  \"active_at_entry\": " << metrics.active_at_entry << ",\n"
        << "  \"inhibit_exit_calls\": " << metrics.inhibit_exit_calls << ",\n"
        << "  \"loop_completed_calls\": " << metrics.loop_completed_calls << ",\n"
        << "  \"child_jsr_calls\": " << metrics.child_jsr_calls << "\n"
        << "}\n";
}

void reset_m68k_00e0ba_state() noexcept {
    active = false;
    interrupted = false;
    child_call_active = false;
    saved_child_sp = 0U;
    resume_pc = 0U;
    return_caller_pc = 0U;
    metrics = {};
}

} // namespace oasis::hybrid
