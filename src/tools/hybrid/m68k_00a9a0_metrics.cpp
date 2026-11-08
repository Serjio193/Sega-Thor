#include "tools/hybrid/m68k_00a9a0_instruction_step.hpp"

#include <fstream>

namespace oasis::hybrid {

namespace {
M68k00A9A0StepMetrics g_metrics{};
} // namespace

M68k00A9A0StepMetrics& mutable_00a9a0_step_metrics() noexcept {
    return g_metrics;
}

const M68k00A9A0StepMetrics& get_00a9a0_step_metrics() noexcept {
    return g_metrics;
}

void reset_00a9a0_step_metrics() noexcept {
    g_metrics = {};
}

void write_00a9a0_step_metrics(const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) return;
    out << "{\n"
        << "  \"target_entries\": " << g_metrics.target_entries << ",\n"
        << "  \"rom_calls\": " << g_metrics.rom_calls << ",\n"
        << "  \"native_calls\": " << g_metrics.native_calls << ",\n"
        << "  \"stepped_instructions\": " << g_metrics.stepped_instructions << ",\n"
        << "  \"completed_calls\": " << g_metrics.completed_calls << ",\n"
        << "  \"fast_path_hidden_rts\": " << g_metrics.fast_path_hidden_rts << ",\n"
        << "  \"full_render_calls\": " << g_metrics.full_render_calls << ",\n"
        << "  \"piece_loop_steps\": " << g_metrics.piece_loop_steps << ",\n"
        << "  \"shadow_render_steps\": " << g_metrics.shadow_render_steps << ",\n"
        << "  \"child_fallback_calls\": " << g_metrics.child_fallback_calls << ",\n"
        << "  \"weapon_hook_fallbacks\": " << g_metrics.weapon_hook_fallbacks << ",\n"
        << "  \"bsr_00b15e_calls\": " << g_metrics.bsr_00b15e_calls << ",\n"
        << "  \"weapon_00b34c_calls\": " << g_metrics.weapon_00b34c_calls << ",\n"
        << "  \"weapon_00b3b2_calls\": " << g_metrics.weapon_00b3b2_calls << "\n"
        << "}\n";
}

} // namespace oasis::hybrid
