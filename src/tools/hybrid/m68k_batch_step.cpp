#include "tools/hybrid/m68k_batch_step.hpp"

#include "tools/hybrid/generated/m68k_recompiled_00d266.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062732.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062722.hpp"

#include <fstream>

namespace oasis::hybrid {

namespace {

void write_metrics_file(const std::filesystem::path& p, unsigned target_entries,
                        unsigned native_calls, unsigned completed_calls,
                        unsigned stepped_instructions) {
    std::ofstream ofs(p);
    if (!ofs) return;
    ofs << "{\n"
        << "  \"target_entries\": " << target_entries << ",\n"
        << "  \"native_calls\": " << native_calls << ",\n"
        << "  \"completed_calls\": " << completed_calls << ",\n"
        << "  \"stepped_instructions\": " << stepped_instructions << "\n"
        << "}\n";
}

} // namespace

M68k9D70StepResult step_m68k_batch_00d266(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    return generated::step_m68k_00d266(pc, api, enabled);
}

M68k9D70StepResult step_m68k_batch_062732(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    return generated::step_m68k_062732(pc, api, enabled);
}

M68k9D70StepResult step_m68k_batch_062722(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    return generated::step_m68k_062722(pc, api, enabled);
}

void write_batch_step_metrics(const std::filesystem::path& p) {
    const auto& m1 = generated::get_metrics_00d266();
    write_metrics_file(p / "00d266-step-metrics.json", m1.target_entries, m1.native_calls, m1.completed_calls, m1.stepped_instructions);
    const auto& m2 = generated::get_metrics_062732();
    write_metrics_file(p / "062732-step-metrics.json", m2.target_entries, m2.native_calls, m2.completed_calls, m2.stepped_instructions);
    const auto& m3 = generated::get_metrics_062722();
    write_metrics_file(p / "062722-step-metrics.json", m3.target_entries, m3.native_calls, m3.completed_calls, m3.stepped_instructions);
}

void reset_batch_step_metrics() noexcept {
    generated::reset_metrics_00d266();
    generated::reset_metrics_062732();
    generated::reset_metrics_062722();
}

} // namespace oasis::hybrid
