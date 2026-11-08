#include "tools/hybrid/m68k_0023cc_instruction_step.hpp"
#include "tools/hybrid/m68k_0023cc_calc_step.hpp"
#include "tools/hybrid/m68k_0023cc_upload_step.hpp"
#include "tools/hybrid/m68k_0023cc_step_ops.hpp"

#include <array>

namespace oasis::hybrid {

namespace {

using namespace step_0023cc;

bool active = false;
bool interrupted = false;
unsigned resume_pc = 0U;

constexpr std::array<std::uint16_t, 4> kRomWords = {{
    0x206BU, 0x0016U, // movea.l 22(a3), a0
    0x3A2BU, 0x001AU  // move.w 26(a3), d5
}};

bool verify_rom(const M68k9D70StepApi& api) noexcept {
    for (std::size_t i = 0; i < kRomWords.size(); ++i) {
        unsigned word = 0U;
        if (!read_be(api, kStartPc + static_cast<unsigned>(i) * 2U, 2, word) ||
            word != kRomWords[i]) {
            return false;
        }
    }
    return true;
}

} // namespace

void reset_0023cc_state() noexcept {
    active = false;
    interrupted = false;
    resume_pc = 0U;
}

M68k9D70StepResult step_m68k_0023cc(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled) return M68k9D70StepResult::NotHandled;

    auto& metrics = mutable_0023cc_step_metrics();

    if (pc == kStartPc && active) {
        active = false;
        interrupted = false;
    }

    if (!active) {
        if (pc != kStartPc) return M68k9D70StepResult::NotHandled;
        if (!verify_rom(api)) return M68k9D70StepResult::NotHandled;

        metrics.target_entries++;
        active = true;
        interrupted = false;
        resume_pc = pc;
        metrics.native_calls++;
    } else if (interrupted) {
        if (pc != resume_pc) return M68k9D70StepResult::NotHandled;
        interrupted = false;
    }

    if (active && pc != resume_pc) {
        interrupted = true;
        return M68k9D70StepResult::NotHandled;
    }

    M68k9D70StepResult result = M68k9D70StepResult::NotHandled;
    if (pc >= 0x0023CCU && pc <= 0x0024AAU) {
        result = step_calc_range(api, pc, resume_pc);
    } else if (pc >= 0x0024AEU && pc <= 0x002510U) {
        result = step_upload_range(api, pc, resume_pc);
    }

    if (result == M68k9D70StepResult::InstructionStepped) {
        metrics.stepped_instructions++;
        if (pc == kExitRtsPc) {
            metrics.completed_calls++;
            metrics.columns_uploaded++;
            active = false;
            interrupted = false;
        }
    }

    return result;
}

} // namespace oasis::hybrid
