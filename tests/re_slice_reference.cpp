#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_slice_decoder.hpp"

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <stdexcept>

namespace {

void expect_bytes(const oasis::Rom& rom, std::size_t offset,
                  std::initializer_list<std::uint8_t> expected,
                  const char* message) {
    if (offset + expected.size() > rom.size()) throw std::runtime_error(message);
    std::size_t index = 0;
    for (const auto value : expected) {
        if (rom.bytes()[offset + index++] != value) throw std::runtime_error(message);
    }
}

bool has_edge(const oasis::tools::DecodedSlice& slice, std::uint32_t source,
              std::uint32_t target, oasis::tools::FlowKind kind) {
    for (const auto& edge : slice.control_flow) {
        if (edge.source == source && edge.target == target && edge.kind == kind) return true;
    }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: oasis_re_slice_reference <Beyond Oasis USA ROM>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("reference test requires the supported USA ROM");
        }
        expect_bytes(rom, 0x60004, {0x60, 0x00, 0x04, 0x24}, "entry branch mismatch");
        expect_bytes(rom, 0x6042A, {0x40, 0xE7, 0x00, 0x7C}, "dispatcher entry mismatch");
        expect_bytes(rom, 0x6042C, {0x00, 0x7C, 0x07, 0x00}, "dispatcher status opcode mismatch");
        expect_bytes(rom, 0x609C6, {0x70, 0x00, 0x08, 0x2D}, "command six handler mismatch");
        expect_bytes(rom, 0x60D10, {0x33, 0xFC, 0x01, 0x00}, "command eight handler mismatch");
        expect_bytes(rom, 0x611D8, {0x4C, 0xDF, 0x7F, 0xFE}, "driver epilogue mismatch");

        const auto slice = oasis::tools::decode_m68k_slice(rom.bytes());
        if (slice.entry != oasis::tools::kReAccelerationEntry || slice.instructions.empty() ||
            slice.basic_blocks.empty()) {
            throw std::runtime_error("bounded slice is empty");
        }
        if (!has_edge(slice, 0x60004, 0x6042A, oasis::tools::FlowKind::direct_branch) ||
            !has_edge(slice, 0x60478, 0x609C6, oasis::tools::FlowKind::direct_branch) ||
            !has_edge(slice, 0x60488, 0x60D10, oasis::tools::FlowKind::direct_branch)) {
            throw std::runtime_error("expected direct control-flow edge is missing");
        }
        bool saw_ram = false;
        bool saw_other = false;
        for (const auto& instruction : slice.instructions) {
            for (const auto& reference : instruction.memory_references) {
                saw_ram = saw_ram || reference.kind == oasis::tools::MemoryKind::ram;
                saw_other = saw_other || reference.address == 0x00A11100U;
            }
        }
        if (!saw_ram || !saw_other) {
            throw std::runtime_error("reference slice did not expose required evidence categories");
        }
        const auto json = oasis::tools::slice_to_json(slice);
        if (json.find("oasis.m68k.re-slice.v1") == std::string::npos ||
            json.find("0x00060004") == std::string::npos ||
            json.find("0x000609c6") == std::string::npos ||
            json.find("unresolved_control_flow") == std::string::npos ||
            json.find("unsupported_instruction_addresses") == std::string::npos) {
            throw std::runtime_error("machine-readable report missing target evidence");
        }
        std::cout << "verified bounded 0x60004 m68k slice and evidence report\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
