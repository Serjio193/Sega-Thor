#include "tools/hybrid/gameplay_session_replay_diagnostic.hpp"

#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace oasis::hybrid::session {
namespace {
std::string quote_json(const std::string& value) {
    std::ostringstream out;
    out << '"';
    for (const auto ch : value) {
        if (ch == '"' || ch == '\\') out << '\\' << ch;
        else if (ch == '\n') out << "\\n";
        else if (ch == '\r') out << "\\r";
        else out << ch;
    }
    out << '"';
    return out.str();
}
}

ReplayLandmarkDiagnostic::ReplayLandmarkDiagnostic(const std::string& output_path)
    : observed_(output_path, std::ios::trunc) {
    if (!observed_) throw std::runtime_error("diagnostic landmark output open failed");
}

bool ReplayLandmarkDiagnostic::observe(std::ifstream& expected, const std::string& actual,
                                       unsigned frame) {
    std::string expected_line;
    const bool have_expected = static_cast<bool>(std::getline(expected, expected_line));
    observed_ << actual << '\n';
    observed_.flush();
    const bool equal = have_expected && expected_line == actual;
    if (!equal) {
        ++differing_frames_;
        if (!first_frame_) {
            first_frame_ = frame;
            expected_first_ = have_expected ? expected_line : "<missing expected frame>";
            actual_first_ = actual;
        }
    }
    return equal;
}

bool ReplayLandmarkDiagnostic::has_divergence() const noexcept {
    return differing_frames_ != 0;
}

void ReplayLandmarkDiagnostic::write_receipt(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out) throw std::runtime_error("diagnostic landmark receipt open failed");
    out << "{\"schema\":\"oasis.replay-landmark-diagnostic.v1\",\"first_different_frame\":"
        << first_frame_ << ",\"different_frame_count\":" << differing_frames_
        << ",\"expected\":" << quote_json(expected_first_)
        << ",\"observed\":" << quote_json(actual_first_) << "}\n";
}

void write_native_island_metrics(const std::string& path, const std::string& status,
    const std::string& reason, std::uint64_t transfers,
    const native_island::Counters& counters, unsigned first_frame, unsigned last_frame,
    bool replay_divergence) {
    if (path.empty()) return;
    std::ofstream out(path, std::ios::trunc);
    if (!out) return;
    out << "{\"schema\":\"oasis.replay-native-island-attempt.v1\",\"status\":\""
        << status << "\",\"reason\":\"" << reason << "\",\"total_m68k_transfers\":" << transfers
        << ",\"eligible_helper_calls\":" << counters.eligible_calls
        << ",\"native_executed_calls\":" << counters.native_calls
        << ",\"rom_fallback_calls\":" << counters.rom_fallbacks
        << ",\"provenance_rejects\":" << counters.provenance_rejects
        << ",\"precondition_rejects\":" << counters.precondition_rejects
        << ",\"contract_rejects\":" << counters.contract_rejects
        << ",\"native_mismatches\":" << counters.mismatches
        << ",\"replay_divergence\":" << (replay_divergence ? "true" : "false")
        << ",\"helper\":{\"entry_pc\":\"0x009D70\",\"caller_pc\":\"0x009144\",\"native_call_count\":"
        << counters.native_calls << ",\"rom_fallback_count\":" << counters.rom_fallbacks
        << ",\"first_frame\":" << first_frame << ",\"last_frame\":" << last_frame << "}}\n";
}

bool is_call_transfer_instruction(unsigned pc, int (*peek)(unsigned)) noexcept {
    if (!peek) return false;
    const auto high = peek(pc & 0xFFFFFFU);
    const auto low = peek((pc + 1U) & 0xFFFFFFU);
    if (high < 0 || low < 0) return false;
    const auto opcode = (static_cast<unsigned>(high & 0xFF) << 8U) |
        static_cast<unsigned>(low & 0xFF);
    return (opcode & 0xFF00U) == 0x6100U ||
        (opcode >= 0x4E90U && opcode <= 0x4EBBU &&
         ((opcode & 0xFFF8U) == 0x4E90U || (opcode & 0xFFC0U) == 0x4E80U));
}

} // namespace oasis::hybrid::session
