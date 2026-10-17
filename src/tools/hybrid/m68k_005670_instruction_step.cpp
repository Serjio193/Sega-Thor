#include "tools/hybrid/m68k_005670_instruction_step.hpp"
#include "tools/hybrid/m68k_005670_step_block1.hpp"
#include "tools/hybrid/m68k_005670_step_block2.hpp"
#include "tools/hybrid/m68k_005670_step_block3.hpp"
#include <fstream>
#include <array>

namespace oasis::hybrid {
namespace {
using namespace step_005670;
constexpr unsigned kMask24 = 0xFFFFFFU;

struct CallerContract {
    unsigned caller_pc{};
    unsigned return_pc{};
};

constexpr std::array<CallerContract, 1> kCallers{{
    {0x005586U, 0x00558AU}
}};

const CallerContract* find_caller(const M68k9D70StepApi& api, unsigned sp) noexcept {
    const unsigned ret = api.read(api.context, sp & kMask24, 4);
    for (const auto& c : kCallers) {
        if (c.return_pc == ret) return &c;
    }
    return nullptr;
}

bool active = false;
bool interrupted = false;
bool child_call_active = false;
unsigned saved_child_sp = 0U;
unsigned entry_sp = 0U;
unsigned resume_pc = 0U;
M68k005670StepMetrics metrics{};
} // namespace

unsigned m68k_005670_resume_pc() noexcept {
    return resume_pc;
}

M68k9D70StepResult step_m68k_005670(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled) return M68k9D70StepResult::NotHandled;

    if (pc == 0x005670U) {
        if (active) {
            metrics.active_at_entry++;
            metrics.completed_calls++;
            active = false;
            interrupted = false;
            child_call_active = false;
            saved_child_sp = 0U;
        }

        const unsigned sp = api.read_register(api.context, 15U) & kMask24;
        if ((sp & 1U) != 0U) {
            metrics.rom_fallback_calls++;
            return M68k9D70StepResult::NotHandled;
        }

        const auto* caller = find_caller(api, sp);
        if (!caller) {
            metrics.caller_other_calls++;
            metrics.rom_fallback_calls++;
            return M68k9D70StepResult::NotHandled;
        }

        if (caller->caller_pc == 0x005586U) {
            metrics.caller_005586_calls++;
        }

        metrics.target_entries++;
        metrics.native_calls++;
        active = true;
        interrupted = false;
        child_call_active = false;
        saved_child_sp = 0U;
        entry_sp = sp;
    }

    if (child_call_active) {
        const unsigned cur_sp = api.read_register(api.context, 15U) & kMask24;
        if (pc == resume_pc && cur_sp == saved_child_sp) {
            child_call_active = false;
            saved_child_sp = 0U;
        } else if (pc == 0x00558AU && cur_sp >= entry_sp + 4U) {
            // Child subroutine (e.g. 0x00E682) popped return PC and returned to grandparent
            active = false;
            child_call_active = false;
            saved_child_sp = 0U;
            metrics.completed_calls++;
            return M68k9D70StepResult::NotHandled;
        } else {
            return M68k9D70StepResult::NotHandled;
        }
    }

    if (active && pc != resume_pc) {
        const unsigned cur_sp = api.read_register(api.context, 15U) & kMask24;
        if (cur_sp < (entry_sp & kMask24)) {
            interrupted = true;
            return M68k9D70StepResult::NotHandled;
        }
        if (interrupted && cur_sp == (entry_sp & kMask24)) {
            interrupted = false;
            if (pc != resume_pc) {
                active = false;
                metrics.rom_fallback_calls++;
                return M68k9D70StepResult::NotHandled;
            }
        }
    }

    if (!active) return M68k9D70StepResult::NotHandled;

    auto res = step_005670_block1(api, pc, resume_pc, child_call_active, saved_child_sp);
    if (res == M68k9D70StepResult::NotHandled) {
        res = step_005670_block2(api, pc, resume_pc, child_call_active, saved_child_sp);
    }
    if (res == M68k9D70StepResult::NotHandled) {
        res = step_005670_block3(api, pc, resume_pc, child_call_active, saved_child_sp);
    }

    if (res == M68k9D70StepResult::InstructionStepped) {
        metrics.stepped_instructions++;
        if (pc == 0x0057D0U) {
            metrics.child_calls_00e682++;
        } else if (pc == 0x0058FAU || pc == 0x005928U || pc == 0x005B9AU) {
            metrics.child_calls_008ef8++;
        } else if (pc == 0x0059B8U) {
            // Dispatch jmp to action handler completes 005670
            metrics.completed_calls++;
            metrics.dispatch_jmp_calls++;
            active = false;
            interrupted = false;
            child_call_active = false;
            saved_child_sp = 0U;
        } else if (pc == 0x0056FAU || pc == 0x0057CEU) {
            metrics.completed_calls++;
            metrics.early_rts_calls++;
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

const M68k005670StepMetrics& get_005670_step_metrics() noexcept {
    return metrics;
}

void reset_005670_step_metrics() noexcept {
    active = false;
    interrupted = false;
    child_call_active = false;
    saved_child_sp = 0U;
    entry_sp = 0U;
    resume_pc = 0U;
    metrics = {};
}

void write_005670_step_metrics(const std::filesystem::path& path) {
    std::ofstream out(path);
    out << "{\n"
        << "  \"target_entries\": " << metrics.target_entries << ",\n"
        << "  \"native_calls\": " << metrics.native_calls << ",\n"
        << "  \"rom_fallback_calls\": " << metrics.rom_fallback_calls << ",\n"
        << "  \"completed_calls\": " << metrics.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << metrics.stepped_instructions << ",\n"
        << "  \"caller_005586_calls\": " << metrics.caller_005586_calls << ",\n"
        << "  \"caller_other_calls\": " << metrics.caller_other_calls << ",\n"
        << "  \"active_at_entry\": " << metrics.active_at_entry << ",\n"
        << "  \"child_calls_00e682\": " << metrics.child_calls_00e682 << ",\n"
        << "  \"child_calls_008ef8\": " << metrics.child_calls_008ef8 << ",\n"
        << "  \"dispatch_jmp_calls\": " << metrics.dispatch_jmp_calls << ",\n"
        << "  \"early_rts_calls\": " << metrics.early_rts_calls << "\n"
        << "}\n";
}

} // namespace oasis::hybrid
