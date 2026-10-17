#include "tools/hybrid/m68k_008b8e_instruction_step.hpp"

#include "tools/hybrid/m68k_008b8e_step_ops.hpp"

#include <array>
#include <fstream>

namespace oasis::hybrid {
namespace {
using namespace step_008b8e;

constexpr unsigned kStart = 0x008B8EU;
constexpr unsigned kMask24 = 0xFFFFFFU;

struct ProvenCaller {
    unsigned caller_pc;
    unsigned return_pc;
};

constexpr std::array<ProvenCaller, 1> kCallers{{
    {0x008B26U, 0x008B2AU}
}};

constexpr std::array<std::uint16_t, 15> kRomWords{{
    0x1039U, 0x00FFU, 0x1864U, // 0x008B8E: move.b $00FF1864, %d0
    0x6700U, 0x0014U,          // 0x008B94: beq.w  0x008BAA
    0x08B9U, 0x0007U, 0x00FFU, 0x1864U, // 0x008B98: bclr #7, $00FF1864
    0x6604U,                   // 0x008BA0: bne.s  0x008BA6
    0x6000U, 0xC768U,          // 0x008BA2: bra.w  0x00530C
    0x6000U, 0xF11EU,          // 0x008BA6: bra.w  0x007CC6
    0x4E75U                    // 0x008BAA: rts
}};

bool active{};
bool interrupted{};
unsigned resume_pc{};
M68k008B8EStepMetrics metrics{};

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
        unsigned opcode{};
        if (read_be(api, caller.caller_pc, 2U, opcode) && opcode == 0x6100U) {
            return &caller;
        }
    }
    return nullptr;
}

} // namespace

unsigned m68k_008b8e_resume_pc() noexcept {
    return resume_pc;
}

M68k9D70StepResult step_m68k_008b8e(unsigned pc, const M68k9D70StepApi& api,
                                    bool enabled) noexcept {
    if (!enabled) return M68k9D70StepResult::NotHandled;

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

        metrics.caller_008b26_calls++;
        metrics.target_entries++;
        metrics.native_calls++;
        active = true;
        interrupted = false;
    }

    if (!active) return M68k9D70StepResult::NotHandled;

    switch (pc) {
    case 0x008B8EU: {
        const auto res = step_move_b_abs_reg(api, pc, 0x00FF1864U, 0U, 0x008B94U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008B94U: {
        const auto res = step_bcc_w(api, pc, 7U, 0x008BAAU, 0x008B98U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            if (resume_pc == 0x008BAAU) metrics.zero_exit_calls++;
        } else {
            active = false;
        }
        return res;
    }

    case 0x008B98U: {
        const auto res = step_bclr_imm_abs(api, pc, 7U, 0x00FF1864U, 0x008BA0U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BA0U: {
        const auto res = step_bcc_s(api, pc, 6U, 0x008BA6U, 0x008BA2U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BA2U: {
        const auto res = step_bra_w(api, pc, 0x00530CU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.tail_bra_530c_calls++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
        } else {
            active = false;
        }
        return res;
    }

    case 0x008BA6U: {
        const auto res = step_bra_w(api, pc, 0x007CC6U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.tail_bra_7cc6_calls++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
        } else {
            active = false;
        }
        return res;
    }

    case 0x008BAAU: {
        const auto res = step_rts(api, pc, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
        } else {
            active = false;
        }
        return res;
    }

    default:
        active = false;
        interrupted = false;
        return M68k9D70StepResult::NotHandled;
    }
}

const M68k008B8EStepMetrics& get_008b8e_step_metrics() noexcept {
    return metrics;
}

void reset_008b8e_step_metrics() noexcept {
    active = false;
    interrupted = false;
    resume_pc = 0U;
    metrics = {};
}

void write_008b8e_step_metrics(const std::filesystem::path& path) {
    std::ofstream out(path);
    out << "{\n"
        << "  \"target_entries\": " << metrics.target_entries << ",\n"
        << "  \"native_calls\": " << metrics.native_calls << ",\n"
        << "  \"rom_fallback_calls\": " << metrics.rom_fallback_calls << ",\n"
        << "  \"completed_calls\": " << metrics.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << metrics.stepped_instructions << ",\n"
        << "  \"caller_008b26_calls\": " << metrics.caller_008b26_calls << ",\n"
        << "  \"caller_other_calls\": " << metrics.caller_other_calls << ",\n"
        << "  \"active_at_entry\": " << metrics.active_at_entry << ",\n"
        << "  \"zero_exit_calls\": " << metrics.zero_exit_calls << ",\n"
        << "  \"tail_bra_530c_calls\": " << metrics.tail_bra_530c_calls << ",\n"
        << "  \"tail_bra_7cc6_calls\": " << metrics.tail_bra_7cc6_calls << "\n"
        << "}\n";
}

} // namespace oasis::hybrid
