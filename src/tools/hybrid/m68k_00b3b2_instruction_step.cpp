#include "tools/hybrid/m68k_00b3b2_instruction_step.hpp"
#include "tools/hybrid/m68k_00b3b2_step_ops.hpp"

#include <array>
#include <fstream>

namespace oasis::hybrid {
namespace {

using namespace step_00b3b2;

inline constexpr unsigned kStart = 0x00B3B2U;
constexpr unsigned kMask24 = 0x00FFFFFFU;
constexpr unsigned kRegA6 = 14U;
constexpr unsigned kRegD5 = 5U;

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

constexpr std::array<ProvenCaller, 2> kCallers = {{
    {0x00AD4EU, 0x00AD52U, static_cast<std::int16_t>(0x0662)},
    {0x00B01AU, 0x00B01EU, static_cast<std::int16_t>(0x0396)}
}};

bool in_work_ram(unsigned addr, unsigned size) noexcept {
    return addr >= 0x00FF0000U && (addr + size) <= 0x01000000U;
}

bool active = false;
bool interrupted = false;
bool child_call_active = false;
bool pause_exit = false;
unsigned saved_child_sp = 0U;
unsigned resume_pc = 0U;
unsigned return_caller_pc = 0U;
M68k00B3B2StepMetrics metrics{};

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

M68k9D70StepResult step_m68k_00b3b2(unsigned pc, const M68k9D70StepApi& api,
                                    bool enabled) noexcept {
    if (!enabled) return M68k9D70StepResult::NotHandled;

    if (active && child_call_active) {
        const unsigned sp = api.read_register(api.context, 15U) & kMask24;
        if (sp == saved_child_sp && pc == 0x00B3CEU) {
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

        if (caller->caller_pc == 0x00AD4EU) {
            metrics.caller_00ad4e_calls++;
        } else {
            metrics.caller_other_calls++;
        }

        metrics.target_entries++;
        metrics.native_calls++;
        active = true;
        interrupted = false;
        child_call_active = false;
        pause_exit = false;
        saved_child_sp = 0U;
        return_caller_pc = caller->return_pc;
    }

    if (!active) return M68k9D70StepResult::NotHandled;

    M68k9D70StepResult res = M68k9D70StepResult::NotHandled;

    switch (pc) {
    case 0x00B3B2U:
        res = step_tst_b_abs(api, pc, 0x00FF1927U, 0x00B3B8U, resume_pc);
        break;
    case 0x00B3B8U:
        res = step_bne_w(api, pc, 0x00B382U, 0x00B3BCU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped && resume_pc == 0x00B382U) {
            pause_exit = true;
        }
        break;
    case 0x00B3BCU:
        res = step_move_b_disp_reg(api, pc, kRegA6, 57, kRegD5, 0x00B3C0U, resume_pc);
        break;
    case 0x00B3C0U:
        res = step_btst_imm_reg(api, pc, 1U, kRegD5, 0x00B3C4U, resume_pc);
        break;
    case 0x00B3C4U:
        res = step_bne_w(api, pc, 0x00B3CAU, 0x00B3C8U, resume_pc);
        break;
    case 0x00B3CAU:
        res = step_bsr_w(api, pc, 0x00B66AU, 0x00B3CEU, resume_pc, child_call_active, saved_child_sp);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.child_bsr_calls++;
        }
        break;
    case 0x00B3CEU:
        res = step_bra_w(api, pc, 0x00B3C8U, resume_pc);
        break;
    case 0x00B3C8U:
        res = step_rts(api, pc, return_caller_pc, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.completed_calls++;
            metrics.normal_exit_calls++;
            active = false;
            interrupted = false;
            child_call_active = false;
            pause_exit = false;
            saved_child_sp = 0U;
        }
        break;
    case 0x00B382U:
        res = step_rts(api, pc, return_caller_pc, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.completed_calls++;
            if (pause_exit) {
                metrics.pause_exit_calls++;
            } else {
                metrics.normal_exit_calls++;
            }
            active = false;
            interrupted = false;
            child_call_active = false;
            pause_exit = false;
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

unsigned m68k_00b3b2_resume_pc() noexcept {
    return resume_pc;
}

const M68k00B3B2StepMetrics& m68k_00b3b2_metrics() noexcept {
    return metrics;
}

void write_00b3b2_step_metrics(const std::filesystem::path& path) {
    std::ofstream ofs(path);
    if (!ofs) return;
    ofs << "{\n"
        << "  \"target_entries\": " << metrics.target_entries << ",\n"
        << "  \"native_calls\": " << metrics.native_calls << ",\n"
        << "  \"rom_fallback_calls\": " << metrics.rom_fallback_calls << ",\n"
        << "  \"completed_calls\": " << metrics.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << metrics.stepped_instructions << ",\n"
        << "  \"caller_00ad4e_calls\": " << metrics.caller_00ad4e_calls << ",\n"
        << "  \"caller_other_calls\": " << metrics.caller_other_calls << ",\n"
        << "  \"active_at_entry\": " << metrics.active_at_entry << ",\n"
        << "  \"pause_exit_calls\": " << metrics.pause_exit_calls << ",\n"
        << "  \"normal_exit_calls\": " << metrics.normal_exit_calls << ",\n"
        << "  \"child_bsr_calls\": " << metrics.child_bsr_calls << "\n"
        << "}\n";
}

void reset_m68k_00b3b2_state() noexcept {
    active = false;
    interrupted = false;
    child_call_active = false;
    pause_exit = false;
    saved_child_sp = 0U;
    resume_pc = 0U;
    return_caller_pc = 0U;
    metrics = {};
}

} // namespace oasis::hybrid
