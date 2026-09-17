#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "live_forward_internal.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

#define LF_DEPTH_MAX 1000000u
#define LF_MEMORY_MAX (1024u * 1024u)

static uint64_t lf_now_ns(void)
{
#ifdef _WIN32
  LARGE_INTEGER counter;
  LARGE_INTEGER frequency;
  if (QueryPerformanceCounter(&counter) &&
      QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0)
  {
    uint64_t ticks = (uint64_t)counter.QuadPart;
    uint64_t ticks_per_second = (uint64_t)frequency.QuadPart;
    return (ticks / ticks_per_second) * 1000000000u +
      ((ticks % ticks_per_second) * 1000000000u) / ticks_per_second;
  }
#endif
#if !defined(_WIN32) && defined(CLOCK_MONOTONIC)
  struct timespec now;
  if (clock_gettime(CLOCK_MONOTONIC, &now) == 0)
    return (uint64_t)now.tv_sec * 1000000000u +
      (uint64_t)now.tv_nsec;
#endif
  clock_t ticks = clock();
  if (ticks != (clock_t)-1)
    return ((uint64_t)ticks * 1000000000u) / CLOCKS_PER_SEC;
  return UINT64_MAX;
}

void lf_seal_worker(uint32_t index, uint32_t reason,
                    const oasis_lf_cpu_state *exit_state)
{
  lf_worker *worker = &workers[index];
  uint64_t end = stream_sequence + 1u;
  uint32_t count = (uint32_t)(end - worker->first_record);
  uint32_t i;
  uint64_t copy_started = lf_now_ns();
  if (worker->state == LF_PENDING)
  {
    worker->first_record = end;
    worker->result.entry_stream_sequence = end;
    worker->result.entry_instruction_sequence = instruction_sequence + 1u;
    worker->result.entry_control_flow_sequence = control_flow_sequence;
    count = 0;
  }
  worker->result.exit_stream_sequence = end;
  worker->result.exit_instruction_sequence = instruction_sequence + 1u;
  worker->result.exit_control_flow_sequence = control_flow_sequence;
  worker->result.termination_reason = reason;
  worker->result.configured_depth = worker->configured_depth;
  worker->result.configured_memory_bytes = worker->configured_memory_bytes;
  worker->result.consumed_depth =
    (uint32_t)(control_flow_sequence - worker->entry_flow);
  worker->result.record_count = count;
  worker->result.records_bytes = count * (uint32_t)sizeof(oasis_lf_record);
  worker->result.consumed_memory_bytes =
    (uint32_t)sizeof(oasis_lf_result) + worker->result.records_bytes;
  worker->result.valid = !worker->invalid &&
    reason != OASIS_LF_END_CAPTURE_ERROR &&
    reason != OASIS_LF_END_UNSUPPORTED_PATH &&
    reason != OASIS_LF_END_NONE &&
    worker->result.consumed_memory_bytes <= worker->configured_memory_bytes;
  if (exit_state)
    worker->result.exit_state = *exit_state;
  if (count != worker->record_count || count > record_capacity ||
      count > OASIS_LF_RING_CAPACITY)
    worker->result.valid = 0;
  for (i = 0; i < count && count <= record_capacity &&
       count <= OASIS_LF_RING_CAPACITY; ++i)
  {
    uint64_t seq = worker->first_record + i;
    lf_ring_slot *slot = &ring_storage[(seq - 1u) % OASIS_LF_RING_CAPACITY];
    if (!slot->valid || slot->sequence != seq)
    {
      worker->result.valid = 0;
      break;
    }
    worker->records[i] = slot->record;
  }
  {
    uint64_t copy_finished = lf_now_ns();
    if (copy_started != UINT64_MAX && copy_finished != UINT64_MAX &&
        copy_finished >= copy_started)
    {
      worker->result.copy_duration_ns = copy_finished - copy_started;
    }
  }
  if (active_worker == index)
    active_worker = UINT32_MAX;
  worker->state = LF_COMPLETE;
}

