#include "tools/hybrid/m68k_00ad76_instruction_step.hpp"
#include "tools/hybrid/m68k_00ad76_attrib_step.hpp"
#include "tools/hybrid/m68k_00ad76_camera_step.hpp"
#include "tools/hybrid/m68k_00ad76_commit_step.hpp"
#include "tools/hybrid/m68k_00ad76_ingress_step.hpp"
#include "tools/hybrid/m68k_00ad76_piece_step.hpp"
#include "tools/hybrid/m68k_00ad76_render_step.hpp"
#include "tools/hybrid/m68k_00ad76_shadow_step.hpp"
#include "tools/hybrid/m68k_00ad76_step_ops.hpp"

#include <array>

namespace oasis::hybrid {

M68k00AD76StepMetrics& mutable_00ad76_step_metrics() noexcept;

namespace {

using namespace step_00ad76;

bool active = false;
bool interrupted = false;
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

M68k9D70StepResult step_m68k_00ad76(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept {
    if (!enabled) return M68k9D70StepResult::NotHandled;

    auto& metrics = mutable_00ad76_step_metrics();

    if (active && pc != resume_pc) {
        interrupted = true;
        return M68k9D70StepResult::NotHandled;
    }
    if (interrupted && pc == resume_pc) {
        interrupted = false;
    }

    if (pc == kStartPc) {
        if (active) {
            active = false;
            interrupted = false;
            return M68k9D70StepResult::NotHandled;
        }
        if (!verify_rom(api)) return M68k9D70StepResult::NotHandled;

        metrics.target_entries++;
        const auto a6 = api.read_register(api.context, kRegA6) & kMask24;
        unsigned weapon_part = 0U;
        if (read_be(api, a6 + 57U, 1, weapon_part) && (weapon_part & 0xFFU) != 0U) {
            metrics.rom_calls++;
            metrics.child_fallback_calls++;
            return M68k9D70StepResult::NotHandled;
        }

        active = true;
        interrupted = false;
        resume_pc = pc;
        metrics.native_calls++;
    }

    if (!active) return M68k9D70StepResult::NotHandled;

    const auto a6 = api.read_register(api.context, kRegA6) & kMask24;

    M68k9D70StepResult res = M68k9D70StepResult::NotHandled;
    if (pc <= 0x00ADB2U) {
        res = step_00ad76_ingress(api, pc, a6, resume_pc, metrics);
    } else if (pc <= 0x00AE3EU) {
        res = step_00ad76_attrib(api, pc, a6, resume_pc, metrics);
    } else if (pc <= 0x00AECAU) {
        res = step_00ad76_camera(api, pc, a6, resume_pc, metrics);
    } else if (pc <= 0x00AF16U) {
        res = step_00ad76_piece(api, pc, a6, resume_pc, metrics);
    } else if (pc <= 0x00AF74U) {
        res = step_00ad76_render(api, pc, a6, resume_pc, metrics);
    } else if (pc <= 0x00AFF8U) {
        res = step_00ad76_shadow(api, pc, a6, resume_pc, metrics);
    } else if (pc <= 0x00B010U) {
        res = step_00ad76_commit(api, pc, resume_pc, active, metrics);
    }

    if (res == M68k9D70StepResult::NotHandled) {
        active = false;
    }
    return res;
}

} // namespace oasis::hybrid
