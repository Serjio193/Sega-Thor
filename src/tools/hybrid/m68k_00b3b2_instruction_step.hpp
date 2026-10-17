#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstdint>
#include <filesystem>

namespace oasis::hybrid {

struct M68k00B3B2StepMetrics {
    std::uint64_t target_entries{};
    std::uint64_t native_calls{};
    std::uint64_t rom_fallback_calls{};
    std::uint64_t completed_calls{};
    std::uint64_t stepped_instructions{};
    std::uint64_t caller_00ad4e_calls{};
    std::uint64_t caller_other_calls{};
    std::uint64_t active_at_entry{};
    std::uint64_t pause_exit_calls{};
    std::uint64_t normal_exit_calls{};
    std::uint64_t child_bsr_calls{};
};

M68k9D70StepResult step_m68k_00b3b2(unsigned pc, const M68k9D70StepApi& api,
                                    bool enabled) noexcept;
unsigned m68k_00b3b2_resume_pc() noexcept;
const M68k00B3B2StepMetrics& m68k_00b3b2_metrics() noexcept;
void write_00b3b2_step_metrics(const std::filesystem::path& path);
void reset_m68k_00b3b2_state() noexcept;

} // namespace oasis::hybrid
