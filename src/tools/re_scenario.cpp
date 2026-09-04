#include "tools/re_scenario.hpp"

#include <algorithm>
#include <charconv>
#include <sstream>
#include <stdexcept>
#include <tuple>
#include <utility>

namespace oasis::tools {
namespace {

std::uint64_t number(std::string_view text, int base) {
    if (text.empty()) throw std::invalid_argument("empty scenario number");
    if (base == 16 && text.size() > 2U && text.substr(0, 2) == "0x") text.remove_prefix(2);
    std::uint64_t result{};
    const auto parsed = std::from_chars(text.data(), text.data() + text.size(), result, base);
    if (parsed.ec != std::errc{} || parsed.ptr != text.data() + text.size())
        throw std::invalid_argument("invalid scenario number");
    return result;
}

std::string value(std::string_view token, std::string_view key) {
    const auto prefix = std::string(key) + "=";
    if (!token.starts_with(prefix)) throw std::invalid_argument("expected scenario field: " + std::string(key));
    return std::string(token.substr(prefix.size()));
}

std::vector<std::string> split(std::string_view text, char separator) {
    std::vector<std::string> result;
    while (!text.empty()) {
        const auto end = text.find(separator);
        result.emplace_back(text.substr(0, end));
        if (end == std::string_view::npos) break;
        text.remove_prefix(end + 1U);
    }
    return result;
}

std::string json(std::string_view text) {
    std::string result = "\"";
    for (const char character : text) {
        if (character == '\\' || character == '"') result += '\\';
        result += character;
    }
    return result + '"';
}

} // namespace

EmulatorScenario parse_emulator_scenario(std::string_view text) {
    EmulatorScenario result;
    std::istringstream input{std::string(text)};
    bool header = false;
    for (std::string line; std::getline(input, line);) {
        const auto first = line.find_first_not_of(" \t\r");
        if (first == std::string::npos || line[first] == '#') continue;
        std::string_view content = std::string_view(line).substr(first);
        if (!content.empty() && content.back() == '\r') content.remove_suffix(1U);
        if (content == "oasis.m68k.emulator-scenario.v1") { header = true; continue; }
        if (!header) throw std::invalid_argument("scenario header must precede fields");
        std::istringstream fields{std::string(content)};
        std::vector<std::string> tokens;
        for (std::string token; fields >> token;) tokens.push_back(std::move(token));
        if (tokens.empty()) continue;
        if (tokens.front() == "input") {
            if (tokens.size() != 4U) throw std::invalid_argument("malformed scenario input");
            EmulatorScenarioInput event{.frame = number(value(tokens[1], "frame"), 10),
                                        .port = static_cast<std::uint8_t>(number(value(tokens[2], "port"), 10)),
                                        .buttons = split(value(tokens[3], "buttons"), '+')};
            if (event.port == 0U || event.buttons.empty()) throw std::invalid_argument("invalid scenario input");
            std::sort(event.buttons.begin(), event.buttons.end());
            result.input_events.push_back(std::move(event));
            continue;
        }
        if (tokens.size() != 1U) throw std::invalid_argument("malformed scenario field");
        const auto separator = tokens.front().find('=');
        if (separator == std::string::npos) throw std::invalid_argument("malformed scenario field");
        const auto key = tokens.front().substr(0, separator);
        const auto field = tokens.front().substr(separator + 1U);
        if (key == "scenario_id") result.scenario_id = field;
        else if (key == "rom_sha256") result.rom_sha256 = field;
        else if (key == "backend") result.backend = field;
        else if (key == "start_state") result.start_state = field;
        else if (key == "stop_condition") result.stop_condition = field;
        else if (key == "target_address") result.target_addresses.push_back(static_cast<std::uint32_t>(number(field, 16)));
        else throw std::invalid_argument("unknown scenario field: " + key);
    }
    if (!header || result.scenario_id.empty() || result.rom_sha256.empty() || result.backend.empty() ||
        result.start_state.empty() || result.stop_condition.empty() || result.target_addresses.empty())
        throw std::invalid_argument("incomplete emulator scenario");
    if (result.start_state != "hardware_reset") throw std::invalid_argument("unsupported scenario start state");
    std::sort(result.input_events.begin(), result.input_events.end(), [](const auto& left, const auto& right) {
        return std::tie(left.frame, left.port, left.buttons) < std::tie(right.frame, right.port, right.buttons);
    });
    return result;
}

std::string scenario_to_json(const EmulatorScenario& scenario) {
    std::ostringstream output;
    output << "{\"schema\":\"oasis.m68k.emulator-scenario.v1\",\"scenario_id\":" << json(scenario.scenario_id)
        << ",\"rom_sha256\":" << json(scenario.rom_sha256) << ",\"backend\":" << json(scenario.backend)
        << ",\"start_state\":" << json(scenario.start_state) << ",\"stop_condition\":" << json(scenario.stop_condition)
        << ",\"target_addresses\":[";
    for (std::size_t index = 0; index < scenario.target_addresses.size(); ++index) {
        if (index) output << ',';
        output << '"' << "0x" << std::hex << std::uppercase << scenario.target_addresses[index] << '"';
    }
    output << "],\"input_events\":[";
    for (std::size_t index = 0; index < scenario.input_events.size(); ++index) {
        if (index) output << ',';
        const auto& event = scenario.input_events[index];
        output << "{\"frame\":" << std::dec << event.frame << ",\"port\":" << static_cast<unsigned>(event.port) << ",\"buttons\":[";
        for (std::size_t button = 0; button < event.buttons.size(); ++button) {
            if (button) output << ',';
            output << json(event.buttons[button]);
        }
        output << "]}";
    }
    output << "]}";
    return output.str();
}

} // namespace oasis::tools
