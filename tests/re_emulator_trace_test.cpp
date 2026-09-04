#include "tools/re_emulator_trace.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace {

using namespace oasis::tools;

ExternalTraceEvent event(std::uint64_t sequence, std::uint32_t pc, EmulatorEventKind kind) {
    return {.sequence = sequence, .pc = pc, .kind = kind};
}

ExternalTraceCapture capture() {
    ExternalTraceCapture result{.emulator = "synthetic", .backend = "synthetic-backend", .version = "1", .scenario = "boot_initial", .stop_condition = "event_limit:16", .event_limit = 16};
    auto first = event(0, 0x100, EmulatorEventKind::instruction);
    first.block_start = 0x100; first.instruction_size = 4;
    auto call = event(1, 0x100, EmulatorEventKind::call);
    call.block_start = 0x100; call.target = 0x200; call.instruction_size = 4;
    auto branch = event(2, 0x104, EmulatorEventKind::branch);
    branch.block_start = 0x100; branch.target = 0x108; branch.taken = false; branch.instruction_size = 2;
    auto indirect = event(3, 0x108, EmulatorEventKind::indirect_control_flow);
    indirect.block_start = 0x108; indirect.target = 0x10C;
    auto read = event(4, 0x200, EmulatorEventKind::memory_read);
    read.address = 0x00FF0000; read.width_bytes = 2;
    auto write = event(5, 0x204, EmulatorEventKind::memory_write);
    write.address = 0x00FF0002; write.width_bytes = 4;
    auto ret = event(6, 0x208, EmulatorEventKind::return_instruction);
    result.events = {first, call, branch, indirect, read, write, ret};
    return result;
}

void test_parser_and_normalization() {
    const auto parsed = parse_external_trace(
        "oasis.m68k.external-trace.v1\n"
        "emulator=synthetic\nbackend=synthetic-backend\nversion=1\nscenario=boot_initial\nstop_condition=event_limit:16\nlimit=16\n"
        "event seq=1 pc=0x100 kind=call block=0x100 target=0x200 size=4\n"
        "event seq=0 pc=0x100 kind=instruction block=0x100 size=4 frame=2 cycles=99\n");
    assert(parsed.events.size() == 2U && parsed.emulator == "synthetic" && parsed.event_limit == 16U);
    const std::array<std::uint32_t, 3> atlas{0x100, 0x104, 0x200};
    const auto report = normalize_emulator_trace(parsed, atlas, ResetVectorEvidence{0x00FF0000, 0x100}, "synthetic-rom", "sha");
    assert(report.events.front().sequence == 0U && report.first_observed_pc == 0x100U);
    assert(report.first_pc_matches_reset && *report.first_pc_matches_reset);
    assert(report.unique_pcs.size() == 1U && report.atlas_known_pcs.size() == 1U);
    assert(report.call_count == 1U && report.direct_call_edges.size() == 1U);
    assert(report.nondeterministic_fields.size() == 2U && report.trace_hash != "");
    assert(report.metadata.backend == "synthetic-backend" && report.metadata.stop_condition == "event_limit:16");
    assert(emulator_trace_to_json(report).find("\"backend\":\"synthetic-backend\"") != std::string::npos);
}

void test_coverage_edges_and_atlas_split() {
    const std::array<std::uint32_t, 3> atlas{0x100, 0x104, 0x200};
    const auto report = normalize_emulator_trace(capture(), atlas);
    assert(report.events.size() == 7U && report.unique_pcs.size() == 6U);
    assert(report.executed_basic_blocks.size() == 2U && report.executed_ranges.size() == 6U);
    assert(report.branch_count == 1U && report.call_count == 1U && report.return_count == 1U);
    assert(report.memory_read_count == 1U && report.memory_write_count == 1U);
    assert(report.indirect_targets == std::vector<std::uint32_t>{0x10C});
    const std::vector<std::uint32_t> expected_known{0x100, 0x104, 0x200};
    const std::vector<std::uint32_t> expected_unknown{0x108, 0x204, 0x208};
    assert(report.atlas_known_pcs == expected_known);
    assert(report.atlas_unknown_pcs == expected_unknown);
    assert(report.direct_call_edges[0].caller == 0x100U && report.direct_call_edges[0].callee == 0x200U);
    const std::vector<std::uint32_t> expected_known_targets{0x200};
    const std::vector<std::uint32_t> expected_unknown_targets{0x108, 0x10C};
    assert(report.atlas_known_control_flow_targets == expected_known_targets);
    assert(report.atlas_unknown_control_flow_targets == expected_unknown_targets);
}

void test_order_and_register_determinism() {
    const std::array<std::uint32_t, 1> atlas{0x100};
    auto left = capture();
    auto right = left;
    std::reverse(right.events.begin(), right.events.end());
    auto registers = EmulatorRegisterSnapshot{};
    registers.d[0] = 0x1234; registers.a[7] = 0x00FF0000; registers.sr = 0x2700;
    left.events[0].registers = registers;
    right.events.back().registers = registers;
    const auto first = normalize_emulator_trace(left, atlas);
    const auto second = normalize_emulator_trace(right, atlas);
    assert(first.trace_hash == second.trace_hash);
    assert(emulator_trace_to_json(first) == emulator_trace_to_json(first));
    assert(emulator_trace_to_json(first).find("registers") != std::string::npos);
    left.events[0].registers->d[0] = 0x1235;
    assert(normalize_emulator_trace(left, atlas).trace_hash != first.trace_hash);
}

void test_malformed_input_rejected() {
    bool failed = false;
    try { (void)parse_external_trace("event seq=0 pc=0x100 kind=instruction\n"); }
    catch (const std::invalid_argument&) { failed = true; }
    assert(failed);
    failed = false;
    try { (void)parse_external_trace("oasis.m68k.external-trace.v1\nevent seq=0 pc=0x100 kind=branch\n"); }
    catch (const std::invalid_argument&) { failed = true; }
    assert(failed);
    failed = false;
    auto duplicate = capture(); duplicate.events[1].sequence = 0;
    try { (void)normalize_emulator_trace(std::move(duplicate), {}); }
    catch (const std::invalid_argument&) { failed = true; }
    assert(failed);
    const auto crlf = parse_external_trace("oasis.m68k.external-trace.v1\r\n"
        "emulator=synthetic\r\nbackend=synthetic-backend\r\nversion=1\r\nscenario=boot_initial\r\nstop_condition=event_limit:1\r\n"
        "event seq=0 pc=0x100 kind=instruction\r\n");
    assert(crlf.events.size() == 1U && crlf.backend == "synthetic-backend" && crlf.stop_condition == "event_limit:1");
}

void test_reset_vector_reader() {
    const std::array<std::uint8_t, 8> rom{0x00, 0xFF, 0x00, 0x00, 0x00, 0x00, 0x01, 0x00};
    const auto reset = read_reset_vectors(rom);
    assert(reset.initial_sp == 0x00FF0000U && reset.initial_pc == 0x100U);
}

} // namespace

int main() {
    test_parser_and_normalization();
    test_coverage_edges_and_atlas_split();
    test_order_and_register_determinism();
    test_malformed_input_rejected();
    test_reset_vector_reader();
    return 0;
}
