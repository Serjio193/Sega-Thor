#include "tools/hybrid/gameplay_session_receipts.hpp"

#include <fstream>
#include <stdexcept>

namespace oasis::hybrid::session {

void write_replay_result(const std::filesystem::path& path, unsigned frames,
                         std::size_t polls, const std::string& input_sha256,
                         const std::string& landmarks_sha256,
                         const std::string& calls_sha256,
                         const std::string& census_sha256,
                         const std::string& full_calls_sha256) {
    std::ofstream result(path, std::ios::trunc);
    if (!result) throw std::runtime_error("replay result receipt open failed");
    result << "OASIS_GAMEPLAY_REPLAY_RESULT 1\nstatus=PASS\nframes=" << frames
           << "\npolls=" << polls << "\ninput_sha256=" << input_sha256
           << "\nlandmarks_sha256=" << landmarks_sha256
           << "\ncalls_sha256=" << calls_sha256
           << "\ncall_census_sha256=" << census_sha256
           << "\nauto_full_calls_sha256=" << full_calls_sha256 << '\n';
}

} // namespace oasis::hybrid::session
