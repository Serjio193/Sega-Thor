#pragma once

#include "tools/hybrid/caller_attribution.hpp"
#include <filesystem>

namespace oasis::hybrid {

void start_a5_lifetime_observer(CallerAttributionApi api);
void record_a5_lifetime_event(int type, int width, unsigned address, unsigned value,
                              unsigned frame, unsigned previous_execute_pc);
void finish_a5_lifetime_observer(const std::filesystem::path& output);
void stop_a5_lifetime_observer();

void start_callee_62ae0_observer(CallerAttributionApi api);
void record_callee_62ae0_event(int type, int width, unsigned address, unsigned value,
                               unsigned frame, unsigned previous_execute_pc);
void finish_callee_62ae0_observer(const std::filesystem::path& output);
void stop_callee_62ae0_observer();

} // namespace oasis::hybrid
