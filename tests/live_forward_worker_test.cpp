#include "live_forward_trace.h"
#include "trace_ring.h"

#include <array>
#include <cassert>
#include <cstdint>
#include <cstring>
#include <vector>

static uint32_t current_pc = 0x100u;

static oasis_lf_cpu_state cpu_state(uint32_t pc, uint16_t sr = 0)
{
  oasis_lf_cpu_state state{};
  state.pc = pc;
  state.sr = sr;
  state.usp = 0x10000u;
  state.isp = 0x20000u;
  return state;
}

static void execute(uint16_t opcode, uint32_t next_pc, uint16_t sr = 0,
                    uint32_t d0_after = 0, bool change_d0 = false)
{
  oasis_lf_cpu_state before = cpu_state(current_pc, sr);
  oasis_lf_cpu_state after = before;
  oasis_lf_instruction_begin(before.pc, &before);
  oasis_lf_instruction_set_opcode(opcode, &before);
  after.pc = next_pc;
  if (change_d0)
    after.d[0] = d0_after;
  oasis_lf_instruction_end(next_pc, &after, 0);
  current_pc = next_pc;
}

static oasis_lf_result result_for(uint32_t worker)
{
  oasis_lf_result result{};
  assert(oasis_lf_result_info(worker, &result));
  return result;
}

static void audit_and_ack(uint32_t worker, uint64_t capture,
                          uint64_t generation, uint64_t run, uint64_t epoch)
{
  const oasis_lf_result result = result_for(worker);
  assert(oasis_lf_mark_audited(worker, generation, result.valid));
  assert(oasis_lf_ack(worker, capture, generation, run, epoch));
}

static std::array<uint64_t, OASIS_LF_METRICS_COUNT> metrics_snapshot()
{
  std::array<uint64_t, OASIS_LF_METRICS_COUNT> values{};
  assert(oasis_lf_metrics_get(values.data(),
                              static_cast<uint32_t>(values.size())) ==
         static_cast<int>(values.size()));
  return values;
}

static std::vector<oasis_lf_record> copy_records(
  uint32_t worker, const oasis_lf_result& result)
{
  std::vector<oasis_lf_record> records(result.record_count);
  assert(oasis_lf_result_copy(worker, result.generation, 0, records.data(),
                              static_cast<uint32_t>(records.size())) ==
         result.record_count);
  return records;
}

static void test_flow_classification()
{
  uint16_t flags = 0;
  assert(oasis_lf_flow_flags(0x6602u, 0, 0, 0, &flags));
  assert((flags & OASIS_LF_BRANCH_TAKEN) != 0);
  flags = 0;
  assert(oasis_lf_flow_flags(0x6702u, 0, 0, 0, &flags));
  assert((flags & OASIS_LF_BRANCH_NOT_TAKEN) != 0);
  flags = 0;
  assert(oasis_lf_flow_flags(0x6000u, 0, 0, 0, &flags));
  assert((flags & OASIS_LF_BRANCH_TAKEN) != 0);
  flags = 0;
  assert(oasis_lf_flow_flags(0x6100u, 0, 0, 0, &flags));
  assert((flags & OASIS_LF_BRANCH_TAKEN) != 0);
  flags = 0;
  assert(oasis_lf_flow_flags(0x51c8u, 0, 1, 1, &flags));
  assert((flags & OASIS_LF_BRANCH_TAKEN) != 0);
  flags = 0;
  assert(oasis_lf_flow_flags(0x51c8u, 0, 0xffffu, 1, &flags));
  assert((flags & OASIS_LF_BRANCH_NOT_TAKEN) != 0);
  for (uint16_t opcode : {0x4ec0u, 0x4e80u, 0x4e75u, 0x4e73u, 0x4e77u})
  {
    flags = 0;
    assert(oasis_lf_flow_flags(opcode, 0, 0, 0, &flags));
  }
}

static void test_legacy_snapshot_projects_shared_ring()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  current_pc = 0x80u;
  execute(0x4e71u, 0x82u);
  execute(0x4e75u, 0x90u);
  assert(oasis_lf_stream_sequence() == 2);
  assert(gpgx_trace_ring_latest() == 2);
  assert(gpgx_trace_ring_count() == 2);
  gpgx_trace_record projected[2]{};
  assert(gpgx_trace_ring_copy(1, projected, 2) == 2);
  assert(projected[0].sequence == 1 && projected[0].pc == 0x80u);
  assert(projected[0].opcode == 0x4e71u);
  assert(projected[1].sequence == 2 && projected[1].pc == 0x82u);
  assert(projected[1].opcode == 0x4e75u);
}

