#include "tools/hybrid/m68k_008bac_instruction_step.hpp"

#include "tools/hybrid/m68k_008bac_step_ops.hpp"

#include <array>
#include <fstream>

namespace oasis::hybrid {
namespace {
using namespace step_008bac;

constexpr unsigned kStart = 0x008BACU;
constexpr unsigned kMask24 = 0xFFFFFFU;

struct ProvenCaller {
    unsigned caller_pc;
    unsigned return_pc;
};

constexpr std::array<ProvenCaller, 1> kCallers{{
    {0x008B22U, 0x008B26U}
}};

constexpr std::array<std::uint16_t, 87> kRomWords{{
    0x1039U, 0x00FFU, 0x165EU, 0x4600U, 0x0200U, 0x0070U, 0x6600U, 0x000EU,
    0x50F9U, 0x00FFU, 0x19A7U, 0x4279U, 0x00FFU, 0x16EAU, 0x4A39U, 0x00FFU,
    0x1987U, 0x6600U, 0x0064U, 0x0839U, 0x0002U, 0x00FFU, 0x17B8U, 0x6600U,
    0x0058U, 0x4A39U, 0x00FFU, 0x1983U, 0x6600U, 0x004EU, 0x4A39U, 0x00FFU,
    0x1982U, 0x6600U, 0x0044U, 0x51F9U, 0x00FFU, 0x185EU, 0x0839U, 0x0007U,
    0x00FFU, 0x165FU, 0x6600U, 0xAF3CU, 0x3039U, 0x00FFU, 0x1660U, 0x0800U,
    0x0002U, 0x6600U, 0x003EU, 0x0C79U, 0x0002U, 0x00FFU, 0x19ECU, 0x6218U,
    0x4A79U, 0x00FFU, 0x1A6EU, 0x6610U, 0x0800U, 0x0001U, 0x6600U, 0x000CU,
    0x0800U, 0x0000U, 0x6600U, 0x0010U, 0x4E75U, 0x13FCU, 0x0001U, 0x00FFU,
    0x185EU, 0x6000U, 0xAEFEU, 0x13FCU, 0x0002U, 0x00FFU, 0x185EU, 0x6000U,
    0xAEF2U, 0x13FCU, 0x0003U, 0x00FFU, 0x185EU, 0x6000U, 0xAEE6U
}};

bool active{};
bool interrupted{};
unsigned resume_pc{};
M68k008BACStepMetrics metrics{};

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

unsigned m68k_008bac_resume_pc() noexcept {
    return resume_pc;
}

M68k9D70StepResult step_m68k_008bac(unsigned pc, const M68k9D70StepApi& api,
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

        metrics.caller_008b22_calls++;
        metrics.target_entries++;
        metrics.native_calls++;
        active = true;
        interrupted = false;
    }

    if (!active) return M68k9D70StepResult::NotHandled;

