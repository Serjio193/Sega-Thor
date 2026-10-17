#include "tools/hybrid/m68k_008dd4_instruction_step.hpp"

#include "tools/hybrid/m68k_008dd4_step_ops.hpp"
#include "tools/hybrid/m68k_00f788_step_ops.hpp"
#include "tools/hybrid/m68k_623ac_step_ops.hpp"

#include <array>
#include <fstream>

namespace oasis::hybrid {
namespace {
using namespace step_623ac;
using namespace step_00f788;
using namespace step_008dd4;

constexpr unsigned kStart = 0x008DD4U;
constexpr unsigned kMask24 = 0xFFFFFFU;

struct ProvenCaller {
    unsigned caller_pc;
    unsigned return_pc;
};

constexpr std::array<ProvenCaller, 1> kCallers{{
    {0x008B46U, 0x008B4AU}
}};

constexpr std::array<std::uint16_t, 39> kRomWords{{
    0x1039U, 0x00FFU, 0x183EU, 0x4A79U, 0x00FFU, 0x1AA4U, 0x6726U, 0x0C00U,
    0x0003U, 0x6404U, 0x5200U, 0x602EU, 0x103CU, 0x0003U, 0x0C79U, 0x0014U,
    0x00FFU, 0x199EU, 0x6520U, 0x0839U, 0x0000U, 0x00FFU, 0x199FU, 0x6616U,
    0x5200U, 0x6012U, 0x4A00U, 0x6714U, 0x0C00U, 0x0004U, 0x6404U, 0x5300U,
    0x6004U, 0x103CU, 0x0002U, 0x13C0U, 0x00FFU, 0x183EU, 0x4E75U
}};

bool active{};
bool interrupted{};
unsigned resume_pc{};
M68k008DD4StepMetrics metrics{};

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

unsigned m68k_008dd4_resume_pc() noexcept {
    return resume_pc;
}

M68k9D70StepResult step_m68k_008dd4(unsigned pc, const M68k9D70StepApi& api,
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

        if (caller->caller_pc == 0x008B46U) {
            metrics.caller_008b46_calls++;
        }

        metrics.target_entries++;
        metrics.native_calls++;
        active = true;
        interrupted = false;
    }

    if (!active) return M68k9D70StepResult::NotHandled;

    switch (pc) {
    case 0x008DD4U: {
        const auto res = step_move_b_abs_reg(api, pc, 0x00FF183EU, 0U, 0x008DDAU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008DDAU: {
        const auto res = step_tst_w_abs(api, pc, 0x00FF1AA4U, 0x008DE0U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008DE0U: {
        const auto res = step_bcc_s(api, pc, 7U, 0x008E08U, 0x008DE2U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008DE2U: {
        const auto res = step_cmpi_b_reg(api, pc, 3U, 0U, 0x008DE6U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008DE6U: {
        const auto res = step_bcc_s(api, pc, 4U, 0x008DECU, 0x008DE8U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008DE8U: {
        const auto res = step_addq_b_reg(api, pc, 1U, 0U, 0x008DEAU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008DEAU: {
        const auto res = step_bra_s(api, pc, 0x008E1AU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008DECU: {
        const auto res = step_move_b_imm_reg(api, pc, 3U, 0U, 0x008DF0U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008DF0U: {
        const auto res = step_cmpi_w_abs(api, pc, 0x0014U, 0x00FF199EU, 0x008DF8U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008DF8U: {
        const auto res = step_bcc_s(api, pc, 5U, 0x008E1AU, 0x008DFAU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008DFAU: {
        const auto res = step_btst_imm_abs(api, pc, 0U, 0x00FF199FU, 0x008E02U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008E02U: {
        const auto res = step_bcc_s(api, pc, 6U, 0x008E1AU, 0x008E04U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008E04U: {
        const auto res = step_addq_b_reg(api, pc, 1U, 0U, 0x008E06U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008E06U: {
        const auto res = step_bra_s(api, pc, 0x008E1AU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008E08U: {
        const auto res = step_tst_b_reg(api, pc, 0U, 0x008E0AU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008E0AU: {
        const auto res = step_bcc_s(api, pc, 7U, 0x008E20U, 0x008E0CU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            if (resume_pc == 0x008E20U) metrics.zero_exit_calls++;
        } else {
            active = false;
        }
        return res;
    }

    case 0x008E0CU: {
        const auto res = step_cmpi_b_reg(api, pc, 4U, 0U, 0x008E10U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008E10U: {
        const auto res = step_bcc_s(api, pc, 4U, 0x008E16U, 0x008E12U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008E12U: {
        const auto res = step_subq_b_reg(api, pc, 1U, 0U, 0x008E14U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008E14U: {
        const auto res = step_bra_s(api, pc, 0x008E1AU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008E16U: {
        const auto res = step_move_b_imm_reg(api, pc, 2U, 0U, 0x008E1AU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008E1AU: {
        const auto res = step_move_b_reg_abs(api, pc, 0U, 0x00FF183EU, 0x008E20U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.update_exit_calls++;
        } else {
            active = false;
        }
        return res;
    }

    case 0x008E20U: {
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

const M68k008DD4StepMetrics& get_008dd4_step_metrics() noexcept {
    return metrics;
}

void reset_008dd4_step_metrics() noexcept {
    active = false;
    interrupted = false;
    resume_pc = 0U;
    metrics = {};
}

void write_008dd4_step_metrics(const std::filesystem::path& path) {
    std::ofstream out(path);
    out << "{\n"
        << "  \"target_entries\": " << metrics.target_entries << ",\n"
        << "  \"native_calls\": " << metrics.native_calls << ",\n"
        << "  \"rom_fallback_calls\": " << metrics.rom_fallback_calls << ",\n"
        << "  \"completed_calls\": " << metrics.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << metrics.stepped_instructions << ",\n"
        << "  \"caller_008b46_calls\": " << metrics.caller_008b46_calls << ",\n"
        << "  \"caller_other_calls\": " << metrics.caller_other_calls << ",\n"
        << "  \"active_at_entry\": " << metrics.active_at_entry << ",\n"
        << "  \"zero_exit_calls\": " << metrics.zero_exit_calls << ",\n"
        << "  \"update_exit_calls\": " << metrics.update_exit_calls << "\n"
        << "}\n";
}

} // namespace oasis::hybrid