static void test_depth_copy_immutability_and_reconnect()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  const uint64_t epoch = oasis_lf_epoch();
  current_pc = 0x100u;
  assert(oasis_lf_request(0, 1001, 1, 77, epoch));
  for (uint32_t i = 0; i < 20; ++i)
  {
    const bool taken = (i % 2u) == 0;
    execute(taken ? 0x6602u : 0x6702u,
            current_pc + (taken ? 8u : 4u));
  }
  oasis_lf_result first = result_for(0);
  assert(first.valid == 1);
  assert(first.run_id == 77 && first.epoch == epoch);
  assert(first.configured_depth == 20 && first.consumed_depth == 20);
  assert(first.termination_reason == OASIS_LF_END_DEPTH_LIMIT);
  assert(first.record_count == 20 && first.records_bytes == 20u * 32u);
  assert(first.exit_stream_sequence - first.entry_stream_sequence == 20);
  assert(first.exit_instruction_sequence - first.entry_instruction_sequence == 20);
  std::vector<oasis_lf_record> before = copy_records(0, first);
  assert(before.front().pc == 0x100u);
  assert((before.front().kind_flags & OASIS_LF_BRANCH_TAKEN) != 0);
  assert((before[1].kind_flags & OASIS_LF_BRANCH_NOT_TAKEN) != 0);

  for (uint32_t i = 0; i < 5000; ++i)
    execute(0x4e71u, current_pc + 2u);
  std::vector<oasis_lf_record> after = copy_records(0, first);
  assert(std::memcmp(before.data(), after.data(),
                     before.size() * sizeof(oasis_lf_record)) == 0);
  assert(!oasis_lf_ack(0, 1001, 2, 77, epoch));
  assert(!oasis_lf_ack(0, 1001, 1, 77, epoch + 1u));
  audit_and_ack(0, 1001, 1, 77, epoch);

  uint64_t old_exit = first.exit_stream_sequence;
  assert(!oasis_lf_request(0, 1002, 1, 77, epoch));
  assert(oasis_lf_request(0, 1002, 2, 77, epoch));
  execute(0x4e71u, current_pc + 2u);
  oasis_lf_epoch_break(nullptr);
  oasis_lf_result rejected = result_for(0);
  assert(rejected.valid == 0);
  assert(rejected.capture_id == 1002 && rejected.generation == 2);
  assert(rejected.epoch == epoch);
  assert(oasis_lf_epoch() == epoch + 1u);
  assert(rejected.entry_stream_sequence > old_exit);
  audit_and_ack(0, 1002, 2, 77, epoch);
}

static void test_exception_boundary_and_epoch_fail_closed()
{
  assert(oasis_lf_configure(1, 1, 16u * 1024u));
  uint64_t epoch = oasis_lf_epoch();
  current_pc = 0x200u;
  assert(oasis_lf_request(0, 2001, 1, 88, epoch));
  oasis_lf_cpu_state entry = cpu_state(current_pc);
  oasis_lf_boundary(&entry);
  oasis_lf_cpu_state interrupt_state = cpu_state(0x400u, 0x2000u);
  oasis_lf_exception(26, 0x200u, 0x400u, 1, &interrupt_state);
  oasis_lf_result result = result_for(0);
  assert(result.valid == 1);
  assert(result.consumed_depth == 1);
  assert(result.termination_reason == OASIS_LF_END_DEPTH_LIMIT);
  assert(result.record_count == 1);
  std::vector<oasis_lf_record> records = copy_records(0, result);
  assert((records[0].kind_flags & OASIS_LF_EXCEPTION_EVENT) != 0);
  assert((records[0].kind_flags & OASIS_LF_ASYNCHRONOUS) != 0);
  assert(records[0].pc == 0x200u && records[0].next_pc == 0x400u);
  assert(result.exit_state.pc == 0x400u);
  audit_and_ack(0, 2001, 1, 88, epoch);

  assert(oasis_lf_configure(1, 20, 16u * 1024u));
  epoch = oasis_lf_epoch();
  current_pc = 0x500u;
  assert(oasis_lf_request(0, 2003, 1, 88, epoch));
  oasis_lf_cpu_state instruction_state = cpu_state(current_pc);
  oasis_lf_instruction_begin(current_pc, &instruction_state);
  oasis_lf_instruction_set_opcode(0x4afcu, &instruction_state);
  oasis_lf_cpu_state exception_state = cpu_state(0x800u, 0x2000u);
  oasis_lf_exception(4, current_pc, exception_state.pc, 0, &exception_state);
  oasis_lf_instruction_end(exception_state.pc, &exception_state, 0);
  oasis_lf_result synchronous = result_for(0);
  assert(synchronous.valid == 0);
  assert(synchronous.termination_reason == OASIS_LF_END_CAPTURE_ERROR);
  assert(synchronous.record_count == 1);
  std::vector<oasis_lf_record> synchronous_records =
    copy_records(0, synchronous);
  assert((synchronous_records[0].kind_flags & OASIS_LF_FAULTED) != 0);
  audit_and_ack(0, 2003, 1, 88, epoch);

  assert(oasis_lf_configure(1, 20, 16u * 1024u));
  epoch = oasis_lf_epoch();
  current_pc = 0x600u;
  assert(oasis_lf_request(0, 2002, 1, 88, epoch));
  execute(0x4e71u, current_pc + 2u);
  oasis_lf_epoch_break(nullptr);
  result = result_for(0);
  assert(result.valid == 0);
  assert(result.termination_reason == OASIS_LF_END_CAPTURE_ERROR);
  assert(result.epoch == epoch);
  audit_and_ack(0, 2002, 1, 88, epoch);
}

