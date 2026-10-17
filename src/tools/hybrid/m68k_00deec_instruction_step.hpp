#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstdint>
#include <filesystem>

namespace oasis::hybrid {

struct M68k00DEECStepMetrics {
    std::uint64_t target_entries{};
    std::uint64_t native_calls{};
    std::uint64_t rom_fallback_calls{};
    std::uint64_t completed_calls{};
    std::uint64_t stepped_instructions{};
    std::uint64_t caller_008b32_calls{};
    std::uint64_t caller_other_calls{};
    std::uint64_t active_at_entry{};
    std::uint64_t inhibit_exit_calls{};
    std::uint64_t loop_completed_calls{};
    std::uint64_t child_jsr_calls{};
};

M68k9D70StepResult step_m68k_00deec(unsigned pc, const M68k9D70StepApi& api,
                                    bool enabled) noexcept;
const M68k00DEECStepMetrics& m68k_00deec_metrics() noexcept;
void write_00deec_step_metrics(const std::filesystem::path& path);
void reset_m68k_00deec_state() noexcept;

} // namespace oasis::hybrid
