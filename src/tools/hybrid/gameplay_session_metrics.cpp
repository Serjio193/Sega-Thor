#include "tools/hybrid/gameplay_session_metrics.hpp"

#include "tools/hybrid/gameplay_session_replay_diagnostic.hpp"
#include "tools/hybrid/m68k_gameplay_step_set.hpp"

namespace oasis::hybrid::session {

void write_gameplay_step_metrics(const std::filesystem::path& path,
    const std::string& status, const std::string& reason, std::uint64_t transfers,
    const native_island::Counters& counters, unsigned first_frame, unsigned last_frame,
    bool replay_divergence) {
    write_native_island_metrics(path.string(), status, reason, transfers, counters,
                                first_frame, last_frame, replay_divergence);
    if (!path.empty()) oasis::hybrid::write_m68k_gameplay_step_metrics(
        path.parent_path() / "e268-step-metrics.json");
}

} // namespace oasis::hybrid::session