static void test_memory_retention_and_unsupported_paths()
{
  const uint32_t small_memory = static_cast<uint32_t>(
    sizeof(oasis_lf_result) + 5u * sizeof(oasis_lf_record));
  assert(oasis_lf_configure(1, 20, small_memory));
  uint64_t epoch = oasis_lf_epoch();
  current_pc = 0x1000u;
  assert(oasis_lf_request(0, 3001, 1, 99, epoch));
  for (uint32_t i = 0; i < 10; ++i)
    execute(0x4e71u, current_pc + 2u);
  oasis_lf_result memory = result_for(0);
  assert(memory.valid == 1);
  assert(memory.termination_reason == OASIS_LF_END_MEMORY_LIMIT);
  assert(memory.record_count == 4);
  assert(memory.consumed_memory_bytes <= memory.configured_memory_bytes);
  audit_and_ack(0, 3001, 1, 99, epoch);

  assert(oasis_lf_configure(1, 100000, 1024u * 1024u));
  epoch = oasis_lf_epoch();
  current_pc = 0x2000u;
  assert(oasis_lf_request(0, 3002, 1, 99, epoch));
  for (uint32_t i = 0; i < OASIS_LF_RING_CAPACITY + 4u; ++i)
    execute(0x4e71u, current_pc + 2u);
  oasis_lf_result retention = result_for(0);
  assert(retention.valid == 1);
  assert(retention.termination_reason == OASIS_LF_END_RETENTION_LIMIT);
  assert(retention.record_count == OASIS_LF_RING_CAPACITY - 1u);
  audit_and_ack(0, 3002, 1, 99, epoch);

  assert(oasis_lf_configure(1, 100, 16u * 1024u));
  epoch = oasis_lf_epoch();
  current_pc = 0x3000u;
  assert(oasis_lf_request(0, 3003, 1, 99, epoch));
  oasis_lf_cpu_state state = cpu_state(current_pc);
  oasis_lf_instruction_begin(current_pc, &state);
  oasis_lf_instruction_set_opcode(0x4e71u, &state);
  oasis_lf_instruction_begin(current_pc + 2u, &state);
  oasis_lf_instruction_set_opcode(0x4e71u, &state);
  oasis_lf_instruction_end(current_pc + 4u, &state, 0);
  oasis_lf_instruction_end(current_pc + 4u, &state, 0);
  oasis_lf_result unsupported = result_for(0);
  assert(unsupported.valid == 0);
  assert(unsupported.termination_reason == OASIS_LF_END_UNSUPPORTED_PATH);
  audit_and_ack(0, 3003, 1, 99, epoch);
}

static void test_recorder_enable_boundary()
{
  assert(oasis_lf_configure(1, 20, 4096));
  uint64_t epoch = oasis_lf_epoch();
  assert(oasis_lf_set_enabled(0));
  assert(oasis_lf_epoch() == epoch + 1u);
  assert(!oasis_lf_request(0, 4001, 1, 111, epoch));
  current_pc = 0x5000u;
  execute(0x4e71u, current_pc + 2u);
  assert(oasis_lf_stream_sequence() == 0);
  assert(oasis_lf_instruction_sequence() == 0);
  assert(oasis_lf_set_enabled(1));
  execute(0x4e71u, current_pc + 2u);
  assert(oasis_lf_stream_sequence() == 1);
  assert(oasis_lf_instruction_sequence() == 1);
}

