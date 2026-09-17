#include "live_forward_internal.h"

#include <limits.h>
#include <stdlib.h>
#include <string.h>

#define LF_DEPTH_MAX 1000000u
#define LF_MEMORY_MAX (1024u * 1024u)

lf_worker *workers;
oasis_lf_record *record_storage;
uint32_t *pending_queue;
uint32_t *active_queue;
lf_identity_entry *identity_table;
lf_ring_slot *ring_storage;
uint32_t worker_count;
uint32_t depth_limit;
uint32_t memory_bytes;
uint32_t record_capacity;
uint32_t identity_capacity;
uint32_t pending_head;
uint32_t pending_tail;
uint32_t active_head;
uint32_t active_tail;
uint32_t active_count;
uint64_t last_started_instruction = UINT64_MAX;
uint64_t stream_sequence;
uint64_t last_entry_stream_sequence;
uint64_t instruction_sequence;
uint64_t control_flow_sequence;
uint64_t runtime_epoch = 1;
uint32_t oasis_lf_recording_enabled;
lf_metrics metrics;

static int checked_add(uint64_t left, uint64_t right, uint64_t *result)
{
  if (UINT64_MAX - left < right)
    return 0;
  *result = left + right;
  return 1;
}

static int checked_mul(uint64_t left, uint64_t right, uint64_t *result)
{
  if (left && right > UINT64_MAX / left)
    return 0;
  *result = left * right;
  return 1;
}

static uint32_t next_power_of_two(uint32_t value)
{
  uint32_t result = 1;
  while (result < value && result <= UINT32_MAX / 2u)
    result <<= 1;
  return result >= value ? result : 0;
}

int oasis_lf_memory_plan_get(uint32_t count, uint32_t depth,
                             uint32_t bytes, oasis_lf_memory_plan *plan)
{
  uint64_t total = 0;
  uint64_t term = 0;
  uint32_t identity_slots;
  uint32_t capacity;
  if (!plan)
    return 0;
  memset(plan, 0, sizeof(*plan));
  if (count == 0 || count > OASIS_LF_MAX_WORKERS || depth == 0 ||
      depth > LF_DEPTH_MAX || bytes <= sizeof(oasis_lf_result) +
      sizeof(oasis_lf_record) || bytes > LF_MEMORY_MAX)
    return 0;
  capacity = (bytes - (uint32_t)sizeof(oasis_lf_result)) /
             (uint32_t)sizeof(oasis_lf_record);
  if (count > UINT32_MAX / 2u)
    return 0;
  identity_slots = next_power_of_two(count * 2u);
  if (!identity_slots)
    return 0;

  plan->worker_count = count;
  plan->depth = depth;
  plan->memory_bytes = bytes;
  plan->record_capacity = capacity;
  plan->identity_capacity = identity_slots;
  plan->descriptor_bytes_each = sizeof(lf_worker);
  plan->result_bytes_each = sizeof(oasis_lf_result) +
    (uint64_t)capacity * sizeof(oasis_lf_record);
  if (!checked_mul(count, sizeof(lf_worker), &plan->descriptor_bytes_total) ||
      !checked_mul(count, capacity, &term) ||
      !checked_mul(term, sizeof(oasis_lf_record),
                   &plan->result_buffers_bytes_total) ||
      !checked_mul(count, sizeof(uint32_t), &plan->pending_queue_bytes) ||
      !checked_mul(count, sizeof(uint32_t), &plan->active_queue_bytes) ||
      !checked_mul(identity_slots, sizeof(lf_identity_entry),
                   &plan->identity_table_bytes) ||
      !checked_mul(OASIS_LF_RING_CAPACITY, sizeof(lf_ring_slot),
                   &plan->shared_ring_bytes))
    return 0;
  if (!checked_add(plan->descriptor_bytes_total,
                   plan->result_buffers_bytes_total, &total) ||
      !checked_add(total, plan->pending_queue_bytes, &total) ||
      !checked_add(total, plan->active_queue_bytes, &total) ||
      !checked_add(total, plan->identity_table_bytes, &total))
    return 0;
  plan->dynamic_bytes = total;
  if (!checked_add(total, plan->shared_ring_bytes, &total) ||
      !checked_add(total, lf_instruction_stack_size(), &total))
    return 0;
  plan->instruction_stack_bytes = lf_instruction_stack_size();
  plan->total_native_bytes = total;
  return 1;
}

