#pragma once

#include <cstdint>
#include <fstream>
#include <map>
#include <string>

namespace oasis::hybrid::session {

struct FrameLandmark {
    unsigned frame{};
    unsigned polls{};
    unsigned instructions{};
    std::uint64_t pc_fingerprint{};
    unsigned end_pc{};
    std::string state_sha256;
    std::string ram_sha256;
    unsigned grid_selector{};
    unsigned object_state{};
    unsigned grid_descriptor{};
    const std::map<unsigned, unsigned>* watched{};
};

[[nodiscard]] std::string serialize_frame_landmark(const FrameLandmark& value);
[[nodiscard]] std::uint64_t append_pc_fingerprint(std::uint64_t hash, unsigned pc) noexcept;
void compare_frame_landmark(std::ifstream& expected, const std::string& actual,
                            unsigned frame);

} // namespace oasis::hybrid::session
