#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <time.h>
#include <unordered_map>
#include <vector>

namespace oasis::profiler {

inline std::uint64_t now_cpu_ns() noexcept {
#ifdef __linux__
    struct timespec ts{};
    clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<std::uint64_t>(ts.tv_nsec);
#else
    return 0;
#endif
}

inline std::uint64_t now_wall_ns() noexcept {
#ifdef __linux__
    struct timespec ts{};
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return static_cast<std::uint64_t>(ts.tv_sec) * 1000000000ULL + static_cast<std::uint64_t>(ts.tv_nsec);
#else
    return 0;
#endif
}

struct HostProfileMetrics {
    std::uint64_t total_cpu_ns{0};
    std::uint64_t total_wall_ns{0};
    std::uint64_t musashi_cpu_ns{0};
    std::uint64_t production_native_cpu_ns{0};
    std::uint64_t portable_cpu_ns{0};
    std::uint64_t bus_cpu_ns{0};
    std::uint64_t scheduler_cpu_ns{0};
    std::uint64_t vdp_cpu_ns{0};
    std::uint64_t audio_z80_cpu_ns{0};
    std::uint64_t input_replay_evidence_cpu_ns{0};
    std::uint64_t other_runner_cpu_ns{0};
    std::uint64_t profiler_overhead_ns{0};
    std::uint64_t total_frames{0};
    std::uint64_t idle_cpu_ns{0};
};

class HostCpuProfiler {
public:
    static HostCpuProfiler& instance() noexcept {
        static HostCpuProfiler inst;
        return inst;
    }

    void init(const std::filesystem::path& out_dir, const std::string& scenario) {
        if (enabled_) return;
        out_dir_ = out_dir;
        scenario_ = scenario;
        enabled_ = true;
        metrics_ = HostProfileMetrics{};
        start_cpu_ns_ = now_cpu_ns();
        start_wall_ns_ = now_wall_ns();
    }

    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

    void on_frame_start() noexcept {
        if (!enabled_) return;
        frame_cpu_start_ = now_cpu_ns();
        ++metrics_.total_frames;
    }

    void on_frame_run_end() noexcept {
        if (!enabled_) return;
        frame_run_end_ = now_cpu_ns();
    }

    void on_frame_end() noexcept {
        if (!enabled_) return;
        const auto now = now_cpu_ns();
        if (frame_run_end_ > 0 && now >= frame_run_end_) {
            metrics_.input_replay_evidence_cpu_ns += (now - frame_run_end_);
        }
    }

    void on_sched_event(int kind, int, int) noexcept {
        if (!enabled_) return;
        const auto now = now_cpu_ns();
        // HYBRID_SCHED_M68K_BEGIN = 1, HYBRID_SCHED_M68K_END = 2
        // HYBRID_SCHED_Z80_BEGIN = 3, HYBRID_SCHED_Z80_END = 4
        // HYBRID_SCHED_VDP_ADVANCE = 5, HYBRID_SCHED_IRQ_PENDING = 6
        // HYBRID_SCHED_Z80_IRQ = 7
        if (kind == 1) { // M68K BEGIN
            m68k_start_ = now;
        } else if (kind == 2) { // M68K END
            if (m68k_start_ > 0 && now >= m68k_start_) {
                total_m68k_cpu_ += (now - m68k_start_);
            }
            m68k_start_ = 0;
        } else if (kind == 3) { // Z80 BEGIN
            z80_start_ = now;
        } else if (kind == 4) { // Z80 END
            if (z80_start_ > 0 && now >= z80_start_) {
                metrics_.audio_z80_cpu_ns += (now - z80_start_);
            }
            z80_start_ = 0;
        } else if (kind == 5) { // VDP ADVANCE
            metrics_.vdp_cpu_ns += 40; // small attributed increment
        } else if (kind == 6 || kind == 7) { // IRQ
            metrics_.scheduler_cpu_ns += 30;
        }
    }

    void record_native_step(std::uint64_t dur_ns, bool is_production) noexcept {
        if (!enabled_) return;
        if (is_production) {
            metrics_.production_native_cpu_ns += dur_ns;
        } else {
            metrics_.portable_cpu_ns += dur_ns;
        }
    }

    void record_bus_access(std::uint64_t dur_ns) noexcept {
        if (!enabled_) return;
        metrics_.bus_cpu_ns += dur_ns;
    }

    void record_idle_spin(std::uint64_t dur_ns) noexcept {
        if (!enabled_) return;
        metrics_.idle_cpu_ns += dur_ns;
    }

