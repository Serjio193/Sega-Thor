#pragma once

#include <array>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <string>
#include <time.h>
#include <unistd.h>
#include <unordered_map>
#include <vector>

#if defined(__x86_64__) || defined(_M_X64)
#include <x86intrin.h>
#endif

namespace oasis::profiler {

inline std::uint64_t read_tsc() noexcept {
#if defined(__x86_64__) || defined(_M_X64)
    return __rdtsc();
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
    // Heavy kernel forensics
    std::uint64_t c8aa_entries{0};
    std::uint64_t c8aa_steps{0};
    std::uint64_t c8aa_cpu_ns{0};
    std::uint64_t d406_entries{0};
    std::uint64_t d406_steps{0};
    std::uint64_t d406_cpu_ns{0};
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

        // Calibrate TSC against monotonic clock
        const auto w0 = now_wall_ns();
        const auto t0 = read_tsc();
        usleep(10000); // 10ms calibration window
        const auto w1 = now_wall_ns();
        const auto t1 = read_tsc();
        const auto wall_delta = (w1 >= w0) ? (w1 - w0) : 1;
        const auto tsc_delta = (t1 >= t0) ? (t1 - t0) : 1;
        ns_per_tsc_ = static_cast<double>(wall_delta) / static_cast<double>(tsc_delta);

        start_tsc_ = read_tsc();
        start_cpu_ns_ = now_ns();
        start_wall_ns_ = now_wall_ns();
    }

    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

    inline std::uint64_t now_ns() const noexcept {
        const auto tsc = read_tsc();
        return static_cast<std::uint64_t>(static_cast<double>(tsc - start_tsc_) * ns_per_tsc_);
    }

    void on_frame_start() noexcept {
        if (!enabled_) return;
        frame_cpu_start_ = now_ns();
        ++metrics_.total_frames;
    }

    void on_frame_run_end() noexcept {
        if (!enabled_) return;
        frame_run_end_ = now_ns();
    }

    void on_frame_end() noexcept {
        if (!enabled_) return;
        const auto now = now_ns();
        if (frame_run_end_ > 0 && now >= frame_run_end_) {
            metrics_.input_replay_evidence_cpu_ns += (now - frame_run_end_);
        }
    }

    void on_sched_event(int kind, int, int) noexcept {
        if (!enabled_) return;
        const auto now = now_ns();
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
            metrics_.vdp_cpu_ns += 40;
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

    void record_musashi_pc(unsigned pc) noexcept {
        if (!enabled_) return;
        if (pc == 0x00C8AA) {
            ++metrics_.c8aa_entries;
            in_c8aa_ = true;
            c8aa_start_ = now_ns();
        } else if (pc == 0x00C90C || pc == 0x00C92A) {
            if (in_c8aa_) {
                const auto now = now_ns();
                if (now >= c8aa_start_) metrics_.c8aa_cpu_ns += (now - c8aa_start_);
                in_c8aa_ = false;
            }
        }
        if (in_c8aa_) ++metrics_.c8aa_steps;

        if (pc == 0x00D406) {
            ++metrics_.d406_entries;
            in_d406_ = true;
            d406_start_ = now_ns();
        } else if (pc == 0x00D7C0 || pc == 0x00D700) {
            if (in_d406_) {
                const auto now = now_ns();
                if (now >= d406_start_) metrics_.d406_cpu_ns += (now - d406_start_);
                in_d406_ = false;
            }
        }
        if (in_d406_) ++metrics_.d406_steps;
    }

    void finish() {
        if (!enabled_ || finished_) return;
        finished_ = true;
        const auto end_cpu = now_ns();
        const auto end_wall = now_wall_ns();
        metrics_.total_cpu_ns = (end_cpu >= start_cpu_ns_) ? (end_cpu - start_cpu_ns_) : 0;
        metrics_.total_wall_ns = (end_wall >= start_wall_ns_) ? (end_wall - start_wall_ns_) : 0;

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
                    << "  \"heavy_kernels\": {\n"
                    << "    \"c8aa\": {\n"
                    << "      \"entries\": " << metrics_.c8aa_entries << ",\n"
                    << "      \"steps\": " << metrics_.c8aa_steps << ",\n"
                    << "      \"cpu_ns\": " << metrics_.c8aa_cpu_ns << "\n"
                    << "    },\n"
                    << "    \"d406\": {\n"
                    << "      \"entries\": " << metrics_.d406_entries << ",\n"
                    << "      \"steps\": " << metrics_.d406_steps << ",\n"
                    << "      \"cpu_ns\": " << metrics_.d406_cpu_ns << "\n"
                    << "    }\n"
                    << "  },\n"
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
    std::uint64_t start_tsc_{0};
    double ns_per_tsc_{1.0};
    std::uint64_t start_cpu_ns_{0};
    std::uint64_t start_wall_ns_{0};
    std::uint64_t frame_cpu_start_{0};
    std::uint64_t frame_run_end_{0};
    std::uint64_t m68k_start_{0};
    std::uint64_t z80_start_{0};
    std::uint64_t total_m68k_cpu_{0};
    // Kernel tracking state
    bool in_c8aa_{false};
    std::uint64_t c8aa_start_{0};
    bool in_d406_{false};
    std::uint64_t d406_start_{0};
    HostProfileMetrics metrics_{};
};

inline std::uint64_t now_cpu_ns() noexcept {
    return HostCpuProfiler::instance().now_ns();
}

} // namespace oasis::profiler
