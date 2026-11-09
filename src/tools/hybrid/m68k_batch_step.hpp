#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstdint>
#include <filesystem>

namespace oasis::hybrid {

struct BatchStepMetrics {
    std::uint64_t target_entries{};
    std::uint64_t native_calls{};
    std::uint64_t completed_calls{};
    std::uint64_t stepped_instructions{};
};

// Experimental Slots 230..238
M68k9D70StepResult step_m68k_062610(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_005660(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_062986(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_06293a(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_062900(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_0626a6(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_062a24(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_062694(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_0629ee(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;

void write_batch_step_metrics(const std::filesystem::path& parent_dir);
void reset_batch_step_metrics() noexcept;

} // namespace oasis::hybrid
