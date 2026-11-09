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

// Scheduler-Safe Experimental Slots 230..232
M68k9D70StepResult step_m68k_batch_00d266(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_062732(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_062722(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;

void write_batch_step_metrics(const std::filesystem::path& parent_dir);
void reset_batch_step_metrics() noexcept;

} // namespace oasis::hybrid
