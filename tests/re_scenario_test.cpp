#include "tools/re_scenario.hpp"

#include <cassert>
#include <stdexcept>

using namespace oasis::tools;

int main() {
    const auto scenario = parse_emulator_scenario(
        "oasis.m68k.emulator-scenario.v1\n"
        "scenario_id=synthetic_start\nrom_sha256=abc\nbackend=bizhawk-lua-bus\n"
        "start_state=hardware_reset\nstop_condition=max_frames:10\n"
        "target_address=0x6121A\ntarget_address=0x60BCC\n"
        "input frame=4 port=1 buttons=Start+Right\n"
        "input frame=2 port=1 buttons=Right\n");
    assert(scenario.input_events.size() == 2U && scenario.input_events[0].frame == 2U);
    assert(scenario.input_events[1].buttons[0] == "Right" && scenario.input_events[1].buttons[1] == "Start");
    const auto json = scenario_to_json(scenario);
    assert(json.find("oasis.m68k.emulator-scenario.v1") != std::string::npos);
    assert(json.find("\"frame\":2") != std::string::npos);
    bool failed = false;
    try { (void)parse_emulator_scenario("oasis.m68k.emulator-scenario.v1\nscenario_id=x\n"); }
    catch (const std::invalid_argument&) { failed = true; }
    assert(failed);
    failed = false;
    try { (void)parse_emulator_scenario(
        "oasis.m68k.emulator-scenario.v1\nscenario_id=x\nrom_sha256=a\nbackend=b\n"
        "start_state=artificial\nstop_condition=max_frames:1\ntarget_address=0x1\n"); }
    catch (const std::invalid_argument&) { failed = true; }
    assert(failed);
}
