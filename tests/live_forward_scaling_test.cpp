#include "live_forward_trace.h"

#include <cassert>
#include <cstdint>

namespace
{
uint32_t current_pc = 0x10000u;

oasis_lf_cpu_state state_at(uint32_t pc)
{
  oasis_lf_cpu_state state{};
  state.pc = pc;
  state.usp = 0x10000u;
  state.isp = 0x20000u;
  return state;
}

void execute(uint16_t opcode, uint32_t next_pc)
{
  oasis_lf_cpu_state before = state_at(current_pc);
  oasis_lf_cpu_state after = before;
  oasis_lf_instruction_begin(before.pc, &before);
  oasis_lf_instruction_set_opcode(opcode, &before);
  after.pc = next_pc;
  oasis_lf_instruction_end(next_pc, &after, 0);
  current_pc = next_pc;
}

void check_100_cycles(uint32_t count, uint32_t depth, uint64_t run_id)
{
  assert(oasis_lf_configure(count, depth, 64u * 1024u));
  const uint64_t epoch = oasis_lf_epoch();
  uint64_t previous_entry[64]{};
  for (uint32_t cycle = 1; cycle <= 100; ++cycle)
  {
    for (uint32_t worker = 0; worker < count; ++worker)
    {
      const uint64_t capture = run_id * 1000000u +
        (uint64_t)(cycle - 1u) * count + worker + 1u;
      uint64_t status[OASIS_LF_WORKER_STATUS_COUNT]{};
      assert(oasis_lf_result_state(worker) == 0u);
      assert(oasis_lf_request(worker, capture, cycle, run_id, epoch));
      assert(oasis_lf_worker_status_get(worker, status,
        OASIS_LF_WORKER_STATUS_COUNT) ==
        static_cast<int>(OASIS_LF_WORKER_STATUS_COUNT));
      assert(status[0] == 1u && status[1] == 0u);
    }
    for (uint32_t worker = 0; worker < count; ++worker)
      execute(0x4e71u, current_pc + 2u);
    for (uint32_t worker = 0; worker < count; ++worker)
    {
      uint64_t status[OASIS_LF_WORKER_STATUS_COUNT]{};
      assert(oasis_lf_result_state(worker) == 2u);
      assert(oasis_lf_worker_status_get(worker, status,
        OASIS_LF_WORKER_STATUS_COUNT) ==
        static_cast<int>(OASIS_LF_WORKER_STATUS_COUNT));
      assert(status[0] == 2u && status[1] == 0u && status[2] == depth);
    }

    for (uint32_t flow = 0; flow < depth; ++flow)
      execute(0x6602u, current_pc + 8u);
    for (uint32_t worker = 0; worker < count; ++worker)
    {
      uint64_t status[OASIS_LF_WORKER_STATUS_COUNT]{};
      assert(oasis_lf_result_state(worker) == 3u);
      assert(oasis_lf_worker_status_get(worker, status,
        OASIS_LF_WORKER_STATUS_COUNT) ==
        static_cast<int>(OASIS_LF_WORKER_STATUS_COUNT));
      assert(status[0] == 3u && status[1] == depth && status[2] == depth);
    }

    uint64_t prior_round_entry = 0;
    for (uint32_t worker = 0; worker < count; ++worker)
    {
      oasis_lf_result result{};
      assert(oasis_lf_result_info(worker, &result));
      assert(oasis_lf_result_state(worker) == 4u);
      assert(result.valid == 1u);
      assert(result.worker_id == worker);
      assert(result.capture_id == run_id * 1000000u +
        (uint64_t)(cycle - 1u) * count + worker + 1u);
      assert(result.generation == cycle);
      assert(result.run_id == run_id && result.epoch == epoch);
      assert(result.consumed_depth == depth);
      assert(result.termination_reason == OASIS_LF_END_DEPTH_LIMIT);
      uint64_t status[OASIS_LF_WORKER_STATUS_COUNT]{};
      assert(oasis_lf_worker_status_get(worker, status,
        OASIS_LF_WORKER_STATUS_COUNT) ==
        static_cast<int>(OASIS_LF_WORKER_STATUS_COUNT));
      assert(status[0] == 4u && status[1] == depth && status[2] == depth);
      assert(status[3] == cycle && status[4] == cycle && status[5] == cycle);
      assert(result.entry_stream_sequence > prior_round_entry);
      assert(result.entry_stream_sequence > previous_entry[worker]);
      prior_round_entry = result.entry_stream_sequence;
      previous_entry[worker] = result.entry_stream_sequence;
      assert(oasis_lf_mark_audited(worker, cycle, 1u));
      assert(oasis_lf_ack(worker, result.capture_id, cycle, run_id, epoch));
      assert(oasis_lf_result_state(worker) == 0u);
      assert(oasis_lf_worker_status_get(worker, status,
        OASIS_LF_WORKER_STATUS_COUNT) ==
        static_cast<int>(OASIS_LF_WORKER_STATUS_COUNT));
      assert(status[0] == 0u && status[1] == 0u && status[6] == cycle);
    }
  }

  uint64_t values[OASIS_LF_METRICS_COUNT]{};
  assert(oasis_lf_metrics_get(values, OASIS_LF_METRICS_COUNT) ==
         static_cast<int>(OASIS_LF_METRICS_COUNT));
  const uint64_t total = (uint64_t)count * 100u;
  assert(values[1] == count);
  assert(values[5] == 0u);
  assert(values[6] == count);
  assert(values[9] == total && values[10] == total && values[11] == total);
  assert(values[12] == 0u && values[13] == 0u && values[14] == 0u);
  assert(values[23] == total && values[24] == 0u);
  for (uint32_t worker = 0; worker < count; ++worker)
  {
    uint64_t lifecycle[OASIS_LF_LIFECYCLE_COUNT]{};
    assert(oasis_lf_worker_lifecycle(worker, lifecycle,
                                     OASIS_LF_LIFECYCLE_COUNT) ==
           static_cast<int>(OASIS_LF_LIFECYCLE_COUNT));
    for (uint32_t transition = 0; transition < OASIS_LF_LIFECYCLE_COUNT;
         ++transition)
      assert(lifecycle[transition] == 100u);
  }
}
}

int main()
{
  check_100_cycles(1, 20, 801);
  check_100_cycles(2, 20, 802);
  check_100_cycles(4, 20, 804);
  check_100_cycles(8, 20, 808);
  check_100_cycles(2, 20, 902);
  check_100_cycles(4, 20, 904);
  check_100_cycles(8, 20, 908);
  check_100_cycles(16, 20, 916);
  check_100_cycles(32, 32, 932);
  return 0;
}
