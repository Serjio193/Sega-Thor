#ifndef _WIN32
#define _POSIX_C_SOURCE 200809L
#endif

#include "live_forward_internal.h"

#include <string.h>
#include <time.h>
#ifdef _WIN32
#include <windows.h>
#endif

static uint64_t lf_now_ns(void)
{
#ifdef _WIN32
  LARGE_INTEGER counter;
  LARGE_INTEGER frequency;
  if (QueryPerformanceCounter(&counter) &&
      QueryPerformanceFrequency(&frequency) && frequency.QuadPart > 0)
  {
    uint64_t ticks = (uint64_t)counter.QuadPart;
    uint64_t rate = (uint64_t)frequency.QuadPart;
    return (ticks / rate) * 1000000000u +
      ((ticks % rate) * 1000000000u) / rate;
  }
#endif
#if !defined(_WIN32) && defined(CLOCK_MONOTONIC)
  {
    struct timespec now;
    if (clock_gettime(CLOCK_MONOTONIC, &now) == 0)
      return (uint64_t)now.tv_sec * 1000000000u + (uint64_t)now.tv_nsec;
  }
#endif
  {
    clock_t ticks = clock();
    if (ticks != (clock_t)-1)
      return ((uint64_t)ticks * 1000000000u) / CLOCKS_PER_SEC;
  }
  return UINT64_MAX;
}

static uint32_t complete_state(uint32_t state)
{
  return state == LF_COMPLETE || state == LF_ANALYZING;
}

void lf_update_peaks(void)
{
  if (metrics.capturing_count > metrics.peak_capturing)
    metrics.peak_capturing = metrics.capturing_count;
  if (metrics.complete_count > metrics.peak_complete)
    metrics.peak_complete = metrics.complete_count;
  if (metrics.occupied_count > metrics.peak_occupied)
    metrics.peak_occupied = metrics.occupied_count;
}

void lf_set_state(lf_worker *worker, uint32_t state)
{
  uint32_t old = worker->state;
  if (old == state)
    return;
  if (old == LF_PENDING)
    metrics.pending_count--;
  else if (old == LF_CAPTURING)
    metrics.capturing_count--;
  else if (complete_state(old))
    metrics.complete_count--;
  if (old != LF_FREE)
    metrics.occupied_count--;
  worker->state = state;
  if (state == LF_PENDING)
    metrics.pending_count++;
  else if (state == LF_CAPTURING)
    metrics.capturing_count++;
  else if (complete_state(state))
    metrics.complete_count++;
  if (state != LF_FREE)
    metrics.occupied_count++;
  lf_update_peaks();
}

static void active_push(uint32_t worker_id)
{
  active_queue[active_tail] = worker_id;
  active_tail = (active_tail + 1u) % worker_count;
  active_count++;
}

static uint32_t active_front(void)
{
  return active_count ? active_queue[active_head] : UINT32_MAX;
}

static void active_pop(void)
{
  if (!active_count)
    return;
  active_head = (active_head + 1u) % worker_count;
  active_count--;
}

static uint32_t pending_pop(void)
{
  while (metrics.pending_count)
  {
    uint32_t index = pending_queue[pending_head];
    lf_worker *worker = &workers[index];
    pending_head = (pending_head + 1u) % worker_count;
    if (worker->state == LF_PENDING)
      return index;
  }
  return UINT32_MAX;
}

