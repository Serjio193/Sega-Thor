#include "tools/hybrid/m68k_0023cc_metrics.hpp"
#include <fstream>

namespace oasis::hybrid {

namespace {
M68k0023ccStepMetrics g_metrics{};
}

M68k0023ccStepMetrics get_0023cc_step_metrics() noexcept { return g_metrics; }
M68k0023ccStepMetrics& mutable_0023cc_step_metrics() noexcept { return g_metrics; }
void reset_0023cc_step_metrics() noexcept { g_metrics = {}; }

void write_0023cc_step_metrics(const std::filesystem::path& out_path) {
    std::ofstream ofs(out_path);
    if (!ofs) return;
    ofs << "{\n"
        << "  \"target_entries\": " << g_metrics.target_entries << ",\n"
        << "  \"native_calls\": " << g_metrics.native_calls << ",\n"
        << "  \"completed_calls\": " << g_metrics.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << g_metrics.stepped_instructions << ",\n"
        << "  \"columns_uploaded\": " << g_metrics.columns_uploaded << "\n"
        << "}\n";
}

} // namespace oasis::hybrid
