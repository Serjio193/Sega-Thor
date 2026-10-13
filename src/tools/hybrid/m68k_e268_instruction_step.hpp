#pragma once

#include "tools/hybrid/m68k_9d70_instruction_step.hpp"

#include <cstdint>
#include <filesystem>

namespace oasis::hybrid {

struct E268StepMetrics {
    std::uint64_t target_entries{};
    std::uint64_t native_calls{};
    std::uint64_t rom_fallback_calls{};
    std::uint64_t provenance_rejects{};
    std::uint64_t precondition_rejects{};
    std::uint64_t contract_rejects{};
    std::uint64_t completed_calls{};
    std::uint64_t stepped_instructions{};
    std::uint64_t scheduler_resume_count{};
    std::uint64_t irq_pause_count{};
    std::uint64_t irq_resume_count{};
    std::uint64_t refresh_variant_calls{};
    std::uint64_t refresh_events{};
};

[[nodiscard]] M68k9D70StepResult step_m68k_e268(
    unsigned pc, const M68k9D70StepApi& api, bool enabled) noexcept;
[[nodiscard]] E268StepMetrics e268_step_metrics() noexcept;
void write_e268_step_metrics(const std::filesystem::path& path);

} // namespace oasis::hybrid
