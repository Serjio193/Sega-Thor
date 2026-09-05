#include "tools/re_ant_queue.hpp"

#include <cassert>
#include <cstdio>
#include <stdexcept>
#include <string>

using namespace oasis::tools;

namespace {

std::string frontier(std::uint32_t pc) {
    char value[9]{};
    std::snprintf(value, sizeof(value), "%08X", pc);
    return "{\"id\":\"f" + std::string(value) + "\",\"source_entry\":\"0x00000100\",\"source_pc\":\"0x" +
        value + "\",\"blocker_type\":\"INDIRECT_FLOW\",\"instruction_bytes\":[78,145],\"opcode\":\"0x00004E91\",\"instruction\":\"jsr\",\"known_context\":\"address_indirect\",\"stop_reason\":\"INDIRECT_TRANSFER\",\"reason\":\"computed target is unresolved\"}";
}

std::string explorer() {
    return "{\"frontier\":[" + frontier(0x100) + "," + frontier(0x200) + "," + frontier(0x300) + "," + frontier(0x400) + "," + frontier(0x500) + "],\"address_map\":[]}";
}

AntResult result_for(const AntJob& job) {
    AntObservation observation{.actual_pc = job.source_pc, .instruction = job.instruction,
                               .instruction_bytes = job.instruction_bytes, .next_pc = 0x600,
                               .indirect_target = 0x600, .frame = 5, .sequence = 9,
                               .target_inside_rom = true};
    return {.job_id = job.job_id, .frontier_id = job.frontier_id, .status = AntStatus::resolved,
            .backend = job.backend, .backend_version = job.backend_version, .rom_sha256 = job.rom_sha256,
            .scenario_id = job.scenario_id, .input_events = job.input_events,
            .input_policy = job.allowed_input_policy, .checkpoint_reference = job.checkpoint_reference,
            .source_entry = job.source_entry, .source_pc = job.source_pc, .observed = observation,
            .reproducible = true, .result_hash = "hash"};
}

void test_deterministic_selection_and_serialization() {
    const auto reach = "{\"frames\":300,\"input_events\":\"\",\"hits\":{\"0x00000100\":{\"count\":1,\"frame\":5},\"0x00000300\":{\"count\":1,\"frame\":20}}}";
    const auto first = make_ant_queue(explorer(), reach, "sha", 0x1000, "2.11.1");
    const auto second = make_ant_queue(explorer(), reach, "sha", 0x1000, "2.11.1");
    assert(first.jobs.size() == 5 && first.queue_state == "FROZEN");
    assert(first.jobs[0].job.source_pc == 0x100 && first.jobs[1].job.source_pc == 0x300);
    assert(first.jobs[2].job.source_pc == 0x200 && ant_queue_equal(first, second));
    assert(ant_queue_equal(first, parse_ant_queue(ant_queue_to_json(first))));
}

void test_lifecycle_duplicate_and_recovery() {
    AntQueue queue; queue.queue_state = "BUILDING";
    FrontierRecord source{.id = "frontier", .source_entry = 0x100, .source_pc = 0x100,
        .blocker_type = FrontierType::indirect_flow, .instruction_bytes = {78, 145},
        .opcode = 0x4E91, .instruction = "jsr", .known_context = "address_indirect",
        .stop_reason = StopReason::indirect_transfer, .reason = "unresolved"};
    const auto job = make_ant_job(source, "sha", 0x1000, "2.11.1");
    assert(add_ant_queue_job(queue, job));
    assert(!add_ant_queue_job(queue, job) && queue.duplicate_jobs_avoided == 0);
    queue.queue_state = "FROZEN";
    const auto claimed = claim_next_ant_job(queue);
    assert(claimed && queue.jobs[*claimed].lifecycle == AntQueueLifecycle::claimed);
    bool second_claim_rejected = false;
    try { (void)claim_next_ant_job(queue); } catch (const std::logic_error&) { second_claim_rejected = true; }
    assert(second_claim_rejected);
    finalize_ant_job(queue, *claimed, result_for(job), true, "accepted_dynamic_evidence");
    assert(queue.jobs[*claimed].lifecycle == AntQueueLifecycle::resolved);
    queue.queue_state = "BUILDING";
    assert(!add_ant_queue_job(queue, job) && queue.duplicate_jobs_avoided == 1);
    queue.queue_state = "FROZEN";
    auto stale = queue;
    stale.jobs.push_back({job});
    const auto stale_index = claim_next_ant_job(stale);
    assert(stale_index);
    recover_stale_ant_claims(stale);
    assert(stale.jobs[*stale_index].lifecycle == AntQueueLifecycle::failed_retryable);
}

} // namespace

int main() {
    test_deterministic_selection_and_serialization();
    test_lifecycle_duplicate_and_recovery();
}
