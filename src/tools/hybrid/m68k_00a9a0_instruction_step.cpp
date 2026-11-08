#include "tools/hybrid/m68k_00a9a0_instruction_step.hpp"
#include "tools/hybrid/m68k_00a9a0_attrib_step.hpp"
#include "tools/hybrid/m68k_00a9a0_camera_step.hpp"
#include "tools/hybrid/m68k_00a9a0_commit_step.hpp"
#include "tools/hybrid/m68k_00a9a0_coords_step.hpp"
#include "tools/hybrid/m68k_00a9a0_ingress_step.hpp"
#include "tools/hybrid/m68k_00a9a0_piece_step.hpp"
#include "tools/hybrid/m68k_00a9a0_pieceloop_step.hpp"
#include "tools/hybrid/m68k_00a9a0_render_step.hpp"
#include "tools/hybrid/m68k_00a9a0_shadow_step.hpp"
#include "tools/hybrid/m68k_00a9a0_weapon_step.hpp"
#include "tools/hybrid/m68k_00a9a0_step_ops.hpp"

#include <array>

namespace oasis::hybrid {

M68k00A9A0StepMetrics& mutable_00a9a0_step_metrics() noexcept;

namespace {

using namespace step_00a9a0;

bool active = false;
bool interrupted = false;
bool child_call_active = false;
unsigned saved_child_sp = 0U;
unsigned resume_pc = 0U;

constexpr std::array<std::uint16_t, 3> kRomWords = {{
    0x082EU, 0x0001U, 0x0036U // btst.b #1, 54(a6)
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

void reset_00a9a0_state() noexcept {
    active = false;
    interrupted = false;
    child_call_active = false;
    saved_child_sp = 0U;
    resume_pc = 0U;
}

M68k9D70StepResult step_m68k_00a9a0(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled) return M68k9D70StepResult::NotHandled;

    auto& metrics = mutable_00a9a0_step_metrics();

    if (pc == kStartPc && active) {
        active = false;
        interrupted = false;
        child_call_active = false;
        saved_child_sp = 0U;
    }

    if (!active) {
        if (pc != kStartPc) return M68k9D70StepResult::NotHandled;
        if (!verify_rom(api)) return M68k9D70StepResult::NotHandled;

        metrics.target_entries++;
        active = true;
        interrupted = false;
        child_call_active = false;
        saved_child_sp = 0U;
        resume_pc = pc;
        metrics.native_calls++;
    } else if (child_call_active) {
        const auto sp = api.read_register(api.context, kRegSP) & kMask24;
        if (sp == saved_child_sp && pc == resume_pc) {
            child_call_active = false;
        } else {
            return M68k9D70StepResult::NotHandled;
        }
    } else if (interrupted) {
        if (pc != resume_pc) return M68k9D70StepResult::NotHandled;
        interrupted = false;
    }

    if (active && pc != resume_pc) {
        interrupted = true;
        return M68k9D70StepResult::NotHandled;
    }

    const auto a6 = api.read_register(api.context, kRegA6) & kMask24;

    M68k9D70StepResult res = M68k9D70StepResult::NotHandled;
    if (pc <= 0x00AA02U) {
        res = step_00a9a0_ingress(api, pc, a6, resume_pc, metrics);
    } else if (pc <= 0x00AA74U) {
        res = step_00a9a0_attrib(api, pc, a6, resume_pc, metrics);
    } else if (pc <= 0x00AAFCU) {
        res = step_00a9a0_camera(api, pc, a6, resume_pc, metrics);
    } else if (pc <= 0x00AB66U) {
        res = step_00a9a0_coords(api, pc, a6, resume_pc, metrics);
    } else if (pc <= 0x00ABDCU) {
        res = step_00a9a0_piece(api, pc, a6, resume_pc, child_call_active, saved_child_sp, metrics);
    } else if (pc <= 0x00AC14U) {
        res = step_00a9a0_pieceloop(api, pc, a6, resume_pc, metrics);
    } else if (pc <= 0x00AC5AU) {
        res = step_00a9a0_render(api, pc, a6, resume_pc, metrics);
    } else if (pc >= 0x00ACA0U && pc <= 0x00ACDEU) {
        res = step_00a9a0_shadow(api, pc, resume_pc, metrics);
    } else if ((pc >= 0x00AC5EU && pc <= 0x00ACFCU) || (pc >= 0x00AD1EU && pc <= 0x00AD40U)) {
        res = step_00a9a0_commit(api, pc, resume_pc, active, child_call_active, saved_child_sp, metrics);
    } else if (pc >= 0x00AD42U && pc <= 0x00AD52U) {
        res = step_00a9a0_weapon(api, pc, resume_pc, child_call_active, saved_child_sp, metrics);
    }

    if (res == M68k9D70StepResult::NotHandled) {
        active = false;
        child_call_active = false;
    }
    return res;
}

} // namespace oasis::hybrid