    void finish() {
        if (!enabled_ || finished_) return;
        finished_ = true;
        const auto end_cpu = now_cpu_ns();
        const auto end_wall = now_wall_ns();
        metrics_.total_cpu_ns = (end_cpu >= start_cpu_ns_) ? (end_cpu - start_cpu_ns_) : 0;
        metrics_.total_wall_ns = (end_wall >= start_wall_ns_) ? (end_wall - start_wall_ns_) : 0;

        // Attributions
        const std::uint64_t native_total = metrics_.production_native_cpu_ns + metrics_.portable_cpu_ns;
        if (total_m68k_cpu_ >= native_total) {
            metrics_.musashi_cpu_ns = total_m68k_cpu_ - native_total;
        } else {
            metrics_.musashi_cpu_ns = total_m68k_cpu_;
        }

        const std::uint64_t accounted = metrics_.musashi_cpu_ns + metrics_.production_native_cpu_ns +
            metrics_.portable_cpu_ns + metrics_.audio_z80_cpu_ns + metrics_.vdp_cpu_ns +
            metrics_.scheduler_cpu_ns + metrics_.input_replay_evidence_cpu_ns;

        if (metrics_.total_cpu_ns >= accounted) {
            metrics_.other_runner_cpu_ns = metrics_.total_cpu_ns - accounted;
        }

        // Write summary
        if (!out_dir_.empty()) {
            std::filesystem::create_directories(out_dir_);
            const auto file_path = out_dir_ / ("host_cpu_profile_" + scenario_ + ".json");
            std::ofstream out(file_path);
            if (out) {
                const double total = static_cast<double>(metrics_.total_cpu_ns ? metrics_.total_cpu_ns : 1);
                out << "{\n"
                    << "  \"scenario\": \"" << scenario_ << "\",\n"
                    << "  \"total_frames\": " << metrics_.total_frames << ",\n"
                    << "  \"total_cpu_ns\": " << metrics_.total_cpu_ns << ",\n"
                    << "  \"total_wall_ns\": " << metrics_.total_wall_ns << ",\n"
                    << "  \"musashi_cpu_ns\": " << metrics_.musashi_cpu_ns << ",\n"
                    << "  \"production_native_cpu_ns\": " << metrics_.production_native_cpu_ns << ",\n"
                    << "  \"portable_cpu_ns\": " << metrics_.portable_cpu_ns << ",\n"
                    << "  \"bus_cpu_ns\": " << metrics_.bus_cpu_ns << ",\n"
                    << "  \"scheduler_cpu_ns\": " << metrics_.scheduler_cpu_ns << ",\n"
                    << "  \"vdp_cpu_ns\": " << metrics_.vdp_cpu_ns << ",\n"
                    << "  \"audio_z80_cpu_ns\": " << metrics_.audio_z80_cpu_ns << ",\n"
                    << "  \"input_replay_evidence_cpu_ns\": " << metrics_.input_replay_evidence_cpu_ns << ",\n"
                    << "  \"other_runner_cpu_ns\": " << metrics_.other_runner_cpu_ns << ",\n"
                    << "  \"idle_cpu_ns\": " << metrics_.idle_cpu_ns << ",\n"
                    << "  \"percentages\": {\n"
                    << "    \"musashi\": " << (metrics_.musashi_cpu_ns * 100.0 / total) << ",\n"
                    << "    \"production_native\": " << (metrics_.production_native_cpu_ns * 100.0 / total) << ",\n"
                    << "    \"portable\": " << (metrics_.portable_cpu_ns * 100.0 / total) << ",\n"
                    << "    \"bus\": " << (metrics_.bus_cpu_ns * 100.0 / total) << ",\n"
                    << "    \"scheduler\": " << (metrics_.scheduler_cpu_ns * 100.0 / total) << ",\n"
                    << "    \"vdp\": " << (metrics_.vdp_cpu_ns * 100.0 / total) << ",\n"
                    << "    \"audio_z80\": " << (metrics_.audio_z80_cpu_ns * 100.0 / total) << ",\n"
                    << "    \"input_replay_evidence\": " << (metrics_.input_replay_evidence_cpu_ns * 100.0 / total) << ",\n"
                    << "    \"other\": " << (metrics_.other_runner_cpu_ns * 100.0 / total) << "\n"
                    << "  }\n"
                    << "}\n";
            }
        }
    }

    [[nodiscard]] const HostProfileMetrics& metrics() const noexcept { return metrics_; }

private:
    HostCpuProfiler() = default;

    bool enabled_{false};
    bool finished_{false};
    std::filesystem::path out_dir_;
    std::string scenario_{"scenario_a"};
    std::uint64_t start_cpu_ns_{0};
    std::uint64_t start_wall_ns_{0};
    std::uint64_t frame_cpu_start_{0};
    std::uint64_t frame_run_end_{0};
    std::uint64_t m68k_start_{0};
    std::uint64_t z80_start_{0};
    std::uint64_t total_m68k_cpu_{0};
    HostProfileMetrics metrics_{};
};

} // namespace oasis::profiler
