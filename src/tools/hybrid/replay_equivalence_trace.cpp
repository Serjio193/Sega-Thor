#include "tools/hybrid/replay_equivalence_trace.hpp"

#include "core/rom_identity.hpp"

#include <cstdint>
#include <stdexcept>
#include <vector>

namespace oasis::hybrid::session {
namespace {
constexpr unsigned execute_event = 1U;
constexpr unsigned read_event = 2U;
constexpr unsigned write_event = 4U;
constexpr unsigned vdp_write_events = (1U << 4U) | (1U << 6U) | (1U << 8U);
constexpr unsigned target_pc = 0x9D70U;
constexpr unsigned caller_pc = 0x9144U;
constexpr unsigned return_pc = 0x9148U;
constexpr unsigned keep_rts_pc = 0x9D86U;
constexpr unsigned write_rts_pc = 0x9D90U;
constexpr char contract_id[] = "M68K_009D70_CALLER_009144_V1";

void write_array(std::ofstream& out, const std::array<unsigned, 8>& values) {
    out << '[';
    for (unsigned i = 0; i < values.size(); ++i) out << (i ? "," : "") << values[i];
    out << ']';
}

void write_architecture(std::ofstream& out, unsigned frame, const CpuSnapshot& cpu,
                        std::string_view memory_hash_json) {
    out << "{\"frame\":" << frame << ",\"pc\":" << cpu.pc << ",\"sr\":" << cpu.sr << ",\"d\":";
    write_array(out, cpu.d);
    out << ",\"a\":";
    write_array(out, cpu.a);
    out << ",\"guest_memory_sha256\":" << memory_hash_json << "}\n";
}

void write_timing(std::ofstream& out, unsigned frame, const CpuSnapshot& cpu,
                  const SchedulerSnapshot& s) {
    out << "{\"frame\":" << frame << ",\"pc\":" << cpu.pc
        << ",\"m68k_cycles\":" << s.m68k_cycles
        << ",\"refresh_cycles\":" << s.refresh_cycles
        << ",\"frame_cycle_end\":" << s.frame_cycle_end
        << ",\"irq_level\":" << s.irq_level << ",\"irq_mask\":" << s.irq_mask
        << ",\"z80_cycles\":" << s.z80_cycles << ",\"vdp_cycle\":" << s.vdp_cycle
        << ",\"scanline\":" << s.scanline << ",\"z80_pc\":" << s.z80_pc
        << ",\"z80_irq_state\":" << s.z80_irq_state << "}\n";
}
}

SchedulerSnapshot read_scheduler_snapshot(SchedulerRead read) {
    return {read(0), read(1), read(2), read(3), read(4), read(5), read(6),
            read(7), read(8), read(9)};
}

ReplayEquivalenceTrace::ReplayEquivalenceTrace(const std::string& output_dir) {
    semantic_events_.open(output_dir + "/normalized-events.jsonl", std::ios::trunc);
    call_timing_.open(output_dir + "/helper-timing.jsonl", std::ios::trunc);
    architectural_sync_.open(output_dir + "/architectural-sync.jsonl", std::ios::trunc);
    timing_sync_.open(output_dir + "/scheduler-sync.jsonl", std::ios::trunc);
    external_events_.open(output_dir + "/external-events.jsonl", std::ios::trunc);
    call_boundaries_.open(output_dir + "/call-boundaries.jsonl", std::ios::trunc);
    z80_ram_samples_.open(output_dir + "/z80-ram-sync.bin", std::ios::binary | std::ios::trunc);
    call_z80_samples_.open(output_dir + "/call-z80-ram.bin", std::ios::binary | std::ios::trunc);
    full_frame_state_.open(output_dir + "/full-frame-state.jsonl", std::ios::trunc);
    if (!semantic_events_ || !call_timing_ || !architectural_sync_ || !timing_sync_ ||
        !external_events_ || !call_boundaries_ || !z80_ram_samples_ || !call_z80_samples_ ||
        !full_frame_state_)
        throw std::runtime_error("normalized replay trace output open failed");
}

void ReplayEquivalenceTrace::observe_frame(unsigned frame, RegisterRead registers,
                                            SchedulerRead scheduler,
                                            CopyObservableMemory copy_memory) {
    std::vector<std::uint8_t> memory(observable_memory_snapshot_size);
    if (!copy_memory(memory.data(), static_cast<unsigned>(memory.size())))
        throw std::runtime_error("full-frame observable memory capture failed");
    const auto cpu = read_cpu_snapshot(registers);
    const auto s = read_scheduler_snapshot(scheduler);
    full_frame_state_ << "{\"frame\":" << frame << ",\"cpu\":"
        << serialize_cpu_snapshot(cpu) << ",\"scheduler\":[" << s.m68k_cycles
        << ',' << s.refresh_cycles << ',' << s.frame_cycle_end << ',' << s.irq_level
        << ',' << s.irq_mask << ',' << s.z80_cycles << ',' << s.vdp_cycle << ','
        << s.scanline << ',' << s.z80_pc << ',' << s.z80_irq_state
        << "],\"observable_memory_sha256\":\"" << calculate_sha256(memory) << "\"}\n";
}

void ReplayEquivalenceTrace::observe(unsigned type, int width, unsigned address, unsigned value,
                                     unsigned previous_pc, unsigned frame, const CpuSnapshot& cpu,
                                     const SchedulerSnapshot& scheduler, std::string_view memory_hash_json,
                                     std::span<const std::uint8_t> z80_ram) {
    if ((type & execute_event) != 0U) {
        if (scheduler_boundary_pending_ &&
            (scheduler.frame_cycle_end != call_return_scheduler_.frame_cycle_end ||
             scheduler.vdp_cycle != call_return_scheduler_.vdp_cycle ||
             scheduler.z80_cycles != call_return_scheduler_.z80_cycles)) {
            write_boundary("NEXT_SCHEDULER_BOUNDARY", frame, cpu, scheduler,
                           memory_hash_json, z80_ram);
            scheduler_boundary_pending_ = false;
        }
        if (close_window_after_events_) {
            selected_window_ = false;
            close_window_after_events_ = false;
        }
        if (address == caller_pc) {
            transfer_source_ = cpu;
            transfer_source_scheduler_ = scheduler;
            transfer_source_pending_ = true;
        }
        if (address == target_pc && previous_pc == caller_pc) {
            if (!transfer_source_pending_)
                throw std::runtime_error("normalized helper entry lacks immediately preceding caller state");
            ++call_count_;
            active_call_ = call_count_;
            call_active_ = true;
            call_entry_ = cpu;
            call_entry_scheduler_ = scheduler;
            native_selected_ = false;
            native_call_marked_ = false;
            transfer_source_pending_ = false;
            semantic_events_ << "{\"event\":\"HELPER_CALL\",\"id\":\"0x009D70\",\"caller\":\"0x009144\",\"return\":\"0x009148\",\"contract\":\""
                             << contract_id << "\",\"sequence\":" << call_count_ << ",\"frame\":" << frame << "}\n";
            write_boundary("ENTRY", frame, cpu, scheduler, memory_hash_json, z80_ram);
        }
        if (call_active_ && active_call_ == selected_call_ &&
            (address == keep_rts_pc || address == write_rts_pc)) {
            write_boundary("RTS_PRE", frame, cpu, scheduler, memory_hash_json, z80_ram);
        }
        if (call_active_ && address == return_pc) {
            write_boundary("RETURN", frame, cpu, scheduler, memory_hash_json, z80_ram);
            call_return_scheduler_ = scheduler;
            scheduler_boundary_pending_ = active_call_ == selected_call_ &&
                scheduler.frame_cycle_end == call_entry_scheduler_.frame_cycle_end &&
                scheduler.vdp_cycle == call_entry_scheduler_.vdp_cycle &&
                scheduler.z80_cycles == call_entry_scheduler_.z80_cycles;
            if (!scheduler_boundary_pending_ && active_call_ == selected_call_)
                write_boundary("NEXT_SCHEDULER_BOUNDARY", frame, cpu, scheduler,
                               memory_hash_json, z80_ram);
            semantic_events_ << "{\"event\":\"HELPER_RETURN\",\"id\":\"0x009D70\",\"caller\":\"0x009144\",\"return\":\"0x009148\",\"contract\":\""
                             << contract_id << "\",\"sequence\":" << active_call_ << ",\"frame\":" << frame << "}\n";
            call_timing_ << "{\"sequence\":" << active_call_ << ",\"frame\":" << frame
                << ",\"native\":" << (active_call_ == selected_call_ && native_selected_ ? "true" : "false")
                << ",\"source_cycle\":" << transfer_source_scheduler_.m68k_cycles
                << ",\"source_refresh_cycles\":" << transfer_source_scheduler_.refresh_cycles
                << ",\"source_cycle_end\":" << transfer_source_scheduler_.frame_cycle_end
                << ",\"source_z80_cycles\":" << transfer_source_scheduler_.z80_cycles
                << ",\"source_vdp_cycle\":" << transfer_source_scheduler_.vdp_cycle
                << ",\"source_to_return_cycle_cost\":" << static_cast<std::int32_t>(scheduler.m68k_cycles - transfer_source_scheduler_.m68k_cycles)
                << ",\"entry_pc\":" << call_entry_.pc << ",\"return_pc\":" << cpu.pc
                << ",\"entry_cycle\":" << call_entry_scheduler_.m68k_cycles
                << ",\"return_cycle\":" << scheduler.m68k_cycles
                << ",\"cycle_cost\":" << static_cast<std::int32_t>(scheduler.m68k_cycles - call_entry_scheduler_.m68k_cycles)
                << ",\"entry\":" << serialize_cpu_snapshot(call_entry_)
                << ",\"source\":" << serialize_cpu_snapshot(transfer_source_)
                << ",\"return\":" << serialize_cpu_snapshot(cpu) << "}\n";
            transfer_source_pending_ = false;
            if (active_call_ == selected_call_) native_selected_ = false;
            native_call_marked_ = false;
            if (active_call_ == selected_call_) {
                selected_window_ = true;
                sync_remaining_ = 128U;
            }
            call_active_ = false;
        }
        if (sync_remaining_ != 0U) {
            write_architecture(architectural_sync_, frame, cpu, memory_hash_json);
            write_timing(timing_sync_, frame, cpu, scheduler);
            if (z80_ram.size() != 0x2000U)
                throw std::runtime_error("normalized sync lacks full Z80 RAM snapshot");
            z80_ram_samples_.write(reinterpret_cast<const char*>(z80_ram.data()),
                                   static_cast<std::streamsize>(z80_ram.size()));
            ++memory_sample_count_;
            --sync_remaining_;
            if (sync_remaining_ == 0U) close_window_after_events_ = true;
        }
    } else if (sync_remaining_ != 0U && (type & (read_event | write_event | vdp_write_events)) != 0U) {
        external_events_ << "{\"frame\":" << frame << ",\"pc\":" << cpu.pc
            << ",\"type\":" << type << ",\"width\":" << width
            << ",\"address\":" << address << ",\"value\":" << value
            << ",\"m68k_cycles\":" << scheduler.m68k_cycles
            << ",\"z80_cycles\":" << scheduler.z80_cycles
            << ",\"vdp_cycle\":" << scheduler.vdp_cycle << "}\n";
    }
}

bool ReplayEquivalenceTrace::wants_memory_snapshot(unsigned type, unsigned address,
                                                   unsigned previous_pc) const noexcept {
    return selected_window_ || ((type & execute_event) != 0U &&
        ((address == target_pc && previous_pc == caller_pc && call_count_ + 1U == selected_call_) ||
         (call_active_ && active_call_ == selected_call_ &&
          (address == return_pc || address == keep_rts_pc || address == write_rts_pc) &&
          previous_pc != 0U) || scheduler_boundary_pending_));
}

void ReplayEquivalenceTrace::write_boundary(std::string_view phase, unsigned frame, const CpuSnapshot& cpu,
                                            const SchedulerSnapshot& scheduler, std::string_view memory_hash_json,
                                            std::span<const std::uint8_t> z80_ram) {
    if (active_call_ != selected_call_) return;
    if (z80_ram.size() != 0x2000U || memory_hash_json == "null")
        throw std::runtime_error("selected helper boundary lacks full memory snapshot");
    call_boundaries_ << "{\"sequence\":" << active_call_ << ",\"phase\":\"" << phase
        << "\",\"frame\":" << frame << ",\"cpu\":" << serialize_cpu_snapshot(cpu)
        << ",\"scheduler\":{\"m68k_cycles\":" << scheduler.m68k_cycles
        << ",\"refresh_cycles\":" << scheduler.refresh_cycles << ",\"frame_cycle_end\":" << scheduler.frame_cycle_end
        << ",\"irq_level\":" << scheduler.irq_level << ",\"irq_mask\":" << scheduler.irq_mask
        << ",\"z80_cycles\":" << scheduler.z80_cycles << ",\"vdp_cycle\":" << scheduler.vdp_cycle
        << ",\"scanline\":" << scheduler.scanline << ",\"z80_pc\":" << scheduler.z80_pc
        << ",\"z80_irq_state\":" << scheduler.z80_irq_state << "},\"memory_sha256\":"
        << memory_hash_json << "}\n";
    call_z80_samples_.write(reinterpret_cast<const char*>(z80_ram.data()),
                            static_cast<std::streamsize>(z80_ram.size()));
}

void ReplayEquivalenceTrace::mark_native_route(unsigned target) {
    if (target == target_pc && call_active_ && !native_call_marked_) {
        native_call_marked_ = true;
        ++native_entry_count_;
        if (active_call_ == selected_call_) native_selected_ = true;
    }
}

void ReplayEquivalenceTrace::write_receipt(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out) throw std::runtime_error("normalized replay trace receipt open failed");
    out << "{\"schema\":\"oasis.replay-native-normalized-trace.v1\",\"contract\":\""
        << contract_id << "\",\"normalized_call_count\":" << call_count_
        << ",\"selected_sequence\":" << selected_call_
        << ",\"native_entry_count\":" << native_entry_count_
        << ",\"architectural_sync_count\":" << memory_sample_count_
        << ",\"strict_rom_trace_identical\":false,\"timing_classification\":\"UNRESOLVED_PENDING_CROSS_RUN_COMPARE\"}\n";
}

} // namespace oasis::hybrid::session