static void test_cpu_stop_boundary()
{
  assert(oasis_lf_configure(1, 20, 4096));
  uint64_t epoch = oasis_lf_epoch();
  current_pc = 0x7000u;
  assert(oasis_lf_request(0, 5001, 1, 123, epoch));
  oasis_lf_cpu_state state = cpu_state(current_pc);
  oasis_lf_boundary(&state);
  state.pc = 0x7010u;
  oasis_lf_cpu_stop(&state);
  oasis_lf_result stopped = result_for(0);
  assert(stopped.valid == 1);
  assert(stopped.termination_reason == OASIS_LF_END_CPU_STOP);
  assert(stopped.exit_state.pc == 0x7010u);
  assert(stopped.record_count == 1);
  std::vector<oasis_lf_record> records = copy_records(0, stopped);
  assert(records[0].kind_flags == OASIS_LF_CPU_STOP_EVENT);
  assert(records[0].pc == 0x7010u && records[0].next_pc == 0x7010u);
  audit_and_ack(0, 5001, 1, 123, epoch);

  assert(oasis_lf_configure(1, 20, 4096));
  epoch = oasis_lf_epoch();
  current_pc = 0x7100u;
  assert(oasis_lf_request(0, 5002, 1, 123, epoch));
  state = cpu_state(current_pc);
  oasis_lf_instruction_begin(current_pc, &state);
  oasis_lf_instruction_set_opcode(0x4e71u, &state);
  oasis_lf_cpu_stop(&state);
  state.pc = current_pc + 2u;
  oasis_lf_instruction_end(state.pc, &state, 1);
  stopped = result_for(0);
  assert(stopped.valid == 1);
  assert(stopped.termination_reason == OASIS_LF_END_CPU_STOP);
  assert(stopped.record_count == 1);
  records = copy_records(0, stopped);
  assert((records[0].kind_flags & OASIS_LF_INSTRUCTION) != 0);
  assert((records[0].kind_flags & OASIS_LF_CPU_STOP_EVENT) == 0);
  audit_and_ack(0, 5002, 1, 123, epoch);
}

static void test_dynamic_pool_preflight()
{
  oasis_lf_memory_plan plan{};
  std::array<uint64_t, OASIS_LF_PLAN_COUNT> values{};
  assert(oasis_lf_memory_plan_get(128, 20, 65536, &plan));
  assert(oasis_lf_memory_plan_values(128, 20, 65536, values.data(),
                                    static_cast<uint32_t>(values.size())) ==
         static_cast<int>(values.size()));
  assert(plan.worker_count == 128);
  assert(values[0] == plan.worker_count);
  assert(values[5] == plan.descriptor_bytes_each);
  assert(values[7] == plan.result_bytes_each);
  assert(values[12] == plan.shared_ring_bytes);
  assert(values[15] == plan.total_native_bytes);
  assert(plan.descriptor_bytes_each > sizeof(oasis_lf_result));
  assert(plan.result_bytes_each <= 65536);
  assert(plan.shared_ring_bytes > 0);
  assert(plan.total_native_bytes > plan.dynamic_bytes);
  const uint64_t exact_allocation = plan.total_native_bytes;
  assert(!oasis_lf_memory_plan_get(OASIS_LF_MAX_WORKERS + 1u, 20,
                                   65536, &plan));

  assert(!oasis_lf_configure_bounded(128, 20, 65536,
                                     exact_allocation - 1u));
  assert(oasis_lf_result_state(0) == 0);
  assert(oasis_lf_configure_bounded(128, 20, 65536,
                                    exact_allocation));
  assert(metrics_snapshot()[0] == 128);
}

