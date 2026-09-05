#include "tools/re_ant.hpp"

#include <cassert>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>

using namespace oasis::tools;

namespace {

FrontierRecord frontier() {
    return {.id = "rom:0x00000100:0x00000100:INDIRECT_FLOW:address_indirect:computed target is unresolved",
            .source_entry = 0x100, .source_pc = 0x100, .blocker_type = FrontierType::indirect_flow,
            .instruction_bytes = {0x4E, 0x91}, .opcode = 0x4E91, .instruction = "jsr",
            .known_context = "address_indirect", .stop_reason = StopReason::indirect_transfer,
            .reason = "computed target is unresolved"};
}

AntResult resolved(const AntJob& job, std::uint32_t target = 0x200) {
    EmulatorRegisterSnapshot registers{};
    registers.a[1] = 0x200;
    AntObservation observation{.actual_pc = job.source_pc, .instruction = job.instruction,
                               .instruction_bytes = job.instruction_bytes, .registers = registers,
                               .next_pc = target, .indirect_target = target, .frame = 7,
                               .sequence = 42, .target_inside_rom = true};
    return {.job_id = job.job_id, .frontier_id = job.frontier_id, .status = AntStatus::resolved,
            .backend = "bizhawk", .backend_version = "2.11.1", .rom_sha256 = job.rom_sha256,
            .reachability_class = AntReachabilityClass::natural_observed, .scenario_id = job.scenario_id,
            .input_events = job.input_events, .input_policy = job.allowed_input_policy,
            .checkpoint_reference = job.checkpoint_reference, .worker_run_id = "test-A",
            .source_entry = job.source_entry,
            .source_pc = job.source_pc, .observed = observation, .stop_reason = "next_pc_observed",
            .reproducible = true, .result_hash = "0xA", .frames_executed = 8,
            .instructions_until_observation = 100};
}

void test_job_and_merge() {
    const auto first = make_ant_job(frontier(), "sha", 0x1000, "2.11.1");
    const auto second = make_ant_job(frontier(), "sha", 0x1000, "2.11.1");
    assert(first.job_id == second.job_id && ant_job_to_json(first) == ant_job_to_json(second));
    const auto result = resolved(first);
    const auto merge = merge_ant_result(first, result, 0x1000, "sha");
    assert(merge.accepted && merge.edge.target == 0x200 && merge.edge.evidence_class == "DYNAMIC_NATURAL");
    auto wrong = result; wrong.frontier_id = "wrong";
    assert(!merge_ant_result(first, wrong, 0x1000, "sha").accepted);
    assert(!merge_ant_result(first, result, 0x1000, "other").accepted);
    wrong = result; wrong.reachability_class = AntReachabilityClass::forced;
    assert(!merge_ant_result(first, wrong, 0x1000, "sha").accepted);
    wrong = result; wrong.result_hash = "0xB";
    assert(!ant_results_equal(result, wrong));
    assert(ant_results_equal(result, result));
}

void test_frontier_selection_and_serialization() {
    const auto source = frontier();
    const std::string json = "{\"frontier\":[{\"id\":\"" + source.id +
        "\",\"source_entry\":\"0x00000100\",\"source_pc\":\"0x00000100\",\"blocker_type\":\"INDIRECT_FLOW\",\"instruction_bytes\":[78,145],\"opcode\":\"0x00004E91\",\"instruction\":\"jsr\",\"known_context\":\"address_indirect\",\"stop_reason\":\"INDIRECT_TRANSFER\",\"reason\":\"computed target is unresolved\"}]}";
    const auto selected = select_ant_frontier(json, 0x100);
    assert(selected && selected->id == source.id && selected->instruction_bytes == source.instruction_bytes);
    const auto job = make_ant_job(source, "sha", 0x1000, "2.11.1");
    const auto result = resolved(job);
    assert(ant_result_to_json(result) == ant_result_to_json(result));
    assert(ant_result_to_text(result).find("DYNAMIC_NATURAL") != std::string::npos);
}

void put16(std::vector<std::uint8_t>& rom, std::size_t address, std::uint16_t value) {
    rom[address] = static_cast<std::uint8_t>(value >> 8U); rom[address + 1U] = static_cast<std::uint8_t>(value);
}

void test_explorer_feedback() {
    std::vector<std::uint8_t> rom(0x400, 0);
    put16(rom, 0x100, 0x4E91); put16(rom, 0x102, 0x4E75); put16(rom, 0x200, 0x4E75);
    CandidateMapReport candidates;
    AtlasReport atlas;
    ExploreOptions options; options.control_entries = {0x100};
    const auto before = explore_m68k(rom, candidates, atlas, options);
    assert(before.metrics.unresolved_indirect == 1U && before.metrics.frontier_count == 1U);
    options.dynamic_edges.push_back({before.frontier.front().id, 0x100, 0x100, 0x200, "DYNAMIC_NATURAL", "job", "result", "bizhawk", "natural_reset_idle_v1"});
    const auto after = explore_m68k(rom, candidates, atlas, options);
    assert(after.metrics.unresolved_indirect == 0U && after.metrics.dynamic_indirect_edges == 1U);
    assert(after.frontier.empty() && after.metrics.entries_processed == 2U);
    assert(after.edges.front().evidence_class == "DYNAMIC_NATURAL");
    assert(after.edges.front().frontier_id == before.frontier.front().id &&
           after.edges.front().job_id == "job" && after.edges.front().result_hash == "result");
}

} // namespace

int main() {
    test_job_and_merge();
    test_frontier_selection_and_serialization();
    test_explorer_feedback();
}
