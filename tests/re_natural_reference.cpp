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
    if (argc != 4) {
        std::cerr << "usage: oasis_re_natural_reference <USA ROM> <scenario> <natural report>\n";
        return 2;
    }
    try {
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
        std::cout << "verified natural USA reachability oracle for 0x6121A\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