int oasis_lf_configure(uint32_t count, uint32_t depth, uint32_t bytes)
{
  uint32_t i;
  uint32_t capacity;
  if (count == 0 || count > OASIS_LF_MAX_WORKERS || depth == 0 ||
      depth > LF_DEPTH_MAX ||
      bytes <= sizeof(oasis_lf_result) + sizeof(oasis_lf_record) ||
      bytes > LF_MEMORY_MAX)
    return 0;
  for (i = 0; i < worker_count; ++i)
    if (workers[i].state != LF_FREE)
      return 0;
  capacity = (bytes - (uint32_t)sizeof(oasis_lf_result)) /
             (uint32_t)sizeof(oasis_lf_record);
  for (i = 0; i < OASIS_LF_MAX_WORKERS; ++i)
  {
    free(workers[i].records);
    memset(&workers[i], 0, sizeof(workers[i]));
  }
  memset(ring_storage, 0, sizeof(ring_storage));
  worker_count = count;
  depth_limit = depth;
  memory_bytes = bytes;
  record_capacity = capacity;
  next_worker = 0;
  active_worker = UINT32_MAX;
  instruction_nesting = 0;
  stream_sequence = 0;
  instruction_sequence = 0;
  control_flow_sequence = 0;
  oasis_lf_recording_enabled = 0;
  if (++runtime_epoch == 0)
    runtime_epoch = 1;
  for (i = 0; i < count; ++i)
  {
    workers[i].records = (oasis_lf_record *)calloc(capacity,
                                                   sizeof(oasis_lf_record));
    if (!workers[i].records)
    {
      while (i > 0)
        free(workers[--i].records);
      worker_count = 0;
      return 0;
    }
  }
  oasis_lf_recording_enabled = 1;
  return 1;
}

int oasis_lf_set_enabled(uint32_t enabled)
{
  uint32_t i;
  enabled = enabled != 0;
  if (active_worker != UINT32_MAX || instruction_nesting != 0)
    return 0;
  for (i = 0; i < worker_count; ++i)
    if (workers[i].state != LF_FREE)
      return 0;
  if (oasis_lf_recording_enabled == enabled)
    return 1;
  oasis_lf_epoch_break(0);
  oasis_lf_recording_enabled = enabled;
  return 1;
}

int oasis_lf_request(uint32_t worker_id, uint64_t capture_id,
                     uint64_t generation, uint64_t run_id, uint64_t epoch)
{
  lf_worker *worker;
  if (worker_id >= worker_count || capture_id == 0 || generation == 0 ||
      run_id == 0 || epoch == 0 || epoch != runtime_epoch)
    return 0;
  worker = &workers[worker_id];
  if (worker->state != LF_FREE || generation <= worker->last_generation)
    return 0;
  worker->capture_id = capture_id;
  worker->generation = generation;
  worker->last_generation = generation;
  worker->run_id = run_id;
  worker->epoch = epoch;
  worker->state = LF_PENDING;
  return 1;
}

void oasis_lf_cpu_stop(const oasis_lf_cpu_state *state)
{
  oasis_lf_record record;
  lf_worker *worker;
  if (!oasis_lf_recording_enabled || !state || active_worker == UINT32_MAX)
    return;
  worker = &workers[active_worker];
  if (instruction_nesting)
  {
    worker->pending_end = OASIS_LF_END_CPU_STOP;
    return;
  }
  memset(&record, 0, sizeof(record));
  record.instruction_sequence = instruction_sequence;
  record.pc = state->pc;
  record.next_pc = state->pc;
  record.kind_flags = OASIS_LF_CPU_STOP_EVENT;
  lf_append_record(record);
  lf_seal_worker(active_worker, OASIS_LF_END_CPU_STOP, state);
}

