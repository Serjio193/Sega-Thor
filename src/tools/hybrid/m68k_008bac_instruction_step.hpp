#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstddef>
#include <filesystem>

namespace oasis::hybrid {

struct M68k008BACStepMetrics {
    std::size_t target_entries{};
    std::size_t native_calls{};
    std::size_t rom_fallback_calls{};
    std::size_t completed_calls{};
    std::size_t stepped_instructions{};
    std::size_t caller_008b22_calls{};
    std::size_t caller_other_calls{};
    std::size_t active_at_entry{};
    std::size_t rts_completed_calls{};
    std::size_t menu_branch_calls{};
};

M68k9D70StepResult step_m68k_008bac(unsigned pc, const M68k9D70StepApi& api,
                                    bool enabled) noexcept;

[[nodiscard]] unsigned m68k_008bac_resume_pc() noexcept;
const M68k008BACStepMetrics& get_008bac_step_metrics() noexcept;
void reset_008bac_step_metrics() noexcept;
void write_008bac_step_metrics(const std::filesystem::path& path);

} // namespace oasis::hybrid
