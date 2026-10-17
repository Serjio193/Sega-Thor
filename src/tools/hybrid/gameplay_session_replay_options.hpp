#pragma once

#include <string>
#include <string_view>
#include <vector>

namespace oasis::hybrid::session {

struct ReplayOptions {
    bool restore_initial_state{};
    bool diagnostic{};
    bool native_island{};
    std::string native_helper;
    // Extra PC_WATCH entry PCs (`--watch-pcs=0x..,0x..`). Observation only;
    // requires --normalized-rom because the calls.jsonl identity changes.
    std::vector<unsigned> watch_pcs;
};

[[nodiscard]] ReplayOptions parse_replay_options(
    int argc, char** argv, bool (*supported_helper)(std::string_view));

} // namespace oasis::hybrid::session
