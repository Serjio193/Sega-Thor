#include "tools/hybrid/generated/m68k_recompiled_00c9b2.hpp"
#include "tools/hybrid/generated/m68k_step_helpers.hpp"
#include "tools/hybrid/generated/m68k_recompiled_00c9b2_part0.hpp"
#include "tools/hybrid/generated/m68k_recompiled_00c9b2_part1.hpp"

namespace oasis::hybrid::generated {
namespace {

constexpr unsigned kStart = 0x00C9B2U;
bool active = false;
bool interrupted = false;
unsigned resume_pc = 0U;
StepMetrics_00c9b2 metrics{};

} // namespace

M68k9D70StepResult step_m68k_00c9b2(unsigned pc_in, const M68k9D70StepApi& api, bool enabled) noexcept {
    const unsigned pc = pc_in & kMask24;
    if (pc == kStart) {
        metrics.target_entries++;
        if (!enabled) return M68k9D70StepResult::NotHandled;
        if (active) {
            metrics.active_at_entry++;
            active = false;
            interrupted = false;
            return M68k9D70StepResult::ContractFault;
        }
        active = true;
        interrupted = false;
        metrics.native_calls++;
    }
    if (!active) return M68k9D70StepResult::NotHandled;
    if (active && pc != resume_pc && pc != kStart) {
        interrupted = true;
        return M68k9D70StepResult::NotHandled;
    }
    if (interrupted && pc == resume_pc) {
        interrupted = false;
    }

    M68k9D70StepResult res = M68k9D70StepResult::NotHandled;
    res = step_m68k_00c9b2_part0(pc, api, resume_pc, metrics, active, interrupted);
    if (res != M68k9D70StepResult::NotHandled) goto step_done;
    res = step_m68k_00c9b2_part1(pc, api, resume_pc, metrics, active, interrupted);
    if (res != M68k9D70StepResult::NotHandled) goto step_done;
step_done:
    if (res == M68k9D70StepResult::InstructionStepped) {
        if (pc == 0x00C9E6U || pc == 0x00C9EAU) {
            active = false;
            interrupted = false;
        }
        return res;
    }
    active = false;
    interrupted = false;
    return res;
}

const StepMetrics_00c9b2& get_metrics_00c9b2() noexcept { return metrics; }
void reset_metrics_00c9b2() noexcept { metrics = {}; active = false; interrupted = false; resume_pc = 0; }

} // namespace oasis::hybrid::generated
