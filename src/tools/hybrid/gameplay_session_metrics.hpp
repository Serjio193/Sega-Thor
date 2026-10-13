#pragma once

#include "tools/hybrid/gameplay_session_replay_diagnostic.hpp"

#include <filesystem>
#include <string>

namespace oasis::hybrid::session {

void write_gameplay_step_metrics(const std::filesystem::path& path,
    const std::string& status, const std::string& reason, std::uint64_t transfers,
    const native_island::Counters& counters, unsigned first_frame, unsigned last_frame,
    bool replay_divergence);

} // namespace oasis::hybrid::session
