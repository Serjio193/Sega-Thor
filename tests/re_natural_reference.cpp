#include "core/rom.hpp"
#include "core/rom_identity.hpp"

#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

namespace {

std::string read_text(const char* path) {
    std::ifstream input(path);
    if (!input) throw std::runtime_error("cannot open oracle input");
    return {std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>()};
}

void require(bool condition, const char* message) {
    if (!condition) throw std::runtime_error(message);
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 4 && argc != 5) {
        std::cerr << "usage: oasis_re_natural_reference <USA ROM> <scenario> <natural report> [caller-search]\n";
        return 2;
    }
    try {
        const bool search_mode = argc == 5 && std::string(argv[4]) == "caller-search";
        require(argc != 5 || search_mode, "unknown oracle mode");
        const auto rom = oasis::Rom::load(argv[1]);
        const auto identity = oasis::identify_rom(rom.bytes());
        require(identity.status == oasis::RomSupportStatus::Supported, "oracle requires supported USA ROM");
        require(identity.fingerprint.sha256 ==
                    "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263",
                "USA ROM fingerprint mismatch");
        const auto scenario = read_text(argv[2]);
        require(scenario.find("scenario_id=natural_idle_to_6121a_v1") != std::string::npos,
                "scenario identity mismatch");
        require(scenario.find("start_state=hardware_reset") != std::string::npos,
                "scenario is not hardware-reset based");
        require(scenario.find("target_address=0x6121A") != std::string::npos,
                "primary target missing from scenario");
        const auto& bytes = rom.bytes();
        const std::uint8_t expected[] = {0x61, 0x00, 0x06, 0x8C};
        for (std::size_t index = 0; index < 4; ++index)
            require(bytes[0x60B8CU + index] == expected[index], "static target call-site bytes mismatch");
        const std::uint8_t second_call[] = {0x61, 0x00, 0x04, 0xCE};
        for (std::size_t index = 0; index < 4; ++index)
            require(bytes[0x60D4AU + index] == second_call[index], "second static call-site bytes mismatch");
        const std::uint8_t third_call[] = {0x61, 0x00, 0x00, 0x2A};
        for (std::size_t index = 0; index < 4; ++index)
            require(bytes[0x611EEU + index] == third_call[index], "third static call-site bytes mismatch");
        const auto report = read_text(argv[3]);
        require(report.find("oasis.m68k.natural-reach.v1") != std::string::npos,
                "natural report schema missing");
        if (search_mode) {
            require(report.find("\"scenario_family\":\"natural_reach_60b8c_60d4a_v1\"") != std::string::npos,
                    "caller-search family missing");
            require(report.find("\"variant_id\":\"start_pulse_120\"") != std::string::npos,
                    "caller-search variant mismatch");
            require(report.find("\"search_mode\":true,\"input_events\":\"120:Start\"") != std::string::npos,
                    "caller-search input metadata mismatch");
            require(report.find("\"search_target_reached\":true,\"search_target_address\":\"0x00060B8C\",\"search_target_frame\":423") != std::string::npos,
                    "caller-search target mismatch");
            require(report.find("\"frames_executed\":424") != std::string::npos,
                    "caller-search frame horizon mismatch");
            require(report.find("\"address\":\"0x00060B8C\",\"count\":3") != std::string::npos,
                    "caller-search hit count mismatch");
            require(report.find("\"address\":\"0x00060D4A\",\"count\":0") != std::string::npos,
                    "unobserved caller count mismatch");
            require(report.find("\"entry\":{\"pc\":\"0x00060B8C\"") != std::string::npos,
                    "caller-search entry snapshot missing");
            require(report.find("\"a\":[\"0x0006F8AE\",\"0x00000C00\",\"0x00FF316C\",\"0x00FF136C\",\"0x00000000\",\"0x00FF001A\",\"0x0003BDA6\",\"0x00FF0BA8\"]") != std::string::npos,
                    "caller-search register snapshot mismatch");
            require(report.find("\"stack_window\":{\"start\":\"0x00FF0B88\"") != std::string::npos,
                    "caller-search stack window missing");
            require(report.find("\"caller_pc\":\"0x00060B8C\"") != std::string::npos,
                    "caller-search downstream pairing missing");
            require(report.find("\"expected_return_address\":\"0x00060B90\",\"return_address_match\":true") != std::string::npos,
                    "caller-search return-address evidence missing");
            require(report.find("\"deterministic\":true") != std::string::npos,
                    "caller-search determinism missing");
            std::cout << "verified natural USA caller-search oracle for 0x60B8C\n";
            return 0;
        }
        require(report.find("\"target_reached\":true") != std::string::npos,
                "natural target was not reached");
        require(report.find("\"target_frame\":113") != std::string::npos,
                "natural target frame mismatch");
        require(report.find("\"frames_executed\":114") != std::string::npos,
                "natural frame horizon mismatch");
        require(report.find("\"address\":\"0x0006121A\",\"count\":2") != std::string::npos,
                "primary target hit count mismatch");
        require(report.find("\"pc\":\"0x0006121A\"") != std::string::npos,
                "entry PC snapshot missing");
        require(report.find("\"a\":[\"0x00FF06F2\",\"0x00FF0BFC") != std::string::npos,
                "entry register snapshot mismatch");
        require(report.find("\"stack_window\":{\"start\":\"0x00FF0BC2\"") != std::string::npos,
                "entry stack window missing");
        require(report.find("\"static_bytes_verified\":true") != std::string::npos,
                "caller static-byte verification missing");
        require(report.find("\"call_site\":\"0x00060B8C\",\"bytes\":\"61 00 06 8C\",\"mnemonic\":\"BSR.W\",\"displacement\":\"0x0000068C\",\"target\":\"0x0006121A\",\"instruction_size\":4,\"expected_return_address\":\"0x00060B90\"") != std::string::npos,
                "first caller static metadata mismatch");
        require(report.find("\"call_site\":\"0x00060D4A\",\"bytes\":\"61 00 04 CE\",\"mnemonic\":\"BSR.W\",\"displacement\":\"0x000004CE\",\"target\":\"0x0006121A\",\"instruction_size\":4,\"expected_return_address\":\"0x00060D4E\"") != std::string::npos,
                "second caller static metadata mismatch");
        require(report.find("\"call_site\":\"0x000611EE\",\"bytes\":\"61 00 00 2A\",\"mnemonic\":\"BSR.W\",\"displacement\":\"0x0000002A\",\"target\":\"0x0006121A\",\"instruction_size\":4,\"expected_return_address\":\"0x000611F2\"") != std::string::npos,
                "caller static metadata mismatch");
        require(report.find("\"target_sequence\":114,\"target_frame\":113,\"target_pc\":\"0x0006121A\",\"paired\":true,\"caller_pc\":\"0x000611EE\",\"caller_sequence\":113") != std::string::npos,
                "first caller-target pairing mismatch");
        require(report.find("\"target_sequence\":116,\"target_frame\":113,\"target_pc\":\"0x0006121A\",\"paired\":true,\"caller_pc\":\"0x000611EE\",\"caller_sequence\":115") != std::string::npos,
                "second caller-target pairing mismatch");
        require(report.find("\"caller_a7\":\"0x00FF0BE6\",\"target_entry_a7\":\"0x00FF0BE2\",\"a7_delta\":-4,\"stack_return_long\":\"0x000611F2\",\"expected_return_address\":\"0x000611F2\",\"return_address_match\":true,\"register_delta\":{\"a7\":") != std::string::npos,
                "first return-address evidence mismatch");
        require(report.find("\"caller_a7\":\"0x00FF0BAC\",\"target_entry_a7\":\"0x00FF0BA8\",\"a7_delta\":-4,\"stack_return_long\":\"0x000611F2\",\"expected_return_address\":\"0x000611F2\",\"return_address_match\":true,\"register_delta\":{\"a7\":") != std::string::npos,
                "second return-address evidence mismatch");
        require(report.find("\"relevant_to_existing_stack_blocker\":\"no\",\"deterministic\":true") != std::string::npos,
                "caller relevance or determinism mismatch");
        std::cout << "verified natural USA reachability oracle for 0x6121A\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