    switch (pc) {
    case 0x008BACU: {
        const auto res = step_move_b_abs_reg(api, pc, 0x00FF165EU, 0U, 0x008BB2U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BB2U: {
        const auto res = step_not_b_reg(api, pc, 0U, 0x008BB4U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BB4U: {
        const auto res = step_andi_b_imm_reg(api, pc, 0x70U, 0U, 0x008BB8U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BB8U: {
        const auto res = step_bcc_w(api, pc, 6U, 0x008BC8U, 0x008BBCU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BBCU: {
        const auto res = step_st_abs(api, pc, 0x00FF19A7U, 0x008BC2U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BC2U: {
        const auto res = step_clr_w_abs(api, pc, 0x00FF16EAU, 0x008BC8U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BC8U: {
        const auto res = step_tst_b_abs(api, pc, 0x00FF1987U, 0x008BCEU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BCEU: {
        const auto res = step_bcc_w(api, pc, 6U, 0x008C34U, 0x008BD2U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BD2U: {
        const auto res = step_btst_imm_abs(api, pc, 2U, 0x00FF17B8U, 0x008BDAU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BDAU: {
        const auto res = step_bcc_w(api, pc, 6U, 0x008C34U, 0x008BDEU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BDEU: {
        const auto res = step_tst_b_abs(api, pc, 0x00FF1983U, 0x008BE4U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BE4U: {
        const auto res = step_bcc_w(api, pc, 6U, 0x008C34U, 0x008BE8U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BE8U: {
        const auto res = step_tst_b_abs(api, pc, 0x00FF1982U, 0x008BEEU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BEEU: {
        const auto res = step_bcc_w(api, pc, 6U, 0x008C34U, 0x008BF2U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BF2U: {
        const auto res = step_sf_abs(api, pc, 0x00FF185EU, 0x008BF8U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008BF8U: {
        const auto res = step_btst_imm_abs(api, pc, 7U, 0x00FF165FU, 0x008C00U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C00U: {
        const auto res = step_bcc_w(api, pc, 6U, 0x003B3EU, 0x008C04U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            if (resume_pc == 0x003B3EU) {
                metrics.menu_branch_calls++;
                metrics.completed_calls++;
                active = false;
                interrupted = false;
            }
        } else {
            active = false;
        }
        return res;
    }

    case 0x008C04U: {
        const auto res = step_move_w_abs_reg(api, pc, 0x00FF1660U, 0U, 0x008C0AU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C0AU: {
        const auto res = step_btst_imm_reg(api, pc, 2U, 0U, 0x008C0EU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C0EU: {
        const auto res = step_bcc_w(api, pc, 6U, 0x008C4EU, 0x008C12U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C12U: {
        const auto res = step_cmpi_w_abs(api, pc, 2U, 0x00FF19ECU, 0x008C1AU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C1AU: {
        const auto res = step_bcc_s(api, pc, 2U, 0x008C34U, 0x008C1CU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C1CU: {
        const auto res = step_tst_w_abs(api, pc, 0x00FF1A6EU, 0x008C22U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C22U: {
        const auto res = step_bcc_s(api, pc, 6U, 0x008C34U, 0x008C24U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C24U: {
        const auto res = step_btst_imm_reg(api, pc, 1U, 0U, 0x008C28U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C28U: {
        const auto res = step_bcc_w(api, pc, 6U, 0x008C36U, 0x008C2CU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C2CU: {
        const auto res = step_btst_imm_reg(api, pc, 0U, 0U, 0x008C30U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C30U: {
        const auto res = step_bcc_w(api, pc, 6U, 0x008C42U, 0x008C34U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C34U: {
        const auto res = step_rts(api, pc, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.rts_completed_calls++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
        } else {
            active = false;
        }
        return res;
    }

    case 0x008C36U: {
        const auto res = step_move_b_imm_abs(api, pc, 1U, 0x00FF185EU, 0x008C3EU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C3EU: {
        const auto res = step_bra_w(api, pc, 0x003B3EU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.menu_branch_calls++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
        } else {
            active = false;
        }
        return res;
    }

    case 0x008C42U: {
        const auto res = step_move_b_imm_abs(api, pc, 2U, 0x00FF185EU, 0x008C4AU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C4AU: {
        const auto res = step_bra_w(api, pc, 0x003B3EU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.menu_branch_calls++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
        } else {
            active = false;
        }
        return res;
    }

    case 0x008C4EU: {
        const auto res = step_move_b_imm_abs(api, pc, 3U, 0x00FF185EU, 0x008C56U, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) metrics.stepped_instructions++;
        else active = false;
        return res;
    }

    case 0x008C56U: {
        const auto res = step_bra_w(api, pc, 0x003B3EU, resume_pc);
        if (res == M68k9D70StepResult::InstructionStepped) {
            metrics.stepped_instructions++;
            metrics.menu_branch_calls++;
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

const M68k008BACStepMetrics& get_008bac_step_metrics() noexcept {
    return metrics;
}

void reset_008bac_step_metrics() noexcept {
    active = false;
    interrupted = false;
    resume_pc = 0U;
    metrics = {};
}

void write_008bac_step_metrics(const std::filesystem::path& path) {
    std::ofstream out(path);
    out << "{\n"
        << "  \"target_entries\": " << metrics.target_entries << ",\n"
        << "  \"native_calls\": " << metrics.native_calls << ",\n"
        << "  \"rom_fallback_calls\": " << metrics.rom_fallback_calls << ",\n"
        << "  \"completed_calls\": " << metrics.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << metrics.stepped_instructions << ",\n"
        << "  \"caller_008b22_calls\": " << metrics.caller_008b22_calls << ",\n"
        << "  \"caller_other_calls\": " << metrics.caller_other_calls << ",\n"
        << "  \"active_at_entry\": " << metrics.active_at_entry << ",\n"
        << "  \"rts_completed_calls\": " << metrics.rts_completed_calls << ",\n"
        << "  \"menu_branch_calls\": " << metrics.menu_branch_calls << "\n"
        << "}\n";
}

} // namespace oasis::hybrid
