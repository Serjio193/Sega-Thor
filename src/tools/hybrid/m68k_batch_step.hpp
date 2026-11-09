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

// Scheduler-Safe Experimental Slots 230..236
M68k9D70StepResult step_m68k_batch_00d266(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_062732(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_062722(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_005660(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_006e7a(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_062694(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_062a78(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_00d2b4(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_0059a8(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_00e682(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_015ae2(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_00c9b2(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_00e6ba(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_00f0d2(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
M68k9D70StepResult step_m68k_batch_016294(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;

void write_batch_step_metrics(const std::filesystem::path& parent_dir);
void reset_batch_step_metrics() noexcept;

} // namespace oasis::hybrid
