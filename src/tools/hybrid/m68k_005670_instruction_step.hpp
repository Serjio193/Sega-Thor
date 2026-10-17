#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"
#include <cstdint>
#include <filesystem>

namespace oasis::hybrid {

struct M68k005670StepMetrics {
    std::uint64_t target_entries{};
    std::uint64_t native_calls{};
    std::uint64_t rom_fallback_calls{};
    std::uint64_t completed_calls{};
    std::uint64_t stepped_instructions{};
    std::uint64_t caller_005586_calls{};
    std::uint64_t caller_other_calls{};
    std::uint64_t active_at_entry{};
    std::uint64_t child_calls_00e682{};
    std::uint64_t child_calls_008ef8{};
    std::uint64_t dispatch_jmp_calls{};
    std::uint64_t early_rts_calls{};
};

M68k9D70StepResult step_m68k_005670(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
const M68k005670StepMetrics& get_005670_step_metrics() noexcept;
void reset_005670_step_metrics() noexcept;
void write_005670_step_metrics(const std::filesystem::path& path);
unsigned m68k_005670_resume_pc() noexcept;

} // namespace oasis::hybrid
