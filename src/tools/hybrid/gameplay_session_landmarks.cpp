#include "tools/hybrid/gameplay_session_landmarks.hpp"

#include "tools/hybrid/m68k_call_census.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid::session {

std::uint64_t append_pc_fingerprint(std::uint64_t hash, unsigned pc) noexcept {
    for (unsigned shift = 0; shift < 32; shift += 8) {
        hash ^= (pc >> shift) & 0xFFU;
        hash *= 1099511628211ULL;
    }
    return hash;
}

std::string serialize_frame_landmark(const FrameLandmark& value) {
    std::ostringstream out;
    out << "{\"frame\":" << value.frame << ",\"polls\":" << value.polls
        << ",\"instructions\":" << value.instructions
        << ",\"pc_fingerprint\":\"" << std::hex << value.pc_fingerprint
        << std::dec << "\",\"end_pc\":" << value.end_pc << ",\"watched\":{";
    bool first = true;
    if (value.watched) for (const auto& [pc, count] : *value.watched) {
        out << (first ? "" : ",") << '\"'
            << oasis::hybrid::call_census::format_hex(pc, 8) << "\":" << count;
        first = false;
    }
    out << '}';
    if (!value.state_sha256.empty()) out << ",\"state_sha256\":\"" << value.state_sha256 << '\"';
    if (!value.ram_sha256.empty()) out << ",\"ram_sha256\":\"" << value.ram_sha256 << '\"'
        << ",\"globals\":[" << value.grid_selector << ',' << value.object_state << ','
        << value.grid_descriptor << ']';
    out << ",\"cross_cpu_events\":null}";
    return out.str();
}

void compare_frame_landmark(std::ifstream& expected, const std::string& actual,
                            unsigned frame) {
    std::string line;
    if (!std::getline(expected, line))
        throw std::runtime_error("extra frame during replay at " + std::to_string(frame));
    if (line != actual)
        throw std::runtime_error("landmark divergence at frame " + std::to_string(frame));
}

} // namespace oasis::hybrid::session
