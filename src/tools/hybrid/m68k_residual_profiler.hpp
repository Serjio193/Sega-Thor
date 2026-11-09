#pragma once

#include "tools/hybrid/gameplay_session_cpu.hpp"
#include "tools/hybrid/replay_equivalence_trace.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <string>
#include <unordered_map>
#include <vector>

namespace oasis::hybrid {

using RegisterRead = session::RegisterRead;
using SchedulerRead = session::SchedulerRead;

struct CallerHit {
    std::uint32_t caller_pc{};
    std::uint64_t count{};
};

struct ResidualInstructionRecord {
    std::uint32_t pc{};
    std::uint16_t opcode{};
    std::uint64_t count{};
    std::uint32_t first_frame{};
    std::uint32_t last_frame{};
    std::uint32_t function_pc{};
    std::vector<CallerHit> top_callers;
    std::array<std::uint64_t, 8> irq_masks{};
};

struct ResidualProfileSummary {
    std::uint64_t total_guest_steps{};
    std::uint64_t idle_interpreter_steps{};
    std::uint64_t production_native_steps{};
    std::uint64_t experimental_native_steps{};
    std::uint64_t non_idle_interpreter_steps{};
    std::size_t unique_interpreted_pcs{};
    std::string profile_sha256;
};

class M68kResidualProfiler {
public:
    static M68kResidualProfiler& instance() noexcept;

    void init(const std::filesystem::path& output_dir, const std::string& scenario);
    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

    void observe(unsigned pc, unsigned opcode, unsigned frame, unsigned previous_pc,
                 RegisterRead read_register, SchedulerRead read_scheduler, bool native_step);

    void finish();

    [[nodiscard]] const ResidualProfileSummary& summary() const noexcept { return summary_; }

private:
    M68kResidualProfiler() = default;

    struct InternalRecord {
        std::uint32_t pc{};
        std::uint16_t opcode{};
        std::uint64_t count{};
        std::uint32_t first_frame{};
        std::uint32_t last_frame{};
        std::unordered_map<std::uint32_t, std::uint64_t> callers;
        std::array<std::uint64_t, 8> irq_masks{};
    };

    static bool is_idle_pc(unsigned pc) noexcept;
    static bool is_experimental_native_pc(unsigned pc) noexcept;

    bool enabled_{false};
    bool finished_{false};
    std::filesystem::path output_dir_;
    std::string scenario_{"scenario_c"};
    ResidualProfileSummary summary_{};
    std::unordered_map<std::uint32_t, InternalRecord> records_;
};

} // namespace oasis::hybrid
