#include "live_forward_internal.h"

int oasis_lf_metrics_get(uint64_t *output, uint32_t capacity)
{
  uint32_t i;
  uint64_t values[OASIS_LF_METRICS_COUNT];
  if (!output || capacity < OASIS_LF_METRICS_COUNT)
    return 0;
  values[0] = worker_count;
  values[1] = metrics.workers_ever_used;
  values[2] = metrics.pending_count;
  values[3] = metrics.capturing_count;
  values[4] = metrics.complete_count;
  values[5] = metrics.occupied_count;
  values[6] = metrics.peak_capturing;
  values[7] = metrics.peak_complete;
  values[8] = metrics.peak_occupied;
  values[9] = metrics.captures_started;
  values[10] = metrics.captures_completed;
  values[11] = metrics.captures_validated;
  values[12] = metrics.captures_invalid;
  values[13] = metrics.captures_dropped;
  values[14] = metrics.stale_ack_count;
  values[15] = metrics.identity_collisions;
  values[16] = metrics.ring_retention_failures;
  values[17] = metrics.memory_limit_endings;
  values[18] = metrics.depth_limit_endings;
  values[19] = metrics.unsupported_path_endings;
  values[20] = stream_sequence;
  values[21] = instruction_sequence;
  values[22] = control_flow_sequence;
  values[23] = metrics.unique_entry_stream_sequences;
  values[24] = metrics.duplicate_entry_stream_sequences;
  values[25] = metrics.total_segment_bytes;
  values[26] = metrics.shared_ring_wraps;
  values[27] = OASIS_LF_RING_CAPACITY;
  values[28] = worker_count ? sizeof(lf_worker) : 0;
  values[29] = worker_count ? memory_bytes : 0;
  for (i = 0; i < OASIS_LF_METRICS_COUNT; ++i)
    output[i] = values[i];
  return (int)OASIS_LF_METRICS_COUNT;
}

int oasis_lf_worker_lifecycle(uint32_t worker_id, uint64_t *output,
                              uint32_t capacity)
{
  lf_worker *worker;
  if (!output || capacity < OASIS_LF_LIFECYCLE_COUNT ||
      worker_id >= worker_count)
    return 0;
  worker = &workers[worker_id];
  output[0] = worker->capture_start_count;
  output[1] = worker->capture_complete_count;
  output[2] = worker->analysis_count;
  output[3] = worker->release_count;
  return (int)OASIS_LF_LIFECYCLE_COUNT;
}
