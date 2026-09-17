#ifndef OASIS_LIVE_FORWARD_TRACE_H
#define OASIS_LIVE_FORWARD_TRACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OASIS_LF_RING_CAPACITY 4096u
#define OASIS_LF_MAX_WORKERS 100000u
#define OASIS_LF_METRICS_COUNT 30u
#define OASIS_LF_PLAN_COUNT 16u
#define OASIS_LF_LIFECYCLE_COUNT 4u

extern uint32_t oasis_lf_recording_enabled;

typedef struct
{
  uint32_t d[8];
  uint32_t a[8];
  uint32_t pc;
  uint32_t sr;
  uint32_t usp;
  uint32_t isp;
} oasis_lf_cpu_state;

typedef struct
{
  uint64_t stream_sequence;
  uint64_t instruction_sequence;
  uint32_t pc;
  uint32_t next_pc;
  uint16_t opcode_or_vector;
  uint16_t kind_flags;
  uint32_t auxiliary;
} oasis_lf_record;

typedef struct
{
  uint64_t capture_id;
  uint64_t generation;
  uint64_t run_id;
  uint64_t epoch;
  uint64_t entry_stream_sequence;
  uint64_t exit_stream_sequence;
  uint64_t entry_instruction_sequence;
  uint64_t exit_instruction_sequence;
  uint64_t entry_control_flow_sequence;
  uint64_t exit_control_flow_sequence;
  uint32_t worker_id;
  uint32_t termination_reason;
  uint32_t configured_depth;
  uint32_t configured_memory_bytes;
  uint32_t consumed_depth;
  uint32_t consumed_memory_bytes;
  uint32_t record_count;
  uint32_t records_bytes;
  uint32_t valid;
  oasis_lf_cpu_state entry_state;
  oasis_lf_cpu_state exit_state;
  uint64_t copy_duration_ns;
} oasis_lf_result;

typedef struct
{
  uint32_t worker_count;
  uint32_t depth;
  uint32_t memory_bytes;
  uint32_t record_capacity;
  uint32_t identity_capacity;
  uint32_t reserved;
  uint64_t descriptor_bytes_each;
  uint64_t descriptor_bytes_total;
  uint64_t result_bytes_each;
  uint64_t result_buffers_bytes_total;
  uint64_t pending_queue_bytes;
  uint64_t active_queue_bytes;
  uint64_t identity_table_bytes;
  uint64_t shared_ring_bytes;
  uint64_t instruction_stack_bytes;
  uint64_t dynamic_bytes;
  uint64_t total_native_bytes;
} oasis_lf_memory_plan;

enum oasis_lf_record_flags
{
  OASIS_LF_INSTRUCTION = 1,
  OASIS_LF_COMPLETE = 2,
  OASIS_LF_FAULTED = 4,
  OASIS_LF_CONTROL_FLOW = 8,
  OASIS_LF_BRANCH_TAKEN = 16,
  OASIS_LF_BRANCH_NOT_TAKEN = 32,
  OASIS_LF_EXCEPTION = 64,
  OASIS_LF_ASYNCHRONOUS = 128,
  OASIS_LF_EXCEPTION_EVENT = 256,
  OASIS_LF_FETCHED = 512,
  OASIS_LF_CPU_STOP_EVENT = 1024
};

enum oasis_lf_end_reason
{
  OASIS_LF_END_NONE = 0,
  OASIS_LF_END_DEPTH_LIMIT = 1,
  OASIS_LF_END_MEMORY_LIMIT = 2,
  OASIS_LF_END_RETENTION_LIMIT = 3,
  OASIS_LF_END_CPU_STOP = 4,
  OASIS_LF_END_CAPTURE_ERROR = 5,
  OASIS_LF_END_UNSUPPORTED_PATH = 6
};

int oasis_lf_configure(uint32_t worker_count, uint32_t depth,
                       uint32_t memory_bytes);
int oasis_lf_configure_bounded(uint32_t worker_count, uint32_t depth,
                               uint32_t memory_bytes,
                               uint64_t allocation_budget_bytes);
int oasis_lf_memory_plan_get(uint32_t worker_count, uint32_t depth,
                             uint32_t memory_bytes,
                             oasis_lf_memory_plan *plan);
int oasis_lf_memory_plan_values(uint32_t worker_count, uint32_t depth,
                                uint32_t memory_bytes, uint64_t *output,
                                uint32_t capacity);
int oasis_lf_metrics_get(uint64_t *output, uint32_t capacity);
int oasis_lf_worker_lifecycle(uint32_t worker_id, uint64_t *output,
                              uint32_t capacity);
int oasis_lf_mark_audited(uint32_t worker_id, uint64_t generation,
                          uint32_t accepted);
void oasis_lf_cancel_pending(void);
int oasis_lf_set_enabled(uint32_t enabled);
int oasis_lf_request(uint32_t worker_id, uint64_t capture_id,
                     uint64_t generation, uint64_t run_id, uint64_t epoch);
void oasis_lf_boundary(const oasis_lf_cpu_state *state);
void oasis_lf_instruction_begin(uint32_t pc,
                                const oasis_lf_cpu_state *state);
void oasis_lf_instruction_set_opcode(uint16_t opcode,
                                     const oasis_lf_cpu_state *state);
void oasis_lf_instruction_end(uint32_t next_pc,
                              const oasis_lf_cpu_state *state,
                              uint32_t stopped);
void oasis_lf_exception(uint16_t vector, uint32_t source_pc,
                        uint32_t target_pc, uint32_t asynchronous,
                        const oasis_lf_cpu_state *state);
void oasis_lf_cpu_stop(const oasis_lf_cpu_state *state);
void oasis_lf_unwind(void);
void oasis_lf_epoch_break(const oasis_lf_cpu_state *state);
uint32_t oasis_lf_result_state(uint32_t worker_id);
int oasis_lf_result_info(uint32_t worker_id, oasis_lf_result *result);
uint32_t oasis_lf_result_copy(uint32_t worker_id, uint64_t generation,
                              uint32_t offset, oasis_lf_record *output,
                              uint32_t capacity);
int oasis_lf_ack(uint32_t worker_id, uint64_t capture_id,
                 uint64_t generation, uint64_t run_id, uint64_t epoch);
uint64_t oasis_lf_stream_sequence(void);
uint64_t oasis_lf_instruction_sequence(void);
uint64_t oasis_lf_control_flow_sequence(void);
uint64_t oasis_lf_epoch(void);
int oasis_lf_ring_record(uint64_t sequence, oasis_lf_record *record);
uint32_t oasis_lf_flow_flags(uint16_t opcode, uint16_t ccr,
                             uint32_t dreg_value, uint32_t after_execution,
                             uint16_t *flags);

#ifdef __cplusplus
}
#endif

#endif
