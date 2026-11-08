#include "tools/hybrid/m68k_00ad76_instruction_step.hpp"

#include <fstream>

namespace oasis::hybrid {

namespace {

M68k00AD76StepMetrics g_metrics{};

} // namespace

void write_00ad76_step_metrics(const std::filesystem::path& path) {
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
        << "  \"weapon_hook_fallbacks\": " << g_metrics.weapon_hook_fallbacks << "\n"
        << "}\n";
}

void reset_00ad76_step_metrics() noexcept {
    g_metrics = {};
}

const M68k00AD76StepMetrics& get_00ad76_step_metrics() noexcept {
    return g_metrics;
}

M68k00AD76StepMetrics& mutable_00ad76_step_metrics() noexcept {
    return g_metrics;
}

} // namespace oasis::hybrid
