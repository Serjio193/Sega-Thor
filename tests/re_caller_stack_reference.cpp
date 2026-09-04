#include "core/rom.hpp"
#include "core/rom_identity.hpp"
#include "tools/re_caller_stack.hpp"

#include <algorithm>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

using namespace oasis::tools;

bool has_event(const CallerStackReport& report, std::uint32_t address, const std::string& event) {
    return std::any_of(report.stack_events.begin(), report.stack_events.end(), [&](const auto& item) {
        return item.instruction_address == address && item.event == event;
    });
}

bool has_blocker(const CallerStackReport& report, const std::string& blocker) {
    return std::find(report.blockers.begin(), report.blockers.end(), blocker) != report.blockers.end();
}

} // namespace

int main(int argc, char** argv) {
    if (argc != 2) return 2;
    const auto rom = oasis::Rom::load(argv[1]);
    if (oasis::identify_rom(rom.bytes()).status != oasis::RomSupportStatus::Supported)
        throw std::runtime_error("caller-stack oracle requires the supported USA reference ROM");

    const auto report = audit_caller_stack(rom.bytes());
    if (report.entry != 0x60004U || report.call_site != 0x60BCCU || report.callee != 0x604BCU ||
        report.window_start != 0x60004U || report.window_end != 0x61204U ||
        report.containing_blocks != std::vector<std::uint32_t>{0x60BC4U} ||
        report.predecessor_blocks != std::vector<std::uint32_t>{0x60BA4U})
        throw std::runtime_error("caller-stack CFG localization mismatch");

    if (report.relevant_path_count != 2U || report.stack_event_count != 14U ||
        report.prior_calls_crossed != 4U || report.known_call_effects != 1U ||
        report.unknown_call_effects != 3U ||
        !has_blocker(report, "unknown direct call 0x00060B8C -> 0x0006121A") ||
        !has_blocker(report, "unknown direct call 0x00060D4A -> 0x0006121A"))
        throw std::runtime_error("caller-stack unknown-call blocker mismatch");
    if (!has_event(report, 0x6042AU, "MOVE.W SR,-(A7)") ||
        !has_event(report, 0x60430U, "MOVEM.L regs,-(A7)") ||
        !has_event(report, 0x60B66U, "MOVE.L An,-(A7)") ||
        !has_event(report, 0x60B8CU, "BSR + unknown callee") ||
        !has_event(report, 0x60D4AU, "BSR + unknown callee") ||
        !has_event(report, 0x60BCCU, "BSR + proven callee + RTS"))
        throw std::runtime_error("caller-stack event evidence mismatch");

    if (report.value_at_pre_call_sp || report.value_kind != "unknown" ||
        report.reachable_unresolved_before != 16U || report.reachable_unresolved_after != 16U ||
        report.speculative_resolutions != 0U || report.target_results.size() != 2U)
        throw std::runtime_error("caller-stack unresolved-value result mismatch");
    for (const auto& target : report.target_results)
        if (target.status != "unresolved_stack_value" || target.effective_address)
            throw std::runtime_error("caller-stack target resolution mismatch");

    if (rom.bytes()[0x6042A] != 0x40 || rom.bytes()[0x6042B] != 0xE7 ||
        rom.bytes()[0x60430] != 0x48 || rom.bytes()[0x60431] != 0xE7 ||
        rom.bytes()[0x60B66] != 0x2F || rom.bytes()[0x60B67] != 0x08 ||
        rom.bytes()[0x60B8C] != 0x61 || rom.bytes()[0x60B8D] != 0x00 ||
        rom.bytes()[0x60B8E] != 0x06 || rom.bytes()[0x60B8F] != 0x8C ||
        rom.bytes()[0x60D4A] != 0x61 || rom.bytes()[0x60D4B] != 0x00 ||
        rom.bytes()[0x60D4C] != 0x04 || rom.bytes()[0x60D4D] != 0xCE ||
        rom.bytes()[0x60BCC] != 0x61 || rom.bytes()[0x60BCD] != 0x00 ||
        rom.bytes()[0x60BCE] != 0xF8 || rom.bytes()[0x60BCF] != 0xEE ||
        rom.bytes()[0x60BD0] != 0x20 || rom.bytes()[0x60BD1] != 0x5F ||
        rom.bytes()[0x60BFA] != 0x16 || rom.bytes()[0x60BFB] != 0x28 ||
        rom.bytes()[0x60C08] != 0x14 || rom.bytes()[0x60C09] != 0x28)
        throw std::runtime_error("caller-stack oracle bytes mismatch");

    const auto json = caller_stack_to_json(report);
    if (json != caller_stack_to_json(report) ||
        json.find("oasis.m68k.re-caller-stack.v1") == std::string::npos ||
        json.find("unknown direct call 0x00060B8C -> 0x0006121A") == std::string::npos)
        throw std::runtime_error("caller-stack report determinism mismatch");

    std::cout << "verified caller_stack call_site=0x60BCC paths=" << report.relevant_path_count
              << " events=" << report.stack_event_count << " unknown_calls=" << report.unknown_call_effects
              << " targets_unresolved=2\n";
    return 0;
}
