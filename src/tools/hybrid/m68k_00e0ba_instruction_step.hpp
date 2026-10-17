#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstdint>
#include <filesystem>

namespace oasis::hybrid {

struct M68k00E0BAStepMetrics {
    std::uint64_t target_entries{};
    std::uint64_t native_calls{};
    std::uint64_t rom_fallback_calls{};
    std::uint64_t completed_calls{};
    std::uint64_t stepped_instructions{};
    std::uint64_t caller_008b36_calls{};
    std::uint64_t caller_other_calls{};
    std::uint64_t active_at_entry{};
    std::uint64_t inhibit_exit_calls{};
    std::uint64_t loop_completed_calls{};
    std::uint64_t child_jsr_calls{};
};

M68k9D70StepResult step_m68k_00e0ba(unsigned pc, const M68k9D70StepApi& api,
                                    bool enabled) noexcept;
const M68k00E0BAStepMetrics& m68k_00e0ba_metrics() noexcept;
void write_00e0ba_step_metrics(const std::filesystem::path& path);
void reset_m68k_00e0ba_state() noexcept;

} // namespace oasis::hybrid
