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
        } else if (option.starts_with("--watch-pcs=")) {
            std::string list = option.substr(12);
            for (std::size_t at = 0; at < list.size();) {
                const auto comma = list.find(',', at);
                const auto item = list.substr(at, comma == std::string::npos ? std::string::npos : comma - at);
                std::size_t used{};
                const auto pc = std::stoul(item, &used, 16);
                if (item.empty() || used != item.size() || pc > 0xFFFFFFUL)
                    throw std::runtime_error("invalid --watch-pcs entry");
                result.watch_pcs.push_back(static_cast<unsigned>(pc));
                at = comma == std::string::npos ? list.size() : comma + 1;
            }
        } else {
            throw std::runtime_error("replay option must be --native-island=<helper-set>, --watch-pcs=<pcs> or --normalized-rom");
        }
    }
    if (!result.watch_pcs.empty() && !result.diagnostic)
        throw std::runtime_error("--watch-pcs requires --normalized-rom diagnostic replay");
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