static void test_overlapping_pool_lifecycle_and_isolation()
{
  constexpr uint32_t count = 8;
  assert(oasis_lf_configure(count, 6, 64u * 1024u));
  const uint64_t epoch = oasis_lf_epoch();
  current_pc = 0x4000u;
  for (uint32_t worker = 0; worker < count; ++worker)
    assert(oasis_lf_request(worker, 700u + worker, 1, 700, epoch));

  oasis_lf_cpu_state state = cpu_state(current_pc);
  oasis_lf_boundary(&state);
  oasis_lf_boundary(&state);
  assert(oasis_lf_result_state(0) == 2);
  assert(oasis_lf_result_state(1) == 1);
  for (uint32_t instruction = 0; instruction < count; ++instruction)
    execute(0x4e71u, current_pc + 2u);

  auto live = metrics_snapshot();
  assert(live[3] == count);
  assert(live[6] == count);
  assert(live[9] == count);
  assert(live[23] == count);
  assert(live[24] == 0);

  for (uint32_t flow = 0; flow < 6; ++flow)
    execute(0x6602u, current_pc + 8u);
  live = metrics_snapshot();
  assert(live[3] == 0);
  assert(live[4] == count);
  assert(live[7] == count);
  assert(live[8] == count);
  assert(live[10] == count);
  assert(live[18] == count);

  std::vector<std::vector<oasis_lf_record>> before;
  before.reserve(count);
  uint64_t last_entry = 0;
  for (uint32_t worker = 0; worker < count; ++worker)
  {
    const oasis_lf_result result = result_for(worker);
    assert(result.valid == 1);
    assert(result.capture_id == 700u + worker);
    assert(result.generation == 1);
    assert(result.worker_id == worker);
    assert(result.entry_stream_sequence == worker + 1u);
    assert(result.entry_stream_sequence > last_entry);
    assert(result.exit_stream_sequence == 15);
    assert(result.consumed_depth == 6);
    last_entry = result.entry_stream_sequence;
    before.push_back(copy_records(worker, result));
  }
  for (uint32_t instruction = 0;
       instruction < OASIS_LF_RING_CAPACITY + 16u; ++instruction)
    execute(0x4e71u, current_pc + 2u);
  for (uint32_t worker = 0; worker < count; ++worker)
  {
    const oasis_lf_result result = result_for(worker);
    const auto after = copy_records(worker, result);
    assert(before[worker].size() == after.size());
    assert(std::memcmp(before[worker].data(), after.data(),
                       after.size() * sizeof(oasis_lf_record)) == 0);
  }
  assert(metrics_snapshot()[26] > 0);

  audit_and_ack(0, 700, 1, 700, epoch);
  assert(oasis_lf_result_state(1) == 4);
  assert(!oasis_lf_ack(1, 700, 1, 700, epoch));
  const oasis_lf_result later = result_for(1);
  assert(later.capture_id == 701);

  assert(oasis_lf_request(0, 900, 2, 700, epoch));
  for (uint32_t flow = 0; flow < 6; ++flow)
    execute(0x6602u, current_pc + 8u);
  const oasis_lf_result reused = result_for(0);
  assert(reused.valid == 1 && reused.generation == 2);
  assert(reused.capture_id == 900);
  assert(reused.entry_stream_sequence > 14u);
  audit_and_ack(0, 900, 2, 700, epoch);
  audit_and_ack(1, 701, 1, 700, epoch);
  for (uint32_t worker = 2; worker < count; ++worker)
    audit_and_ack(worker, 700u + worker, 1, 700, epoch);

  live = metrics_snapshot();
  assert(live[5] == 0);
  assert(live[14] > 0);
  assert(live[15] == 0);
}

static void test_capture_identity_collision_fails_closed()
{
  assert(oasis_lf_configure(2, 1, 64u * 1024u));
  const uint64_t epoch = oasis_lf_epoch();
  current_pc = 0x9000u;
  assert(oasis_lf_request(0, 501, 1, 900, epoch));
  assert(!oasis_lf_request(1, 501, 1, 900, epoch));
  assert(oasis_lf_request(1, 502, 1, 900, epoch));
  execute(0x6602u, current_pc + 8u);
  execute(0x6602u, current_pc + 8u);
  auto live = metrics_snapshot();
  assert(live[9] == 2);
  assert(live[10] == 2);
  assert(live[13] == 1);
  assert(live[15] == 1);
  const oasis_lf_result first = result_for(0);
  const oasis_lf_result second = result_for(1);
  assert(first.capture_id != second.capture_id);
  audit_and_ack(0, 501, 1, 900, epoch);
  audit_and_ack(1, 502, 1, 900, epoch);
}

int main()
{
  assert(oasis_lf_recording_enabled == 0);
  assert(oasis_lf_stream_sequence() == 0);
  test_flow_classification();
  test_legacy_snapshot_projects_shared_ring();
  test_depth_copy_immutability_and_reconnect();
  test_exception_boundary_and_epoch_fail_closed();
  test_memory_retention_and_unsupported_paths();
  test_recorder_enable_boundary();
  test_cpu_stop_boundary();
  test_dynamic_pool_preflight();
  test_overlapping_pool_lifecycle_and_isolation();
  test_capture_identity_collision_fails_closed();
  return 0;
}
