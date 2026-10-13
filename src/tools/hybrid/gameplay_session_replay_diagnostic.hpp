#pragma once

#include <fstream>
#include <string>
#include "tools/hybrid/native_island_runtime.hpp"

namespace oasis::hybrid::session {

class ReplayLandmarkDiagnostic {
public:
    explicit ReplayLandmarkDiagnostic(const std::string& output_path);
    [[nodiscard]] bool observe(std::ifstream& expected, const std::string& actual,
                               unsigned frame);
    void write_receipt(const std::string& path) const;
    [[nodiscard]] bool has_divergence() const noexcept;

private:
    std::ofstream observed_;
    unsigned first_frame_{};
    std::string expected_first_;
    std::string actual_first_;
    unsigned differing_frames_{};
};

void write_native_island_metrics(const std::string& path, const std::string& status,
    const std::string& reason, std::uint64_t transfers,
    const native_island::Counters& counters, unsigned first_frame, unsigned last_frame,
    bool replay_divergence);
[[nodiscard]] bool is_call_transfer_instruction(unsigned pc, int (*peek)(unsigned)) noexcept;

} // namespace oasis::hybrid::session
