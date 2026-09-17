#include "live_forward_trace.h"
#include "trace_ring.h"

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
  assert(oasis_lf_ack(0, 1001, 1, 77, epoch));

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
  assert(oasis_lf_ack(0, 1002, 2, 77, epoch));
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
  assert(oasis_lf_ack(0, 2001, 1, 88, epoch));

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
  assert(oasis_lf_ack(0, 2003, 1, 88, epoch));

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
  assert(oasis_lf_ack(0, 2002, 1, 88, epoch));
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
  assert(oasis_lf_ack(0, 3001, 1, 99, epoch));

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
  assert(oasis_lf_ack(0, 3002, 1, 99, epoch));

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
  assert(oasis_lf_ack(0, 3003, 1, 99, epoch));
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
  assert(oasis_lf_ack(0, 5001, 1, 123, epoch));

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
  assert(oasis_lf_ack(0, 5002, 1, 123, epoch));
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
  return 0;
}
