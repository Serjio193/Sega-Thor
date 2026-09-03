#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_trace.hpp"

#include <cstdint>
#include <iostream>
#include <stdexcept>

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: oasis_re_trace_reference <Beyond Oasis USA ROM>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("reference test requires the supported USA ROM");
        }
        const auto report = oasis::tools::trace_m68k_scenario(rom.bytes());
        const auto& trace = report;
        const std::uint32_t expected_pc[] = {0xA7D4U, 0xA7DAU, 0xA7DEU, 0xA7E2U, 0xA7E4U};
        if (trace.executed_instructions.size() != 5U || trace.stop_reason != "return") {
            throw std::runtime_error("bounded trace did not complete the controlled scenario");
        }
        for (std::size_t i = 0; i < 5U; ++i) {
            if (trace.executed_instructions[i].address != expected_pc[i]) {
                throw std::runtime_error("executed PC trace mismatch");
            }
        }
        if (trace.branches.size() != 1U || trace.branches.front().taken ||
            trace.branches.front().instruction_address != 0xA7DAU) {
            throw std::runtime_error("branch outcome mismatch");
        }
        if (trace.memory_reads.size() != 2U || trace.memory_reads[0].address != 0x00FF2954U ||
            trace.memory_reads[1].address != 0x00FF2976U || trace.memory_reads[1].value != 0xA7E4U) {
            throw std::runtime_error("controlled RAM evidence mismatch");
        }
        if (trace.indirect_targets.size() != 1U || trace.indirect_targets.front().target != 0xA7E4U) {
            throw std::runtime_error("indirect target was not resolved");
        }
        bool saw_jump_resolution = false;
        bool saw_memory_resolution = false;
        for (const auto& item : trace.newly_resolved) {
            saw_jump_resolution = saw_jump_resolution || item.static_kind == "unresolved_control_flow" &&
                                  item.instruction_address == 0xA7E2U && item.observed_value == 0xA7E4U;
            saw_memory_resolution = saw_memory_resolution || item.static_kind == "unresolved_memory_reference" &&
                                    item.instruction_address == 0xA7DEU && item.observed_value == 0x00FF2976U;
        }
        if (!saw_jump_resolution || !saw_memory_resolution || trace.still_unresolved.size() != 9U) {
            throw std::runtime_error("static/dynamic evidence comparison mismatch");
        }
        if (oasis::tools::trace_to_json(trace).find("oasis.m68k.re-trace.v1") == std::string::npos) {
            throw std::runtime_error("trace JSON schema missing");
        }
        std::cout << "verified bounded A7D4/A7E2 USA dynamic trace oracle\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