void lf_seal_worker(uint32_t index, uint32_t reason,
                    const oasis_lf_cpu_state *exit_state)
{
  lf_worker *worker;
  uint64_t end;
  uint32_t count;
  uint32_t i;
  uint64_t copy_started;
  uint64_t copy_finished;
  if (index >= worker_count)
    return;
  worker = &workers[index];
  if (worker->state != LF_CAPTURING)
    return;
  end = stream_sequence + 1u;
  count = (uint32_t)(end - worker->first_record);
  copy_started = lf_now_ns();
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
  if (count > record_capacity || count > OASIS_LF_RING_CAPACITY)
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
  copy_finished = lf_now_ns();
  if (copy_started != UINT64_MAX && copy_finished != UINT64_MAX &&
      copy_finished >= copy_started)
    worker->result.copy_duration_ns = copy_finished - copy_started;

  metrics.captures_completed++;
  worker->capture_complete_count++;
  metrics.total_segment_bytes += worker->result.consumed_memory_bytes;
  if (!worker->result.valid)
    metrics.captures_invalid++;
  if (reason == OASIS_LF_END_DEPTH_LIMIT)
    metrics.depth_limit_endings++;
  else if (reason == OASIS_LF_END_MEMORY_LIMIT)
    metrics.memory_limit_endings++;
  else if (reason == OASIS_LF_END_RETENTION_LIMIT)
    metrics.ring_retention_failures++;
  else if (reason == OASIS_LF_END_UNSUPPORTED_PATH)
    metrics.unsupported_path_endings++;
  lf_set_state(worker, LF_COMPLETE);
}

void lf_finish_due(const oasis_lf_cpu_state *state, uint32_t stopped)
{
  if (!oasis_lf_recording_enabled || instruction_nesting != 0)
    return;
  while (active_count)
  {
    uint32_t index = active_front();
    lf_worker *worker = &workers[index];
    uint64_t used = stream_sequence + 1u - worker->first_record;
    uint64_t consumed = control_flow_sequence - worker->entry_flow;
    uint32_t reason = OASIS_LF_END_NONE;
    if (worker->pending_end != OASIS_LF_END_NONE)
      reason = worker->pending_end;
    else if (consumed >= worker->configured_depth)
      reason = OASIS_LF_END_DEPTH_LIMIT;
    else if (used + 2u > record_capacity)
      reason = OASIS_LF_END_MEMORY_LIMIT;
    else if (used + 2u > OASIS_LF_RING_CAPACITY)
      reason = OASIS_LF_END_RETENTION_LIMIT;
    else if (stopped)
      reason = OASIS_LF_END_CPU_STOP;
    if (reason == OASIS_LF_END_NONE)
      break;
    lf_seal_worker(index, reason, state);
    active_pop();
  }
}

void lf_boundary(const oasis_lf_cpu_state *state)
{
  uint64_t next_instruction;
  uint32_t index;
  lf_finish_due(state, 0);
  if (!oasis_lf_recording_enabled || instruction_nesting != 0 ||
      !state || !metrics.pending_count)
    return;
  next_instruction = instruction_sequence + 1u;
  if (last_started_instruction == next_instruction)
    return;
  index = pending_pop();
  if (index == UINT32_MAX)
    return;
  {
    lf_worker *worker = &workers[index];
    uint64_t entry_stream = stream_sequence + 1u;
    memset(&worker->result, 0, sizeof(worker->result));
    worker->result.capture_id = worker->capture_id;
    worker->result.generation = worker->generation;
    worker->result.run_id = worker->run_id;
    worker->result.epoch = worker->epoch;
    worker->result.worker_id = index;
    worker->result.entry_stream_sequence = entry_stream;
    worker->result.entry_instruction_sequence = next_instruction;
    worker->result.entry_control_flow_sequence = control_flow_sequence;
    worker->result.entry_state = *state;
    worker->first_record = entry_stream;
    worker->entry_flow = control_flow_sequence;
    worker->configured_depth = depth_limit;
    worker->configured_memory_bytes = memory_bytes;
    worker->record_count = 0;
    worker->invalid = 0;
    worker->pending_end = OASIS_LF_END_NONE;
    worker->host_audited = 0;
    if (worker->ever_used == 0)
    {
      worker->ever_used = 1;
      metrics.workers_ever_used++;
    }
    if (last_entry_stream_sequence &&
        entry_stream <= last_entry_stream_sequence)
      metrics.duplicate_entry_stream_sequences++;
    else
      metrics.unique_entry_stream_sequences++;
    last_entry_stream_sequence = entry_stream;
    last_started_instruction = next_instruction;
    metrics.captures_started++;
    worker->capture_start_count++;
    lf_set_state(worker, LF_CAPTURING);
    active_push(index);
  }
}

