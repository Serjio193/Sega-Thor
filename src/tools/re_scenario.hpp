#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace oasis::tools {

struct EmulatorScenarioInput {
    std::uint64_t frame{};
    std::uint8_t port{};
    std::vector<std::string> buttons;
};

struct EmulatorScenario {
    std::string scenario_id;
    std::string rom_sha256;
    std::string backend;
    std::string start_state;
    std::string stop_condition;
    std::vector<std::uint32_t> target_addresses;
    std::vector<EmulatorScenarioInput> input_events;
};

[[nodiscard]] EmulatorScenario parse_emulator_scenario(std::string_view text);
[[nodiscard]] std::string scenario_to_json(const EmulatorScenario& scenario);

} // namespace oasis::tools
