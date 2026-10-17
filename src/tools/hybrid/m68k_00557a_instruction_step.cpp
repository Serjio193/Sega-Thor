#include "tools/hybrid/m68k_00557a_instruction_step.hpp"

#include "tools/hybrid/m68k_00557a_step_block1.hpp"
#include "tools/hybrid/m68k_00557a_step_block2.hpp"
#include "tools/hybrid/m68k_623ac_step_ops.hpp"

#include <array>
#include <fstream>

namespace oasis::hybrid {
namespace {
using namespace step_623ac;
using namespace step_00557a;

constexpr unsigned kStart = 0x00557AU;
constexpr unsigned kMask24 = 0xFFFFFFU;

struct ProvenCaller {
    unsigned caller_pc;
    unsigned return_pc;
    std::int16_t displacement;
};

constexpr std::array<ProvenCaller, 1> kCallers{{
    {0x008B2EU, 0x008B32U, static_cast<std::int16_t>(0xCA4AU)}
}};

constexpr std::array<std::uint16_t, 123> kRomWords{{
    0x51F9U, 0x00FFU, 0x1986U, 0x4DF9U, 0x00FFU, 0x19E8U, 0x6100U, 0x00E8U,
    0x4DF9U, 0x00FFU, 0x19E8U, 0x4241U, 0x3401U, 0x122EU, 0x0048U, 0x142EU,
    0x0049U, 0x082EU, 0x0000U, 0x0017U, 0x6700U, 0x0004U, 0xC342U, 0x3D41U,
    0x0042U, 0x3D42U, 0x0044U, 0x1039U, 0x00FFU, 0x1993U, 0x6700U, 0x000EU,
    0x5300U, 0x13C0U, 0x00FFU, 0x1993U, 0x6000U, 0x002AU, 0x302EU, 0x008AU,
    0x6700U, 0x0022U, 0x0C40U, 0x0040U, 0x6200U, 0x001AU, 0xE248U, 0x0640U,
    0x000AU, 0x13C0U, 0x00FFU, 0x1993U, 0x203CU, 0x00A0U, 0x038CU, 0x4EB9U,
    0x0006U, 0x0004U, 0x1039U, 0x00FFU, 0x1994U, 0x6700U, 0x000EU, 0x5300U,
    0x13C0U, 0x00FFU, 0x1994U, 0x6000U, 0x0036U, 0x3039U, 0x00FFU, 0x1AA4U,
    0x6700U, 0x002CU, 0x0C40U, 0x0016U, 0x6700U, 0x0024U, 0x302EU, 0x008EU,
    0x0C40U, 0x0018U, 0x6200U, 0x0018U, 0x0640U, 0x000CU, 0x13C0U, 0x00FFU,
    0x1994U, 0x203CU, 0x00A0U, 0x04DDU, 0x4EB9U, 0x0006U, 0x0004U, 0x082EU,
    0x0007U, 0x006FU, 0x6600U, 0x001EU, 0x322EU, 0x0004U, 0x0C41U, 0x0000U,
    0x6700U, 0x0014U, 0x0C41U, 0x0002U, 0x6700U, 0x000CU, 0x0C41U, 0x0004U,
    0x6700U, 0x0004U, 0x4E75U, 0x41F9U, 0x00FFU, 0x1972U, 0x30EEU, 0x0008U,
    0x30AEU, 0x000CU, 0x4E75U
}};

bool active{};
bool interrupted{};
bool child_call_active{};
unsigned saved_child_sp{};
unsigned resume_pc{};
M68k00557AStepMetrics metrics{};

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

unsigned m68k_00557a_resume_pc() noexcept {
    return resume_pc;
}

M68k9D70StepResult step_m68k_00557a(unsigned pc, const M68k9D70StepApi& api,
                                    bool enabled) noexcept {
    if (!enabled) return M68k9D70StepResult::NotHandled;

    if (active && child_call_active) {
        const unsigned sp = api.read_register(api.context, 15U) & kMask24;
        if (sp == saved_child_sp && (pc == 0x00558AU || pc == 0x0055EEU || pc == 0x005638U)) {
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

        if (caller->caller_pc == 0x008B2EU) {
            metrics.caller_008b2e_calls++;
        }

        metrics.target_entries++;
        metrics.native_calls++;
        active = true;
        interrupted = false;
        child_call_active = false;
        saved_child_sp = 0U;
    }

    if (!active) return M68k9D70StepResult::NotHandled;

    auto res = step_00557a_block1(api, pc, resume_pc, child_call_active, saved_child_sp);
    if (res == M68k9D70StepResult::NotHandled) {
        res = step_00557a_block2(api, pc, resume_pc, child_call_active, saved_child_sp);
    }

    if (res == M68k9D70StepResult::InstructionStepped) {
        metrics.stepped_instructions++;
        if (pc == 0x005586U) {
            metrics.child_calls_005670++;
        } else if (pc == 0x0055E8U || pc == 0x005632U) {
            metrics.child_calls_060004++;
        } else if (pc == 0x00565EU) {
            metrics.completed_calls++;
            metrics.shadow_skipped_calls++;
            active = false;
            interrupted = false;
            child_call_active = false;
            saved_child_sp = 0U;
        } else if (pc == 0x00566EU) {
            metrics.completed_calls++;
            metrics.shadow_updated_calls++;
            active = false;
            interrupted = false;
            child_call_active = false;
            saved_child_sp = 0U;
        }
    } else {
        active = false;
        child_call_active = false;
        saved_child_sp = 0U;
    }

    return res;
}

const M68k00557AStepMetrics& get_00557a_step_metrics() noexcept {
    return metrics;
}

void reset_00557a_step_metrics() noexcept {
    active = false;
    interrupted = false;
    child_call_active = false;
    saved_child_sp = 0U;
    resume_pc = 0U;
    metrics = {};
}

void write_00557a_step_metrics(const std::filesystem::path& path) {
    std::ofstream out(path);
    out << "{\n"
        << "  \"target_entries\": " << metrics.target_entries << ",\n"
        << "  \"native_calls\": " << metrics.native_calls << ",\n"
        << "  \"rom_fallback_calls\": " << metrics.rom_fallback_calls << ",\n"
        << "  \"completed_calls\": " << metrics.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << metrics.stepped_instructions << ",\n"
        << "  \"caller_008b2e_calls\": " << metrics.caller_008b2e_calls << ",\n"
        << "  \"caller_other_calls\": " << metrics.caller_other_calls << ",\n"
        << "  \"active_at_entry\": " << metrics.active_at_entry << ",\n"
        << "  \"child_calls_005670\": " << metrics.child_calls_005670 << ",\n"
        << "  \"child_calls_060004\": " << metrics.child_calls_060004 << ",\n"
        << "  \"shadow_skipped_calls\": " << metrics.shadow_skipped_calls << ",\n"
        << "  \"shadow_updated_calls\": " << metrics.shadow_updated_calls << "\n"
        << "}\n";
}

} // namespace oasis::hybrid
