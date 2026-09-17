#ifndef OASIS_LIVE_FORWARD_INTERNAL_H
#define OASIS_LIVE_FORWARD_INTERNAL_H

#include "live_forward_trace.h"

#include <stdint.h>

#define LF_FREE 0u
#define LF_PENDING 1u
#define LF_CAPTURING 2u
#define LF_COMPLETE 3u
#define LF_ANALYZING 4u

typedef struct
{
  uint32_t state;
  uint64_t capture_id;
  uint64_t generation;
  uint64_t last_generation;
  uint64_t epoch;
  uint64_t run_id;
  uint64_t first_record;
  uint64_t entry_flow;
  uint32_t configured_depth;
  uint32_t configured_memory_bytes;
  uint32_t record_count;
  uint32_t invalid;
  uint32_t pending_end;
  oasis_lf_result result;
  oasis_lf_record *records;
} lf_worker;

typedef struct
{
  uint64_t sequence;
  uint32_t valid;
  oasis_lf_record record;
} lf_ring_slot;

extern lf_worker workers[OASIS_LF_MAX_WORKERS];
extern lf_ring_slot ring_storage[OASIS_LF_RING_CAPACITY];
extern uint32_t worker_count;
extern uint32_t depth_limit;
extern uint32_t memory_bytes;
extern uint32_t record_capacity;
extern uint32_t next_worker;
extern uint32_t active_worker;
extern uint32_t instruction_nesting;
extern uint64_t stream_sequence;
extern uint64_t instruction_sequence;
extern uint64_t control_flow_sequence;
extern uint64_t runtime_epoch;
extern uint32_t oasis_lf_recording_enabled;

void lf_append_record(oasis_lf_record record);
void lf_seal_worker(uint32_t index, uint32_t reason,
                    const oasis_lf_cpu_state *exit_state);

#endif
