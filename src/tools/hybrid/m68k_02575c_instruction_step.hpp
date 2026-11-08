#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstdint>
#include <filesystem>

namespace oasis::hybrid {

struct M68k02575CStepMetrics {
    std::uint64_t target_entries{};
    std::uint64_t rom_calls{};
    std::uint64_t native_calls{};
    std::uint64_t unsupported_state_entries{};
    std::uint64_t rom_fallback_calls{};
    std::uint64_t completed_calls{};
    std::uint64_t stepped_instructions{};

    // State 0 (0x025896..0x0258A0)
    std::uint64_t state0_calls{};
    std::uint64_t state0_steps{};
    std::uint64_t state0_rts_exits{};
    std::uint64_t state0_loopback_exits{};

    // State 1 (0x0258A0..0x025940)
    std::uint64_t state1_calls{};
    std::uint64_t state1_steps{};
    std::uint64_t state1_rts_exits{};
    std::uint64_t state1_loopback_exits{};
    std::uint64_t state1_child_calls{};

    // State 2 (0x025940..0x025968)
    std::uint64_t state2_calls{};
    std::uint64_t state2_steps{};
    std::uint64_t state2_rts_exits{};
    std::uint64_t state2_loopback_exits{};

    // State 3 (0x025968..0x0259AC)
    std::uint64_t state3_calls{};
    std::uint64_t state3_steps{};
    std::uint64_t state3_rts_exits{};
    std::uint64_t state3_loopback_exits{};
    std::uint64_t state3_child_calls{};
    std::uint64_t state3_transfer_exits{};

    // State 4 (0x0259AC..0x0259CE)
    std::uint64_t state4_calls{};
    std::uint64_t state4_steps{};
    std::uint64_t state4_rts_exits{};
    std::uint64_t state4_child_calls{};

    // State 5 (0x0259CE..0x025A72)
    std::uint64_t state5_calls{};
    std::uint64_t state5_steps{};
    std::uint64_t state5_rts_exits{};
    std::uint64_t state5_loopback_exits{};
    std::uint64_t state5_child_calls{};

    // State 6 (0x025A72..0x025B22)
    std::uint64_t state6_calls{};
    std::uint64_t state6_steps{};
    std::uint64_t state6_rts_exits{};
    std::uint64_t state6_child_calls{};

    // State 7 (0x025B22..0x025B32)
    std::uint64_t state7_calls{};
    std::uint64_t state7_steps{};
    std::uint64_t state7_rts_exits{};

    // Target 10 (0x0257EE..0x02583E)
    std::uint64_t target10_calls{};
    std::uint64_t target10_steps{};
    std::uint64_t target10_rts_exits{};
    std::uint64_t target10_child_calls{};
    std::uint64_t target10_continuation_entries{};

    // Target 11 (0x02583E..0x025880)
    std::uint64_t target11_calls{};
    std::uint64_t target11_steps{};
    std::uint64_t target11_rts_exits{};
    std::uint64_t target11_transfer_exits{};

    // Target 12 (0x025880..0x025896)
    std::uint64_t target12_calls{};
    std::uint64_t target12_steps{};
    std::uint64_t target12_rts_exits{};
    std::uint64_t target12_loopback_exits{};

    // Global transitions
    std::uint64_t rom_child_return_completions{};
    std::uint64_t native_continuation_entries{};
    std::uint64_t contract_faults{};
};

M68k9D70StepResult step_m68k_02575c(unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
const M68k02575CStepMetrics& get_02575c_step_metrics() noexcept;
void reset_02575c_step_metrics() noexcept;
void write_02575c_step_metrics(const std::filesystem::path& path);

} // namespace oasis::hybrid