int oasis_lf_memory_plan_values(uint32_t count, uint32_t depth,
                                uint32_t bytes, uint64_t *output,
                                uint32_t capacity)
{
  oasis_lf_memory_plan plan;
  uint64_t values[OASIS_LF_PLAN_COUNT];
  uint32_t i;
  if (!output || capacity < OASIS_LF_PLAN_COUNT ||
      !oasis_lf_memory_plan_get(count, depth, bytes, &plan))
    return 0;
  values[0] = plan.worker_count;
  values[1] = plan.depth;
  values[2] = plan.memory_bytes;
  values[3] = plan.record_capacity;
  values[4] = plan.identity_capacity;
  values[5] = plan.descriptor_bytes_each;
  values[6] = plan.descriptor_bytes_total;
  values[7] = plan.result_bytes_each;
  values[8] = plan.result_buffers_bytes_total;
  values[9] = plan.pending_queue_bytes;
  values[10] = plan.active_queue_bytes;
  values[11] = plan.identity_table_bytes;
  values[12] = plan.shared_ring_bytes;
  values[13] = plan.instruction_stack_bytes;
  values[14] = plan.dynamic_bytes;
  values[15] = plan.total_native_bytes;
  for (i = 0; i < OASIS_LF_PLAN_COUNT; ++i)
    output[i] = values[i];
  return (int)OASIS_LF_PLAN_COUNT;
}

static void free_pool(void)
{
  free(workers);
  free(record_storage);
  free(pending_queue);
  free(active_queue);
  free(identity_table);
  free(ring_storage);
  workers = 0;
  record_storage = 0;
  pending_queue = 0;
  active_queue = 0;
  identity_table = 0;
  ring_storage = 0;
  worker_count = 0;
  depth_limit = 0;
  memory_bytes = 0;
  record_capacity = 0;
  identity_capacity = 0;
  pending_head = 0;
  pending_tail = 0;
  active_head = 0;
  active_tail = 0;
  active_count = 0;
}

int oasis_lf_configure_bounded(uint32_t count, uint32_t depth,
                               uint32_t bytes, uint64_t budget)
{
  oasis_lf_memory_plan plan;
  uint64_t record_count;
  uint64_t record_bytes;
  uint32_t i;
  if (!oasis_lf_memory_plan_get(count, depth, bytes, &plan) ||
      plan.total_native_bytes > budget ||
      plan.descriptor_bytes_total > SIZE_MAX ||
      plan.result_buffers_bytes_total > SIZE_MAX ||
      plan.pending_queue_bytes > SIZE_MAX ||
      plan.active_queue_bytes > SIZE_MAX ||
      plan.identity_table_bytes > SIZE_MAX ||
      plan.shared_ring_bytes > SIZE_MAX)
    return 0;
  if (active_count || instruction_nesting)
    return 0;
  for (i = 0; i < worker_count; ++i)
    if (workers[i].state != LF_FREE)
      return 0;
  oasis_lf_recording_enabled = 0;
  free_pool();
  memset(&metrics, 0, sizeof(metrics));

  record_count = (uint64_t)count * plan.record_capacity;
  if (!checked_mul(record_count, sizeof(oasis_lf_record), &record_bytes) ||
      record_bytes > SIZE_MAX)
    return 0;
  workers = (lf_worker *)calloc(count, sizeof(*workers));
  record_storage = (oasis_lf_record *)calloc(1, (size_t)record_bytes);
  pending_queue = (uint32_t *)calloc(count, sizeof(*pending_queue));
  active_queue = (uint32_t *)calloc(count, sizeof(*active_queue));
  identity_table = (lf_identity_entry *)calloc(plan.identity_capacity,
                                                sizeof(*identity_table));
  ring_storage = (lf_ring_slot *)calloc(OASIS_LF_RING_CAPACITY,
                                         sizeof(*ring_storage));
  if (!workers || !record_storage || !pending_queue || !active_queue ||
      !identity_table || !ring_storage)
  {
    free_pool();
    return 0;
  }
  worker_count = count;
  depth_limit = depth;
  memory_bytes = bytes;
  record_capacity = plan.record_capacity;
  identity_capacity = plan.identity_capacity;
  for (i = 0; i < count; ++i)
    workers[i].records = record_storage + (size_t)i * record_capacity;
  stream_sequence = 0;
  last_entry_stream_sequence = 0;
  instruction_sequence = 0;
  control_flow_sequence = 0;
  instruction_nesting = 0;
  last_started_instruction = UINT64_MAX;
  pending_head = 0;
  pending_tail = 0;
  active_head = 0;
  active_tail = 0;
  active_count = 0;
  if (++runtime_epoch == 0)
    runtime_epoch = 1;
  oasis_lf_recording_enabled = 1;
  return 1;
}

int oasis_lf_configure(uint32_t count, uint32_t depth, uint32_t bytes)
{
  oasis_lf_memory_plan plan;
  if (!oasis_lf_memory_plan_get(count, depth, bytes, &plan))
    return 0;
  return oasis_lf_configure_bounded(count, depth, bytes,
                                    plan.total_native_bytes);
}

static uint32_t hash_id(uint64_t capture_id)
{
  capture_id ^= capture_id >> 33;
  capture_id *= UINT64_C(0xff51afd7ed558ccd);
  capture_id ^= capture_id >> 33;
  capture_id *= UINT64_C(0xc4ceb9fe1a85ec53);
  capture_id ^= capture_id >> 33;
  return (uint32_t)capture_id & (identity_capacity - 1u);
}