void oasis_lf_boundary(const oasis_lf_cpu_state *state)
{
  uint32_t offset;
  if (!oasis_lf_recording_enabled || !state || instruction_nesting != 0)
    return;
  if (active_worker != UINT32_MAX)
  {
    lf_worker *active = &workers[active_worker];
    uint64_t used = stream_sequence + 1u - active->first_record;
    uint64_t consumed = control_flow_sequence - active->entry_flow;
    if (active->pending_end != OASIS_LF_END_NONE)
      lf_seal_worker(active_worker, active->pending_end, state);
    else if (consumed == depth_limit)
      lf_seal_worker(active_worker, OASIS_LF_END_DEPTH_LIMIT, state);
    else if (consumed > depth_limit)
    {
      active->invalid = 1;
      lf_seal_worker(active_worker, OASIS_LF_END_CAPTURE_ERROR, state);
    }
    else if (used + 2u > record_capacity)
      lf_seal_worker(active_worker, OASIS_LF_END_MEMORY_LIMIT, state);
    else if (used + 2u > OASIS_LF_RING_CAPACITY)
      lf_seal_worker(active_worker, OASIS_LF_END_RETENTION_LIMIT, state);
    else
      return;
  }
  for (offset = 0; offset < worker_count; ++offset)
  {
    uint32_t index = (next_worker + offset) % worker_count;
    lf_worker *worker = &workers[index];
    if (worker->state != LF_PENDING)
      continue;
    memset(&worker->result, 0, sizeof(worker->result));
    worker->result.capture_id = worker->capture_id;
    worker->result.generation = worker->generation;
    worker->result.run_id = worker->run_id;
    worker->result.epoch = worker->epoch;
    worker->result.worker_id = index;
    worker->result.entry_stream_sequence = stream_sequence + 1u;
    worker->result.entry_instruction_sequence = instruction_sequence + 1u;
    worker->result.entry_control_flow_sequence = control_flow_sequence;
    worker->result.entry_state = *state;
    worker->first_record = stream_sequence + 1u;
    worker->entry_flow = control_flow_sequence;
    worker->configured_depth = depth_limit;
    worker->configured_memory_bytes = memory_bytes;
    worker->record_count = 0;
    worker->invalid = 0;
    worker->pending_end = OASIS_LF_END_NONE;
    worker->state = LF_CAPTURING;
    active_worker = index;
    next_worker = (index + 1u) % worker_count;
    break;
  }
}

void oasis_lf_epoch_break(const oasis_lf_cpu_state *state)
{
  uint32_t i;
  for (i = 0; i < worker_count; ++i)
  {
    lf_worker *worker = &workers[i];
    if (worker->state != LF_CAPTURING && worker->state != LF_PENDING)
      continue;
    if (worker->state == LF_PENDING)
    {
      memset(&worker->result, 0, sizeof(worker->result));
      worker->result.capture_id = worker->capture_id;
      worker->result.generation = worker->generation;
      worker->result.run_id = worker->run_id;
      worker->result.epoch = worker->epoch;
      worker->result.worker_id = i;
      worker->result.configured_depth = depth_limit;
      worker->result.configured_memory_bytes = memory_bytes;
    }
    worker->invalid = 1;
    lf_seal_worker(i, OASIS_LF_END_CAPTURE_ERROR, state);
  }
  active_worker = UINT32_MAX;
  instruction_nesting = 0;
  stream_sequence = 0;
  instruction_sequence = 0;
  control_flow_sequence = 0;
  memset(ring_storage, 0, sizeof(ring_storage));
  if (++runtime_epoch == 0)
    runtime_epoch = 1;
}

uint32_t oasis_lf_result_state(uint32_t worker_id)
{
  return worker_id < worker_count ? workers[worker_id].state : LF_FREE;
}

int oasis_lf_result_info(uint32_t worker_id, oasis_lf_result *result)
{
  if (!result || worker_id >= worker_count || workers[worker_id].state < LF_COMPLETE)
    return 0;
  *result = workers[worker_id].result;
  workers[worker_id].state = LF_ANALYZING;
  return 1;
}

uint32_t oasis_lf_result_copy(uint32_t worker_id, uint64_t generation,
                              uint32_t offset, oasis_lf_record *output,
                              uint32_t capacity)
{
  lf_worker *worker;
  uint32_t count;
  if (!output || worker_id >= worker_count)
    return 0;
  worker = &workers[worker_id];
  if (worker->state != LF_ANALYZING || worker->generation != generation ||
      offset > worker->result.record_count)
    return 0;
  count = worker->result.record_count - offset;
  if (count > capacity)
    count = capacity;
  if (offset + count > record_capacity)
    return 0;
  memcpy(output, worker->records + offset, count * sizeof(*output));
  return count;
}

int oasis_lf_ack(uint32_t worker_id, uint64_t capture_id,
                 uint64_t generation, uint64_t run_id, uint64_t epoch)
{
  lf_worker *worker;
  if (worker_id >= worker_count)
    return 0;
  worker = &workers[worker_id];
  if (worker->state != LF_ANALYZING || worker->capture_id != capture_id ||
      worker->generation != generation || worker->run_id != run_id ||
      worker->epoch != epoch)
    return 0;
  memset(&worker->result, 0, sizeof(worker->result));
  worker->capture_id = 0;
  worker->generation = 0;
  worker->run_id = 0;
  worker->epoch = 0;
  worker->record_count = 0;
  worker->state = LF_FREE;
  return 1;
}