void oasis_lf_boundary(const oasis_lf_cpu_state *state)
{
  lf_boundary(state);
}

void lf_cancel_pending(void)
{
  uint32_t remaining = metrics.pending_count;
  while (remaining--)
  {
    uint32_t index = pending_queue[pending_head];
    lf_worker *worker = &workers[index];
    pending_head = (pending_head + 1u) % worker_count;
    if (worker->state != LF_PENDING)
      continue;
    lf_identity_remove(worker->capture_id);
    worker->capture_id = 0;
    worker->generation = 0;
    worker->run_id = 0;
    worker->epoch = 0;
    worker->host_audited = 0;
    metrics.captures_dropped++;
    lf_set_state(worker, LF_FREE);
  }
  pending_tail = pending_head;
}

void oasis_lf_cancel_pending(void)
{
  lf_cancel_pending();
}

void lf_cancel_all_active(uint32_t reason,
                          const oasis_lf_cpu_state *exit_state)
{
  while (active_count)
  {
    uint32_t index = active_front();
    if (reason == OASIS_LF_END_CAPTURE_ERROR ||
        reason == OASIS_LF_END_UNSUPPORTED_PATH)
      workers[index].invalid = 1;
    lf_seal_worker(index, reason, exit_state);
    active_pop();
  }
}

int oasis_lf_set_enabled(uint32_t enabled)
{
  uint32_t i;
  enabled = enabled != 0;
  if (enabled && (!workers || !ring_storage))
    return 0;
  if (active_count || metrics.pending_count || instruction_nesting ||
      metrics.occupied_count)
    return 0;
  if (oasis_lf_recording_enabled == enabled)
    return 1;
  oasis_lf_epoch_break(0);
  oasis_lf_recording_enabled = enabled;
  if (ring_storage)
    for (i = 0; i < OASIS_LF_RING_CAPACITY; ++i)
      ring_storage[i].valid = 0;
  return 1;
}

void oasis_lf_cpu_stop(const oasis_lf_cpu_state *state)
{
  uint32_t offset;
  oasis_lf_record record;
  if (!oasis_lf_recording_enabled || !state)
    return;
  if (instruction_nesting)
  {
    for (offset = 0; offset < active_count; ++offset)
    {
      uint32_t slot = (active_head + offset) % worker_count;
      workers[active_queue[slot]].pending_end = OASIS_LF_END_CPU_STOP;
    }
    return;
  }
  memset(&record, 0, sizeof(record));
  record.instruction_sequence = instruction_sequence;
  record.pc = state->pc;
  record.next_pc = state->pc;
  record.kind_flags = OASIS_LF_CPU_STOP_EVENT;
  lf_append_record(record);
  lf_cancel_all_active(OASIS_LF_END_CPU_STOP, state);
  lf_cancel_pending();
}

void oasis_lf_epoch_break(const oasis_lf_cpu_state *state)
{
  uint32_t offset;
  for (offset = 0; offset < active_count; ++offset)
  {
    uint32_t slot = (active_head + offset) % worker_count;
    workers[active_queue[slot]].invalid = 1;
  }
  lf_cancel_all_active(OASIS_LF_END_CAPTURE_ERROR, state);
  lf_cancel_pending();
  instruction_nesting = 0;
  stream_sequence = 0;
  last_entry_stream_sequence = 0;
  instruction_sequence = 0;
  control_flow_sequence = 0;
  last_started_instruction = UINT64_MAX;
  if (ring_storage)
    memset(ring_storage, 0,
           OASIS_LF_RING_CAPACITY * sizeof(*ring_storage));
  if (++runtime_epoch == 0)
    runtime_epoch = 1;
}
