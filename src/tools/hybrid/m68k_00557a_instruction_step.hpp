#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstddef>
#include <filesystem>

namespace oasis::hybrid {

struct M68k00557AStepMetrics {
    std::size_t target_entries{};
    std::size_t native_calls{};
    std::size_t rom_fallback_calls{};
    std::size_t completed_calls{};
    std::size_t stepped_instructions{};
    std::size_t caller_008b2e_calls{};
    std::size_t caller_other_calls{};
    std::size_t active_at_entry{};
    std::size_t child_calls_005670{};
    std::size_t child_calls_060004{};
    std::size_t shadow_skipped_calls{};
    std::size_t shadow_updated_calls{};
};

M68k9D70StepResult step_m68k_00557a(unsigned pc, const M68k9D70StepApi& api,
                                    bool enabled) noexcept;

[[nodiscard]] unsigned m68k_00557a_resume_pc() noexcept;
const M68k00557AStepMetrics& get_00557a_step_metrics() noexcept;
void reset_00557a_step_metrics() noexcept;
void write_00557a_step_metrics(const std::filesystem::path& path);

} // namespace oasis::hybrid
