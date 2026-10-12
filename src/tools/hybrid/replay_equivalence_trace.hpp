#pragma once

#include "tools/hybrid/gameplay_session_cpu.hpp"

#include <fstream>
#include <span>
#include <string>
#include <string_view>

namespace oasis::hybrid::session {

struct SchedulerSnapshot {
    unsigned m68k_cycles{};
    unsigned refresh_cycles{};
    unsigned frame_cycle_end{};
    unsigned irq_level{};
    unsigned irq_mask{};
    unsigned z80_cycles{};
    unsigned vdp_cycle{};
    unsigned scanline{};
    unsigned z80_pc{};
    unsigned z80_irq_state{};
};

using SchedulerRead = unsigned (*)(unsigned);
using CopyObservableMemory = int (*)(unsigned char*, unsigned);
inline constexpr unsigned observable_memory_snapshot_size = 0x10000U + 0x2000U + 0x10000U + 0x80U + 0x80U + 0x20U + 0x400U;

[[nodiscard]] SchedulerSnapshot read_scheduler_snapshot(SchedulerRead read);

class ReplayEquivalenceTrace {
public:
    explicit ReplayEquivalenceTrace(const std::string& output_dir);
    void observe(unsigned type, int width, unsigned address, unsigned value,
                 unsigned previous_pc, unsigned frame, const CpuSnapshot& cpu,
                 const SchedulerSnapshot& scheduler, std::string_view memory_hash_json,
                 std::span<const std::uint8_t> z80_ram);
    void observe_frame(unsigned frame, RegisterRead registers, SchedulerRead scheduler,
                       CopyObservableMemory copy_memory);
    void mark_native_route(unsigned target_pc);
    void write_receipt(const std::string& path) const;
    [[nodiscard]] bool capturing_external_window() const noexcept { return selected_window_; }
    [[nodiscard]] bool wants_memory_snapshot(unsigned type, unsigned address,
                                             unsigned previous_pc) const noexcept;

private:
    void write_boundary(std::string_view phase, unsigned frame, const CpuSnapshot& cpu,
                        const SchedulerSnapshot& scheduler, std::string_view memory_hash_json,
                        std::span<const std::uint8_t> z80_ram);
    std::ofstream semantic_events_;
    std::ofstream call_timing_;
    std::ofstream architectural_sync_;
    std::ofstream timing_sync_;
    std::ofstream external_events_;
    std::ofstream call_boundaries_;
    std::ofstream z80_ram_samples_;
    std::ofstream call_z80_samples_;
    std::ofstream full_frame_state_;
    unsigned call_count_{};
    unsigned native_entry_count_{};
    unsigned active_call_{};
    unsigned selected_call_{1U};
    unsigned sync_remaining_{};
    bool call_active_{};
    bool scheduler_boundary_pending_{};
    bool selected_window_{};
    bool close_window_after_events_{};
    bool native_selected_{};
    bool native_call_marked_{};
    CpuSnapshot call_entry_{};
    SchedulerSnapshot call_entry_scheduler_{};
    SchedulerSnapshot call_return_scheduler_{};
    CpuSnapshot transfer_source_{};
    SchedulerSnapshot transfer_source_scheduler_{};
    bool transfer_source_pending_{};
    unsigned memory_sample_count_{};
};

} // namespace oasis::hybrid::session
