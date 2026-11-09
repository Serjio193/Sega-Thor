#include "tools/hybrid/m68k_batch_step.hpp"

#include "tools/hybrid/generated/m68k_recompiled_00d266.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062732.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062722.hpp"
#include "tools/hybrid/generated/m68k_recompiled_005660.hpp"
#include "tools/hybrid/generated/m68k_recompiled_006e7a.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062694.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062a78.hpp"
#include "tools/hybrid/generated/m68k_recompiled_00d2b4.hpp"
#include "tools/hybrid/generated/m68k_recompiled_0059a8.hpp"

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

M68k9D70StepResult step_m68k_batch_005660(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    return generated::step_m68k_005660(pc, api, enabled);
}

M68k9D70StepResult step_m68k_batch_006e7a(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    return generated::step_m68k_006e7a(pc, api, enabled);
}

M68k9D70StepResult step_m68k_batch_062694(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    return generated::step_m68k_062694(pc, api, enabled);
}

M68k9D70StepResult step_m68k_batch_062a78(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    return generated::step_m68k_062a78(pc, api, enabled);
}

M68k9D70StepResult step_m68k_batch_00d2b4(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    return generated::step_m68k_00d2b4(pc, api, enabled);
}

M68k9D70StepResult step_m68k_batch_0059a8(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    return generated::step_m68k_0059a8(pc, api, enabled);
}

void write_batch_step_metrics(const std::filesystem::path& p) {
    const auto& m1 = generated::get_metrics_00d266();
    write_metrics_file(p / "00d266-step-metrics.json", m1.target_entries, m1.native_calls, m1.completed_calls, m1.stepped_instructions);
    const auto& m2 = generated::get_metrics_062732();
    write_metrics_file(p / "062732-step-metrics.json", m2.target_entries, m2.native_calls, m2.completed_calls, m2.stepped_instructions);
    const auto& m3 = generated::get_metrics_062722();
    write_metrics_file(p / "062722-step-metrics.json", m3.target_entries, m3.native_calls, m3.completed_calls, m3.stepped_instructions);
    const auto& m4 = generated::get_metrics_005660();
    write_metrics_file(p / "005660-step-metrics.json", m4.target_entries, m4.native_calls, m4.completed_calls, m4.stepped_instructions);
    const auto& m5 = generated::get_metrics_006e7a();
    write_metrics_file(p / "006e7a-step-metrics.json", m5.target_entries, m5.native_calls, m5.completed_calls, m5.stepped_instructions);
    const auto& m6 = generated::get_metrics_062694();
    write_metrics_file(p / "062694-step-metrics.json", m6.target_entries, m6.native_calls, m6.completed_calls, m6.stepped_instructions);
    const auto& m7 = generated::get_metrics_062a78();
    write_metrics_file(p / "062a78-step-metrics.json", m7.target_entries, m7.native_calls, m7.completed_calls, m7.stepped_instructions);
    const auto& m8 = generated::get_metrics_00d2b4();
    write_metrics_file(p / "00d2b4-step-metrics.json", m8.target_entries, m8.native_calls, m8.completed_calls, m8.stepped_instructions);
    const auto& m9 = generated::get_metrics_0059a8();
    write_metrics_file(p / "0059a8-step-metrics.json", m9.target_entries, m9.native_calls, m9.completed_calls, m9.stepped_instructions);
}

void reset_batch_step_metrics() noexcept {
    generated::reset_metrics_00d266();
    generated::reset_metrics_062732();
    generated::reset_metrics_062722();
    generated::reset_metrics_005660();
    generated::reset_metrics_006e7a();
    generated::reset_metrics_062694();
    generated::reset_metrics_062a78();
    generated::reset_metrics_00d2b4();
    generated::reset_metrics_0059a8();
}

} // namespace oasis::hybrid
