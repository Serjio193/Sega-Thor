#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_callee_effect.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <vector>

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const auto rom = oasis::Rom::load(argv[1]);
    if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported)
        throw std::runtime_error("callee-effect oracle requires the supported USA reference ROM");
    const auto report = oasis::tools::audit_callee_effect(rom.bytes());
    if (report.requested_entry != 0x60BCCU || report.call_site != 0x60BCCU || report.entry != 0x604BCU ||
        report.bounded_start != 0x604BCU || report.bounded_end != 0x604E6U || !report.boundary_proven ||
        report.boundary_status != "return_terminated_bounded_code" || report.reachable_blocks != std::vector<std::uint32_t>{0x604BCU} ||
        report.return_sites != std::vector<std::uint32_t>{0x604E4U} || !report.direct_callees.empty() ||
        !report.indirect_flow.empty() || !report.unsupported_instructions.empty())
        throw std::runtime_error("callee bounded CFG mismatch");

    const std::vector<std::string> expected_effects{
        "overwritten_unknown", "not_touched", "not_touched", "not_touched",
        "not_touched", "not_touched", "overwritten_known", "preserved"};
    if (report.register_effects.size() != expected_effects.size()) throw std::runtime_error("register effect count mismatch");
    for (std::size_t index = 0; index < expected_effects.size(); ++index)
        if (report.register_effects[index].effect != expected_effects[index]) throw std::runtime_error("register effect mismatch");
    if (report.register_effects[6].known_value != 0x00FF06F2U || report.register_effects[0].known_value)
        throw std::runtime_error("known register value mismatch");

    if (report.stack_effect.entry_a7 != "caller_pre_bsr" ||
        report.stack_effect.bsr_push != "pushes return address at caller_pre_bsr-4" ||
        report.stack_effect.internal_operations != "explicit callee delta is 0 bytes on every analyzed path" ||
        report.stack_effect.rts_pop != "RTS pops the BSR return address" ||
        report.stack_effect.net_after_return != "caller_pre_bsr" || report.stack_effect.status != "CONFIRMED")
        throw std::runtime_error("stack effect mismatch");

    if (rom.bytes()[0x60BCC] != 0x61 || rom.bytes()[0x60BCD] != 0x00 || rom.bytes()[0x60BCE] != 0xF8 ||
        rom.bytes()[0x60BCF] != 0xEE || rom.bytes()[0x60BD0] != 0x20 || rom.bytes()[0x60BD1] != 0x5F ||
        rom.bytes()[0x604BC] != 0x4D || rom.bytes()[0x604BD] != 0xF9 || rom.bytes()[0x604E4] != 0x4E ||
        rom.bytes()[0x604E5] != 0x75)
        throw std::runtime_error("callee oracle bytes mismatch");

    const auto has_memory = [&](std::uint32_t address, std::uint32_t instruction_address) {
        return std::any_of(report.memory_references.begin(), report.memory_references.end(),
                           [&](const auto& item) { return item.address == address && item.instruction_address == instruction_address; });
    };
    if (!has_memory(0x00FF0628U, 0x604BCU) || !has_memory(0x00FF06F2U, 0x604C8U) ||
        !has_memory(0x00FF0016U, 0x604DEU) || report.unresolved_memory_references.size() != 3U)
        throw std::runtime_error("callee memory evidence mismatch");

    if (report.target_rechecks.size() != 2U || report.target_rechecks[0].instruction_address != 0x60BFAU ||
        report.target_rechecks[1].instruction_address != 0x60C08U ||
        report.target_rechecks[0].status != "unresolved_stack_value" || report.target_rechecks[1].status != "unresolved_stack_value" ||
        report.target_rechecks[0].effective_address || report.target_rechecks[1].effective_address)
        throw std::runtime_error("target recheck mismatch");

    const auto json = oasis::tools::callee_effect_to_json(report);
    if (json != oasis::tools::callee_effect_to_json(report) ||
        json.find("oasis.m68k.re-callee-effect.v1") == std::string::npos)
        throw std::runtime_error("callee report determinism mismatch");
    std::cout << "verified callee=0x604BC returns=1 target_rechecks=2 unresolved=2\n";
    return 0;
}
