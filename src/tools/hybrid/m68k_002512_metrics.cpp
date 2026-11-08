#include "tools/hybrid/m68k_002512_metrics.hpp"

#include <fstream>

namespace oasis::hybrid {

namespace {
M68k002512StepMetrics g_metrics{};
} // namespace

M68k002512StepMetrics& mutable_002512_step_metrics() noexcept {
    return g_metrics;
}

const M68k002512StepMetrics& get_002512_step_metrics() noexcept {
    return g_metrics;
}

void reset_002512_step_metrics() noexcept {
    g_metrics = M68k002512StepMetrics{};
}

void write_002512_step_metrics(const std::filesystem::path& path) {
    std::ofstream out(path);
    if (!out) return;
    out << "{\n"
        << "  \"target_entries\": " << g_metrics.target_entries << ",\n"
        << "  \"rom_calls\": " << g_metrics.rom_calls << ",\n"
        << "  \"native_calls\": " << g_metrics.native_calls << ",\n"
        << "  \"stepped_instructions\": " << g_metrics.stepped_instructions << ",\n"
        << "  \"completed_calls\": " << g_metrics.completed_calls << ",\n"
        << "  \"rows_uploaded\": " << g_metrics.rows_uploaded << "\n"
        << "}\n";
}

} // namespace oasis::hybrid
