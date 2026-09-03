#include "tools/re_trace.hpp"

#include <cassert>
#include <cstdint>
#include <string>
#include <vector>

namespace {

void put16(std::vector<std::uint8_t>& bytes, std::uint32_t address, std::uint16_t value) {
    bytes[address] = static_cast<std::uint8_t>(value >> 8U);
    bytes[address + 1U] = static_cast<std::uint8_t>(value);
}

void put32(std::vector<std::uint8_t>& bytes, std::uint32_t address, std::uint32_t value) {
    put16(bytes, address, static_cast<std::uint16_t>(value >> 16U));
    put16(bytes, address + 2U, static_cast<std::uint16_t>(value));
}

std::vector<std::uint8_t> synthetic_rom() {
    std::vector<std::uint8_t> bytes(0xA800, 0x4E);
    for (std::uint32_t address = 0; address + 1U < bytes.size(); address += 2U) put16(bytes, address, 0x4E71U);
    put16(bytes, 0xA6A4, 0x6000U);
    put16(bytes, 0xA6A6, 0x012EU); // bounded static entry jumps to A7D4
    put16(bytes, 0xA7D4, 0x0C6EU);
    put16(bytes, 0xA7D6, 0xFFFEU);
    put16(bytes, 0xA7D8, 0x0000U);
    put16(bytes, 0xA7DA, 0x6700U);
    put16(bytes, 0xA7DC, 0x0008U);
    put16(bytes, 0xA7DE, 0x206EU);
    put16(bytes, 0xA7E0, 0x0022U);
    put16(bytes, 0xA7E2, 0x4ED0U);
    put16(bytes, 0xA7E4, 0x4E75U);
    return bytes;
}

} // namespace

int main() {
    using namespace oasis::tools;
    const auto rom = synthetic_rom();
    const TraceOptions options{.function_entry = 0xA6A4,
                               .static_byte_budget = 0x180,
                               .start_pc = 0xA7D4,
                               .instruction_budget = 8,
                               .initial_a6 = 0x00FF1000,
                               .initial_record_word = 1,
                               .callback_target = 0xA7E4};
    const auto first = trace_m68k_scenario(rom, options);
    const auto second = trace_m68k_scenario(rom, options);
    assert(first.executed_instructions.size() == 5U);
    assert(first.executed_basic_blocks.size() == 3U);
    assert(first.branches.size() == 1U);
    assert(!first.branches.front().taken);
    assert(first.calls.empty());
    assert(first.returns.size() == 1U);
    assert(first.memory_reads.size() == 2U);
    assert(first.memory_writes.empty());
    assert(first.memory_reads[0].address == 0x00FF1000U);
    assert(first.memory_reads[1].address == 0x00FF1022U);
    assert(first.indirect_targets.size() == 1U);
    assert(first.indirect_targets.front().target == 0xA7E4U);
    assert(first.newly_resolved.size() == 3U);
    std::size_t static_unresolved = first.static_slice.unresolved_control_flow.size();
    for (const auto& instruction : first.static_slice.instructions) {
        static_unresolved += instruction.unresolved_memory_references.size();
    }
    assert(first.still_unresolved.size() + first.newly_resolved.size() == static_unresolved);
    assert(first.still_unresolved.size() < static_unresolved);
    assert(first.stop_reason == "return");
    assert(trace_to_json(first) == trace_to_json(second));
    assert(trace_to_json(first).find("oasis.m68k.re-trace.v1") != std::string::npos);
    assert(trace_to_json(first).find("memory_writes") != std::string::npos);
    assert(trace_to_text(first).find("Newly resolved:") != std::string::npos);
    return 0;
}
