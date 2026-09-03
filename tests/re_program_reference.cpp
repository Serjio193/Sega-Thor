#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_program.hpp"

#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace {

void expect_bytes(const oasis::Rom& rom, std::size_t offset,
                  std::initializer_list<std::uint8_t> expected, const char* message) {
    if (offset + expected.size() > rom.size()) throw std::runtime_error(message);
    std::size_t index = 0;
    for (const auto value : expected) {
        if (rom.bytes()[offset + index++] != value) throw std::runtime_error(message);
    }
}

bool has_edge(const oasis::tools::MultiSliceReport& report, std::uint32_t caller,
              std::uint32_t callee) {
    for (const auto& edge : report.function_call_edges) {
        if (edge.caller_entry == caller && edge.callee_entry == callee) return true;
    }
    return false;
}

bool has_slice_edge(const oasis::tools::MultiSliceReport& report, std::uint32_t function,
                    std::uint32_t source, std::uint32_t target,
                    oasis::tools::FlowKind kind) {
    for (const auto& item : report.functions) {
        if (item.entry != function) continue;
        for (const auto& edge : item.slice.control_flow) {
            if (edge.source == source && edge.target == target && edge.kind == kind) return true;
        }
    }
    return false;
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        std::cerr << "usage: oasis_re_program_reference <Beyond Oasis USA ROM>\n";
        return 2;
    }
    try {
        const auto rom = oasis::Rom::load(argv[1]);
        if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported) {
            throw std::runtime_error("reference test requires the supported USA ROM");
        }
        expect_bytes(rom, 0x3820, {0x48, 0xE7, 0xE0, 0x20}, "decompressor entry mismatch");
        expect_bytes(rom, 0x8E90, {0x4D, 0xF9, 0x00, 0xFF}, "pool loop entry mismatch");
        expect_bytes(rom, 0xA6A4, {0x4D, 0xF9, 0x00, 0xFF}, "callback entry mismatch");
        expect_bytes(rom, 0xD3B2, {0x48, 0xE7, 0x80, 0xC0}, "resource reader entry mismatch");

        const std::vector<oasis::tools::FunctionTarget> targets{
            {.entry = 0x3820, .byte_budget = 0, .confirmed_end = 0x3B3E},
            {.entry = 0x8E90, .byte_budget = 0x120, .confirmed_end = std::nullopt},
            {.entry = 0xA6A4, .byte_budget = 0x180, .confirmed_end = std::nullopt},
            {.entry = 0xD3B2, .byte_budget = 0, .confirmed_end = 0xD406},
        };
        const auto report = oasis::tools::analyze_m68k_functions(rom.bytes(), targets);
        if (report.functions.size() != 4U || report.functions[0].entry != 0x3820U ||
            report.functions[1].entry != 0x8E90U || report.functions[2].entry != 0xA6A4U ||
            report.functions[3].entry != 0xD3B2U) {
            throw std::runtime_error("representative function set mismatch");
        }
        if (report.functions[0].boundary != oasis::tools::BoundaryStatus::confirmed ||
            report.functions[3].boundary != oasis::tools::BoundaryStatus::confirmed) {
            throw std::runtime_error("known function boundary was not preserved");
        }
        if (!has_edge(report, 0xD3B2, 0x3820)) {
            throw std::runtime_error("resource reader direct call was not reproduced");
        }
        if (!has_slice_edge(report, 0x8E90, 0x8EA6, 0x8F12, oasis::tools::FlowKind::direct_branch) ||
            !has_slice_edge(report, 0x8E90, 0x8EC8, 0x8F22, oasis::tools::FlowKind::direct_branch)) {
            throw std::runtime_error("pool loop dispatch edges were not reproduced");
        }
        bool saw_callback_jump = false;
        for (const auto& item : report.unresolved_control_flow) {
            saw_callback_jump = saw_callback_jump || item.function_entry == 0xA6A4U &&
                                item.flow.address == 0xA7E2U &&
                                item.flow.kind == oasis::tools::FlowKind::indirect_jump;
        }
        if (!saw_callback_jump) throw std::runtime_error("indirect callback edge was not preserved");
        bool saw_ram = false;
        bool saw_table = false;
        bool saw_destination = false;
        for (const auto& item : report.confirmed_memory_references) {
            saw_ram = saw_ram || item.reference.kind == oasis::tools::MemoryKind::ram;
            saw_table = saw_table || item.reference.address == 0x0005CE96U;
            saw_destination = saw_destination || item.reference.address == 0x00FF2FA8U;
            if (item.block_start == 0U || item.instruction_address == 0U) {
                throw std::runtime_error("memory reference lost function/block binding");
            }
        }
        if (!saw_ram || !saw_table || !saw_destination || report.unresolved_control_flow.empty()) {
            throw std::runtime_error("representative evidence categories are incomplete");
        }
        const auto json = oasis::tools::program_to_json(report);
        const auto text = oasis::tools::program_to_text(report);
        if (json.find("oasis.m68k.re-program.v1") == std::string::npos ||
            json.find("0x0000d3b2") == std::string::npos ||
            json.find("0x00003820") == std::string::npos ||
            text.find("caller_to_callee=1") == std::string::npos) {
            throw std::runtime_error("multi-slice evidence report is incomplete");
        }
        std::cout << "verified representative bounded multi-slice report and USA evidence\n";
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "error: " << error.what() << '\n';
        return 1;
    }
}
