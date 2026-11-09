#include "tools/hybrid/m68k_batch_step.hpp"

#include "tools/hybrid/generated/m68k_recompiled_062610.hpp"
#include "tools/hybrid/generated/m68k_recompiled_005660.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062986.hpp"
#include "tools/hybrid/generated/m68k_recompiled_06293a.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062900.hpp"
#include "tools/hybrid/generated/m68k_recompiled_0626a6.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062a24.hpp"
#include "tools/hybrid/generated/m68k_recompiled_062694.hpp"
#include "tools/hybrid/generated/m68k_recompiled_0629ee.hpp"

#include <fstream>

namespace oasis::hybrid {

namespace {

BatchStepMetrics g_m_062610{};
BatchStepMetrics g_m_005660{};
BatchStepMetrics g_m_062986{};
BatchStepMetrics g_m_06293a{};
BatchStepMetrics g_m_062900{};
BatchStepMetrics g_m_0626a6{};
BatchStepMetrics g_m_062a24{};
BatchStepMetrics g_m_062694{};
BatchStepMetrics g_m_0629ee{};

inline M68k9D70StepResult invoke_recompiled(
    const M68k9D70StepApi& api,
    M68k9D70StepResult (*fn)(const M68k9D70StepApi&, unsigned&) noexcept,
    BatchStepMetrics& m,
    unsigned insn_count) noexcept {
    m.target_entries++;
    unsigned resume_pc = 0U;
    const auto res = fn(api, resume_pc);
    if (res != M68k9D70StepResult::InstructionStepped || resume_pc == 0U) {
        return M68k9D70StepResult::ContractFault;
    }
    const auto w0 = static_cast<std::uint16_t>(api.read(api.context, resume_pc & 0xFFFFFFU, 2));
    api.set_return_state(api.context, resume_pc & 0xFFFFFFU, (resume_pc + 2U) & 0xFFFFFFU, w0);
    m.native_calls++;
    m.completed_calls++;
    m.stepped_instructions += insn_count;
    return M68k9D70StepResult::InstructionStepped;
}

void write_single_metric(const std::filesystem::path& p, const BatchStepMetrics& m) {
    std::ofstream ofs(p);
    if (!ofs) return;
    ofs << "{\n"
        << "  \"target_entries\": " << m.target_entries << ",\n"
        << "  \"native_calls\": " << m.native_calls << ",\n"
        << "  \"completed_calls\": " << m.completed_calls << ",\n"
        << "  \"stepped_instructions\": " << m.stepped_instructions << "\n"
        << "}\n";
}

} // namespace

M68k9D70StepResult step_m68k_062610(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled || (pc & 0xFFFFFFU) != 0x062610U) return M68k9D70StepResult::NotHandled;
    return invoke_recompiled(api, generated::m68k_recompiled_062610, g_m_062610, 32);
}

M68k9D70StepResult step_m68k_005660(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled || (pc & 0xFFFFFFU) != 0x005660U) return M68k9D70StepResult::NotHandled;
    return invoke_recompiled(api, generated::m68k_recompiled_005660, g_m_005660, 4);
}

M68k9D70StepResult step_m68k_062986(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled || (pc & 0xFFFFFFU) != 0x062986U) return M68k9D70StepResult::NotHandled;
    return invoke_recompiled(api, generated::m68k_recompiled_062986, g_m_062986, 12);
}

M68k9D70StepResult step_m68k_06293a(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled || (pc & 0xFFFFFFU) != 0x06293AU) return M68k9D70StepResult::NotHandled;
    return invoke_recompiled(api, generated::m68k_recompiled_06293a, g_m_06293a, 20);
}

M68k9D70StepResult step_m68k_062900(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled || (pc & 0xFFFFFFU) != 0x062900U) return M68k9D70StepResult::NotHandled;
    return invoke_recompiled(api, generated::m68k_recompiled_062900, g_m_062900, 19);
}

M68k9D70StepResult step_m68k_0626a6(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled || (pc & 0xFFFFFFU) != 0x0626A6U) return M68k9D70StepResult::NotHandled;
    return invoke_recompiled(api, generated::m68k_recompiled_0626a6, g_m_0626a6, 8);
}

M68k9D70StepResult step_m68k_062a24(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled || (pc & 0xFFFFFFU) != 0x062A24U) return M68k9D70StepResult::NotHandled;
    return invoke_recompiled(api, generated::m68k_recompiled_062a24, g_m_062a24, 25);
}

M68k9D70StepResult step_m68k_062694(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled || (pc & 0xFFFFFFU) != 0x062694U) return M68k9D70StepResult::NotHandled;
    return invoke_recompiled(api, generated::m68k_recompiled_062694, g_m_062694, 5);
}

M68k9D70StepResult step_m68k_0629ee(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled || (pc & 0xFFFFFFU) != 0x0629EEU) return M68k9D70StepResult::NotHandled;
    return invoke_recompiled(api, generated::m68k_recompiled_0629ee, g_m_0629ee, 9);
}

void write_batch_step_metrics(const std::filesystem::path& p) {
    write_single_metric(p / "062610-step-metrics.json", g_m_062610);
    write_single_metric(p / "005660-step-metrics.json", g_m_005660);
    write_single_metric(p / "062986-step-metrics.json", g_m_062986);
    write_single_metric(p / "06293a-step-metrics.json", g_m_06293a);
    write_single_metric(p / "062900-step-metrics.json", g_m_062900);
    write_single_metric(p / "0626a6-step-metrics.json", g_m_0626a6);
    write_single_metric(p / "062a24-step-metrics.json", g_m_062a24);
    write_single_metric(p / "062694-step-metrics.json", g_m_062694);
    write_single_metric(p / "0629ee-step-metrics.json", g_m_0629ee);
}

void reset_batch_step_metrics() noexcept {
    g_m_062610 = {};
    g_m_005660 = {};
    g_m_062986 = {};
    g_m_06293a = {};
    g_m_062900 = {};
    g_m_0626a6 = {};
    g_m_062a24 = {};
    g_m_062694 = {};
    g_m_0629ee = {};
}

} // namespace oasis::hybrid
