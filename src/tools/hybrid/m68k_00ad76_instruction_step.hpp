#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstdint>
#include <filesystem>

namespace oasis::hybrid {

struct M68k00AD76StepMetrics {
    std::uint64_t target_entries{};
    std::uint64_t rom_calls{};
    std::uint64_t native_calls{};
    std::uint64_t stepped_instructions{};
    std::uint64_t completed_calls{};
    std::uint64_t fast_path_hidden_rts{};
    std::uint64_t full_render_calls{};
    std::uint64_t piece_loop_steps{};
    std::uint64_t shadow_render_steps{};
    std::uint64_t child_fallback_calls{};
    std::uint64_t weapon_hook_fallbacks{};
};

M68k9D70StepResult step_m68k_00ad76(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
void write_00ad76_step_metrics(const std::filesystem::path& path);
void reset_00ad76_step_metrics() noexcept;
void reset_00ad76_state() noexcept;
const M68k00AD76StepMetrics& get_00ad76_step_metrics() noexcept;

} // namespace oasis::hybrid
