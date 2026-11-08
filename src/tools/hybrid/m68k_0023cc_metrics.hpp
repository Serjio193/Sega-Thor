#pragma once
#include <cstdint>
#include <filesystem>

namespace oasis::hybrid {

struct M68k0023ccStepMetrics {
    std::uint64_t target_entries{0};
    std::uint64_t native_calls{0};
    std::uint64_t completed_calls{0};
    std::uint64_t stepped_instructions{0};
    std::uint64_t columns_uploaded{0};
};

M68k0023ccStepMetrics get_0023cc_step_metrics() noexcept;
M68k0023ccStepMetrics& mutable_0023cc_step_metrics() noexcept;
void reset_0023cc_step_metrics() noexcept;
void write_0023cc_step_metrics(const std::filesystem::path& out_path);

} // namespace oasis::hybrid
