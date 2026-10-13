#pragma once

#include <string>
#include <string_view>

namespace oasis::hybrid::session {

struct ReplayOptions {
    bool restore_initial_state{};
    bool diagnostic{};
    bool native_island{};
    std::string native_helper;
};

[[nodiscard]] ReplayOptions parse_replay_options(
    int argc, char** argv, bool (*supported_helper)(std::string_view));

} // namespace oasis::hybrid::session
