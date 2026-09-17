#ifndef OASIS_LIVE_FORWARD_INTERNAL_H
#define OASIS_LIVE_FORWARD_INTERNAL_H

#include "live_forward_trace.h"

#include <stdint.h>

#define LF_FREE 0u
#define LF_PENDING 1u
#define LF_CAPTURING 2u
#define LF_COMPLETE 3u
#define LF_ANALYZING 4u
#define LF_ID_EMPTY 0u
#define LF_ID_USED 1u
#define LF_ID_TOMBSTONE 2u

typedef struct
{
  uint32_t state;
  uint32_t ever_used;
  uint32_t host_audited;
  uint32_t invalid;
  uint32_t pending_end;
  uint32_t configured_depth;
  uint32_t configured_memory_bytes;
  uint32_t record_count;
  uint32_t reserved;
  uint64_t capture_start_count;
  uint64_t capture_complete_count;
  uint64_t analysis_count;
  uint64_t release_count;
  uint64_t capture_id;
  uint64_t generation;
  uint64_t last_generation;
  uint64_t epoch;
  uint64_t run_id;
  uint64_t first_record;
  uint64_t entry_flow;
  oasis_lf_result result;
  oasis_lf_record *records;
} lf_worker;

typedef struct
{
  uint64_t capture_id;
  uint32_t worker_id;
  uint32_t state;
} lf_identity_entry;

typedef struct
{
  uint64_t sequence;
  uint32_t valid;
  oasis_lf_record record;
} lf_ring_slot;

typedef struct
{
  uint64_t captures_started;
  uint64_t captures_completed;
  uint64_t captures_validated;
  uint64_t captures_invalid;
  uint64_t captures_dropped;
  uint64_t stale_ack_count;
  uint64_t identity_collisions;
  uint64_t ring_retention_failures;
  uint64_t memory_limit_endings;
  uint64_t depth_limit_endings;
  uint64_t unsupported_path_endings;
  uint64_t unique_entry_stream_sequences;
  uint64_t duplicate_entry_stream_sequences;
  uint64_t total_segment_bytes;
  uint64_t shared_ring_wraps;
  uint32_t workers_ever_used;
  uint32_t pending_count;
  uint32_t capturing_count;
  uint32_t complete_count;
  uint32_t occupied_count;
  uint32_t peak_capturing;
  uint32_t peak_complete;
  uint32_t peak_occupied;
} lf_metrics;

extern lf_worker *workers;
extern oasis_lf_record *record_storage;
extern uint32_t *pending_queue;
extern uint32_t *active_queue;
extern lf_identity_entry *identity_table;
extern lf_ring_slot *ring_storage;
extern uint32_t worker_count;
extern uint32_t depth_limit;
extern uint32_t memory_bytes;
extern uint32_t record_capacity;
extern uint32_t identity_capacity;
extern uint32_t pending_head;
extern uint32_t pending_tail;
extern uint32_t active_head;
extern uint32_t active_tail;
extern uint32_t active_count;
extern uint64_t last_started_instruction;
extern uint32_t instruction_nesting;
extern uint64_t stream_sequence;
extern uint64_t last_entry_stream_sequence;
extern uint64_t instruction_sequence;
extern uint64_t control_flow_sequence;
extern uint64_t runtime_epoch;
extern uint32_t oasis_lf_recording_enabled;
extern lf_metrics metrics;

void lf_append_record(oasis_lf_record record);
void lf_boundary(const oasis_lf_cpu_state *state);
void lf_finish_due(const oasis_lf_cpu_state *state, uint32_t stopped);
void lf_seal_worker(uint32_t index, uint32_t reason,
                    const oasis_lf_cpu_state *exit_state);
void lf_cancel_pending(void);
void lf_cancel_all_active(uint32_t reason,
                          const oasis_lf_cpu_state *exit_state);
void lf_update_peaks(void);
void lf_set_state(lf_worker *worker, uint32_t state);
int lf_identity_insert(uint64_t capture_id, uint32_t worker_id);
void lf_identity_remove(uint64_t capture_id);
uint64_t lf_instruction_stack_size(void);

#endif
