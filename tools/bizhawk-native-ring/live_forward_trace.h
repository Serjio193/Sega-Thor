#ifndef OASIS_LIVE_FORWARD_TRACE_H
#define OASIS_LIVE_FORWARD_TRACE_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define OASIS_LF_RING_CAPACITY_DEFAULT 2097152u
#define OASIS_LF_MAX_WORKERS 100000u
#define OASIS_LF_METRICS_COUNT 32u
#define OASIS_LF_PLAN_COUNT 16u
#define OASIS_LF_LIFECYCLE_COUNT 4u
#define OASIS_LF_WORKER_STATUS_COUNT 7u
#define OASIS_LF_RECORD_VERSION 2u

extern uint32_t oasis_lf_recording_enabled;
extern uint32_t oasis_lf_ring_capacity;
int oasis_lf_set_ring_capacity(uint32_t capacity);

typedef struct
{
  uint32_t d[8];
  uint32_t a[8];
  uint32_t pc;
  uint32_t sr;
  uint32_t usp;
  uint32_t isp;
} oasis_lf_cpu_state;

/* V2 record: 48 bytes, explicit fields, lossless.
 *
 * Instruction records:
 *   stream_sequence  = global capture append order
 *   instruction_sequence = CPU-local monotonic counter
 *   master_time      = common emulated master cycles (frame-relative)
 *   pc               = CPU program counter (24-bit M68K, 16-bit Z80)
 *   address          = next_pc after instruction
 *   value            = M68K opcode (low 16); Z80 raw instruction bytes
 *   kind_flags       = instruction + control flow flags
 *   cpu_id           = OASIS_LF_CPU_68K or OASIS_LF_CPU_Z80
 *   length_or_width  = instruction byte count (Z80: 1-4)
 *   domain           = 0 for instructions
 *   auxiliary        = exception info (M68K exceptions) or 0
 *
 * Bus event records:
 *   pc               = causing instruction PC
 *   address          = bus logical address
 *   value            = full 32-bit bus value
 *   length_or_width  = bus width (8, 16, 32)
 *   domain           = bus domain enum
 *   auxiliary        = resolved physical address (Z80 banked ROM) or 0
 *
 * Frame boundary records:
 *   pc               = frame_number low 32 bits
 *   address          = frame_number high 32 bits
 *   kind_flags       = OASIS_LF_EVENT | FRAME_BOUNDARY subtype
 *
 * Bank register change records:
 *   pc               = CPU PC at time of change
 *   address          = bank register address
 *   value            = new bank value
 *   auxiliary        = resulting physical base address
 */
typedef struct
{
  uint64_t stream_sequence;
  uint64_t instruction_sequence;
  uint64_t master_time;
  uint32_t pc;
  uint32_t address;
  uint32_t value;
  uint16_t kind_flags;
  uint8_t  cpu_id;
  uint8_t  length_or_width;
  uint16_t domain;
  uint16_t reserved;
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

/* Event records set OASIS_LF_EVENT and encode their subtype in bits 11..13. */
#define OASIS_LF_EVENT 0x8000u
#define OASIS_LF_EVENT_SUBTYPE_SHIFT 11u
#define OASIS_LF_EVENT_SUBTYPE_MASK 0x3800u
#define OASIS_LF_EVENT_KIND(subtype) \
  ((uint16_t)(OASIS_LF_EVENT | \
    (((uint16_t)(subtype) << OASIS_LF_EVENT_SUBTYPE_SHIFT) & \
     OASIS_LF_EVENT_SUBTYPE_MASK)))

enum oasis_lf_event_subtype
{
  OASIS_LF_EVENT_NONE = 0,
  OASIS_LF_EVENT_BUS_READ = 1,
  OASIS_LF_EVENT_BUS_WRITE = 2,
  OASIS_LF_EVENT_FRAME_BOUNDARY = 3,
  OASIS_LF_EVENT_BANK_REGISTER_CHANGE = 4
};

enum oasis_lf_bus_domain
{
  OASIS_LF_BUS_ROM = 0,
  OASIS_LF_BUS_68K_RAM = 1,
  OASIS_LF_BUS_Z80_WINDOW = 2,
  OASIS_LF_BUS_VDP = 3,
  OASIS_LF_BUS_YM2612 = 4,
  OASIS_LF_BUS_PSG = 5,
  OASIS_LF_BUS_OTHER = 6,
  OASIS_LF_BUS_Z80_RAM = 7,
  OASIS_LF_BUS_BANKED_ROM = 8
};

enum oasis_lf_bus_cpu
{
  OASIS_LF_CPU_68K = 0,
  OASIS_LF_CPU_Z80 = 1,
  OASIS_LF_CPU_NONE = 0xFF
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
int oasis_lf_worker_status_get(uint32_t worker_id, uint64_t *output,
                               uint32_t capacity);
int oasis_lf_mark_audited(uint32_t worker_id, uint64_t generation,
                          uint32_t accepted);
void oasis_lf_cancel_pending(void);
int oasis_lf_set_enabled(uint32_t enabled);
int oasis_lf_request(uint32_t worker_id, uint64_t capture_id,
                     uint64_t generation, uint64_t run_id, uint64_t epoch);

/* M68K hooks. */
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
void oasis_lf_bus_read(uint32_t address, uint32_t value, uint32_t width,
                       uint32_t domain);
void oasis_lf_bus_write(uint32_t address, uint32_t value, uint32_t width,
                        uint32_t domain);
void oasis_lf_frame_boundary(void);
void oasis_lf_set_master_time(uint64_t master_cycles);

/* Z80 hooks. */
void oasis_lf_z80_instruction(uint16_t pc, uint16_t next_pc,
                              const uint8_t *bytes, uint8_t length);
void oasis_lf_z80_bus_read(uint16_t pc, uint16_t z80_address,
                           uint32_t value, uint8_t width,
                           uint16_t domain, uint32_t resolved_address);
void oasis_lf_z80_bus_write(uint16_t pc, uint16_t z80_address,
                            uint32_t value, uint8_t width,
                            uint16_t domain, uint32_t resolved_address);
void oasis_lf_z80_bank_register_change(uint16_t pc, uint32_t bank_value,
                                       uint32_t physical_base);

/* Query. */
uint16_t oasis_lf_resolve_domain(const void *memory_map, uint32_t address);
uint32_t oasis_lf_result_state(uint32_t worker_id);
int oasis_lf_result_info(uint32_t worker_id, oasis_lf_result *result);
uint32_t oasis_lf_result_copy(uint32_t worker_id, uint64_t generation,
                              uint32_t offset, oasis_lf_record *output,
                              uint32_t capacity);
int oasis_lf_ack(uint32_t worker_id, uint64_t capture_id,
                 uint64_t generation, uint64_t run_id, uint64_t epoch);
uint64_t oasis_lf_stream_sequence(void);
uint64_t oasis_lf_instruction_sequence(void);
uint64_t oasis_lf_z80_instruction_seq(void);
uint64_t oasis_lf_control_flow_sequence(void);
uint64_t oasis_lf_epoch(void);
uint64_t oasis_lf_current_master_time(void);
int oasis_lf_ring_record(uint64_t sequence, oasis_lf_record *record);
uint32_t oasis_lf_flow_flags(uint16_t opcode, uint16_t ccr,
                             uint32_t dreg_value, uint32_t after_execution,
                             uint16_t *flags);

#ifdef __cplusplus
}
#endif

#endif
uint64_t oasis_lf_latest_frame_boundary_record(void);
