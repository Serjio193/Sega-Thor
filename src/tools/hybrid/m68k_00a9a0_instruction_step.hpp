#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstdint>
#include <filesystem>

namespace oasis::hybrid {

struct M68k00A9A0StepMetrics {
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
    std::uint64_t bsr_00b15e_calls{};
    std::uint64_t weapon_00b34c_calls{};
    std::uint64_t weapon_00b3b2_calls{};
};

M68k9D70StepResult step_m68k_00a9a0(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
void write_00a9a0_step_metrics(const std::filesystem::path& path);
void reset_00a9a0_step_metrics() noexcept;
void reset_00a9a0_state() noexcept;
const M68k00A9A0StepMetrics& get_00a9a0_step_metrics() noexcept;

} // namespace oasis::hybrid