int lf_identity_insert(uint64_t capture_id, uint32_t worker_id)
{
  uint32_t index;
  uint32_t first_tombstone = UINT32_MAX;
  uint32_t probes;
  if (!identity_table || !identity_capacity || !capture_id)
    return 0;
  index = hash_id(capture_id);
  for (probes = 0; probes < identity_capacity; ++probes)
  {
    lf_identity_entry *entry = &identity_table[index];
    if (entry->state == LF_ID_EMPTY)
    {
      if (first_tombstone != UINT32_MAX)
        entry = &identity_table[first_tombstone];
      entry->capture_id = capture_id;
      entry->worker_id = worker_id;
      entry->state = LF_ID_USED;
      return 1;
    }
    if (entry->state == LF_ID_TOMBSTONE && first_tombstone == UINT32_MAX)
      first_tombstone = index;
    else if (entry->state == LF_ID_USED && entry->capture_id == capture_id)
    {
      metrics.identity_collisions++;
      return 0;
    }
    index = (index + 1u) & (identity_capacity - 1u);
  }
  if (first_tombstone != UINT32_MAX)
  {
    lf_identity_entry *entry = &identity_table[first_tombstone];
    entry->capture_id = capture_id;
    entry->worker_id = worker_id;
    entry->state = LF_ID_USED;
    return 1;
  }
  return 0;
}

void lf_identity_remove(uint64_t capture_id)
{
  uint32_t index;
  uint32_t probes;
  if (!identity_table || !identity_capacity || !capture_id)
    return;
  index = hash_id(capture_id);
  for (probes = 0; probes < identity_capacity; ++probes)
  {
    lf_identity_entry *entry = &identity_table[index];
    if (entry->state == LF_ID_EMPTY)
      return;
    if (entry->state == LF_ID_USED && entry->capture_id == capture_id)
    {
      entry->capture_id = 0;
      entry->worker_id = 0;
      entry->state = LF_ID_TOMBSTONE;
      return;
    }
    index = (index + 1u) & (identity_capacity - 1u);
  }
}

int oasis_lf_request(uint32_t worker_id, uint64_t capture_id,
                     uint64_t generation, uint64_t run_id, uint64_t epoch)
{
  lf_worker *worker;
  if (worker_id >= worker_count || !capture_id || !generation || !run_id ||
      !epoch || epoch != runtime_epoch)
  {
    metrics.captures_dropped++;
    return 0;
  }
  worker = &workers[worker_id];
  if (worker->state != LF_FREE || generation <= worker->last_generation)
  {
    metrics.captures_dropped++;
    return 0;
  }
  if (!lf_identity_insert(capture_id, worker_id))
  {
    metrics.captures_dropped++;
    return 0;
  }
  worker->capture_id = capture_id;
  worker->generation = generation;
  worker->last_generation = generation;
  worker->run_id = run_id;
  worker->epoch = epoch;
  worker->host_audited = 0;
  worker->state = LF_PENDING;
  pending_queue[pending_tail] = worker_id;
  pending_tail = (pending_tail + 1u) % worker_count;
  metrics.pending_count++;
  metrics.occupied_count++;
  lf_update_peaks();
  return 1;
}

uint32_t oasis_lf_result_state(uint32_t worker_id)
{
  return worker_id < worker_count ? workers[worker_id].state : LF_FREE;
}

int oasis_lf_result_info(uint32_t worker_id, oasis_lf_result *result)
{
  lf_worker *worker;
  if (!result || worker_id >= worker_count)
    return 0;
  worker = &workers[worker_id];
  if (worker->state != LF_COMPLETE && worker->state != LF_ANALYZING)
    return 0;
  if (worker->state == LF_COMPLETE)
    worker->analysis_count++;
  *result = worker->result;
  lf_set_state(worker, LF_ANALYZING);
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

int oasis_lf_mark_audited(uint32_t worker_id, uint64_t generation,
                          uint32_t accepted)
{
  lf_worker *worker;
  if (worker_id >= worker_count)
    return 0;
  worker = &workers[worker_id];
  if (worker->state != LF_ANALYZING || worker->generation != generation ||
      worker->host_audited || (accepted && !worker->result.valid))
    return 0;
  worker->host_audited = 1;
  if (accepted)
    metrics.captures_validated++;
  return 1;
}

int oasis_lf_ack(uint32_t worker_id, uint64_t capture_id,
                 uint64_t generation, uint64_t run_id, uint64_t epoch)
{
  lf_worker *worker;
  if (worker_id >= worker_count)
  {
    metrics.stale_ack_count++;
    return 0;
  }
  worker = &workers[worker_id];
  if (worker->state != LF_ANALYZING || !worker->host_audited ||
      worker->capture_id != capture_id || worker->generation != generation ||
      worker->run_id != run_id || worker->epoch != epoch)
  {
    metrics.stale_ack_count++;
    return 0;
  }
  worker->release_count++;
  lf_identity_remove(worker->capture_id);
  memset(&worker->result, 0, sizeof(worker->result));
  worker->capture_id = 0;
  worker->generation = 0;
  worker->run_id = 0;
  worker->epoch = 0;
  worker->record_count = 0;
  worker->host_audited = 0;
  lf_set_state(worker, LF_FREE);
  return 1;
}
