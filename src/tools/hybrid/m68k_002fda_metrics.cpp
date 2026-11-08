#include "tools/hybrid/m68k_002fda_metrics.hpp"

#include <fstream>

namespace oasis::hybrid {

namespace {
M68k002FDAStepMetrics g_metrics{};
} // namespace

M68k002FDAStepMetrics& mutable_002fda_step_metrics() noexcept {
    return g_metrics;
}

const M68k002FDAStepMetrics& get_002fda_step_metrics() noexcept {
    return g_metrics;
}

void reset_002fda_step_metrics() noexcept {
    g_metrics = {};
}

void write_002fda_step_metrics(const std::filesystem::path& path) {
    if (path.has_parent_path()) {
        std::filesystem::create_directories(path.parent_path());
    }
    std::ofstream out(path);
    if (!out) return;

    out << "{\n";
    out << "  \"target_entries\": " << g_metrics.target_entries << ",\n";
    out << "  \"rom_calls\": " << g_metrics.rom_calls << ",\n";
    out << "  \"native_calls\": " << g_metrics.native_calls << ",\n";
    out << "  \"stepped_instructions\": " << g_metrics.stepped_instructions << ",\n";
    out << "  \"completed_calls\": " << g_metrics.completed_calls << ",\n";
    out << "  \"colors_scaled\": " << g_metrics.colors_scaled << "\n";
    out << "}\n";
}

} // namespace oasis::hybrid
