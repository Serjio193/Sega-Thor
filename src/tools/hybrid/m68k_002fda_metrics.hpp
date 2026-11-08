#pragma once

#include <cstdint>
#include <filesystem>

namespace oasis::hybrid {

struct M68k002FDAStepMetrics {
    std::uint64_t target_entries{};
    std::uint64_t rom_calls{};
    std::uint64_t native_calls{};
    std::uint64_t stepped_instructions{};
    std::uint64_t completed_calls{};
    std::uint64_t colors_scaled{};
};

M68k002FDAStepMetrics& mutable_002fda_step_metrics() noexcept;
const M68k002FDAStepMetrics& get_002fda_step_metrics() noexcept;
void reset_002fda_step_metrics() noexcept;
void write_002fda_step_metrics(const std::filesystem::path& path);

} // namespace oasis::hybrid
