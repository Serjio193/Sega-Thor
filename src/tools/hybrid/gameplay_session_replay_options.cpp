#include "tools/hybrid/gameplay_session_replay_options.hpp"

#include <stdexcept>

namespace oasis::hybrid::session {

ReplayOptions parse_replay_options(int argc, char** argv,
                                   bool (*supported_helper)(std::string_view)) {
    ReplayOptions result;
    for (int index = 5; index < argc; ++index) {
        const std::string option = argv[index];
        constexpr std::string_view prefix = "--native-island=";
        if (option == "--restore-initial-state") {
            result.restore_initial_state = true;
        } else if (option == "--normalized-rom") {
            result.diagnostic = true;
        } else if (option.starts_with(prefix)) {
            result.native_helper = option.substr(prefix.size());
        } else {
            throw std::runtime_error("replay option must be --native-island=<helper-set> or --normalized-rom");
        }
    }
    if (result.native_helper == "9d70-diagnostic") {
        result.native_helper = "9d70";
        result.diagnostic = true;
    }
    const bool step_set = result.native_helper.starts_with("step:");
    if (!result.native_helper.empty() && result.native_helper != "9d70-step" &&
        !step_set && (!supported_helper || !supported_helper(result.native_helper)))
        throw std::runtime_error("native replay adapter is not available for selected helper");
    result.native_island = result.native_helper == "9d70";
    return result;
}

} // namespace oasis::hybrid::session
