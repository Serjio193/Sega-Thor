#include "live_forward_internal.h"

#include <string.h>

typedef struct
{
  uint64_t sequence;
  uint32_t pc;
  uint16_t opcode;
  uint16_t ccr;
  uint32_t dreg;
  uint32_t is_control_flow;
  uint32_t fetched;
  uint16_t flow_flags;
  uint32_t exception_seen;
  uint32_t exception_async;
  uint32_t exception_source_pc;
  uint32_t exception_target_pc;
  uint16_t exception_vector;
} lf_instruction;

uint32_t instruction_nesting;
static lf_instruction instruction_stack[16];
uint64_t frame_number;
uint64_t current_master_time;
uint64_t z80_instruction_sequence;

typedef char oasis_lf_record_must_be_48_bytes[
    sizeof(oasis_lf_record) == 48u ? 1 : -1];

uint64_t lf_instruction_stack_size(void)
{
  return sizeof(instruction_stack);
}

void lf_append_record(oasis_lf_record record)
{
  uint64_t sequence = ++stream_sequence;
  uint32_t cap = oasis_lf_ring_capacity ? oasis_lf_ring_capacity
                                        : OASIS_LF_RING_CAPACITY_DEFAULT;
  lf_ring_slot *slot;
  if ((cap & (cap - 1u)) == 0)
  {
    uint32_t mask = cap - 1u;
    slot = &ring_storage[(sequence - 1u) & mask];
    if ((sequence & mask) == 0)
      metrics.shared_ring_wraps++;
  }
  else
  {
    slot = &ring_storage[(sequence - 1u) % cap];
    if (sequence % cap == 0)
      metrics.shared_ring_wraps++;
  }
  record.stream_sequence = sequence;
  slot->record = record;
  slot->valid = 1;
  slot->sequence = sequence;
}

void oasis_lf_set_master_time(uint64_t master_cycles)
{
  current_master_time = master_cycles;
}

void oasis_lf_instruction_begin(uint32_t pc,
                                const oasis_lf_cpu_state *state)
{
  lf_instruction *instruction;
  if (!oasis_lf_recording_enabled)
    return;
  lf_boundary(state);
  if (instruction_nesting >= 16u)
  {
    uint32_t offset;
    for (offset = 0; offset < active_count; ++offset)
    {
      uint32_t slot = (active_head + offset) % worker_count;
      lf_worker *worker = &workers[active_queue[slot]];
      worker->invalid = 1;
      worker->pending_end = OASIS_LF_END_UNSUPPORTED_PATH;
    }
    return;
  }
  if (instruction_nesting)
  {
    uint32_t offset;
    for (offset = 0; offset < active_count; ++offset)
    {
      uint32_t slot = (active_head + offset) % worker_count;
      lf_worker *worker = &workers[active_queue[slot]];
      worker->invalid = 1;
      worker->pending_end = OASIS_LF_END_UNSUPPORTED_PATH;
    }
  }
  instruction = &instruction_stack[instruction_nesting++];
  memset(instruction, 0, sizeof(*instruction));
  instruction->sequence = ++instruction_sequence;
  instruction->pc = pc;
  instruction->ccr = state ? (uint16_t)state->sr : 0;
}

void oasis_lf_instruction_set_opcode(uint16_t opcode,
                                     const oasis_lf_cpu_state *state)
{
  lf_instruction *instruction;
  uint16_t flags = 0;
  if (!oasis_lf_recording_enabled || !instruction_nesting)
    return;
  instruction = &instruction_stack[instruction_nesting - 1u];
  instruction->opcode = opcode;
  instruction->fetched = 1;
  instruction->ccr = state ? (uint16_t)state->sr : 0;
  instruction->dreg = state ? state->d[opcode & 7u] : 0;
  instruction->is_control_flow = oasis_lf_flow_flags(opcode,
    instruction->ccr, instruction->dreg, 0, &flags);
  instruction->flow_flags = flags;
}

static void append_bus_event(uint16_t subtype, uint32_t address,
                             uint32_t value, uint32_t width,
                             uint32_t domain)
{
  lf_instruction *instruction;
  oasis_lf_record record;
  if (!oasis_lf_recording_enabled || !instruction_nesting)
    return;
  instruction = &instruction_stack[instruction_nesting - 1u];
  memset(&record, 0, sizeof(record));
  record.instruction_sequence = instruction->sequence;
  record.master_time = current_master_time;
  record.pc = instruction->pc;
  record.address = address;
  record.value = value;
  record.kind_flags = OASIS_LF_EVENT_KIND(subtype);
  record.cpu_id = OASIS_LF_CPU_68K;
  record.length_or_width = (uint8_t)width;
  record.domain = (uint16_t)domain;
  lf_append_record(record);
}

void oasis_lf_bus_read(uint32_t address, uint32_t value, uint32_t width,
                       uint32_t domain)
{
  append_bus_event(OASIS_LF_EVENT_BUS_READ, address, value, width, domain);
}

void oasis_lf_bus_write(uint32_t address, uint32_t value, uint32_t width,
                        uint32_t domain)
{
  append_bus_event(OASIS_LF_EVENT_BUS_WRITE, address, value, width, domain);
}

void oasis_lf_frame_boundary(void)
{
  oasis_lf_record record;
  if (!oasis_lf_recording_enabled)
    return;
  memset(&record, 0, sizeof(record));
  frame_number++;
  record.master_time = current_master_time;
  record.pc = (uint32_t)frame_number;
  record.address = (uint32_t)(frame_number >> 32u);
  record.kind_flags = OASIS_LF_EVENT_KIND(OASIS_LF_EVENT_FRAME_BOUNDARY);
  record.cpu_id = OASIS_LF_CPU_NONE;
  lf_append_record(record);
}

void oasis_lf_instruction_end(uint32_t next_pc,
                              const oasis_lf_cpu_state *state,
                              uint32_t stopped)
{
  lf_instruction instruction;
  oasis_lf_record record;
  if (!oasis_lf_recording_enabled)
    return;
  if (!instruction_nesting)
  {
    uint32_t offset;
    for (offset = 0; offset < active_count; ++offset)
    {
      uint32_t slot = (active_head + offset) % worker_count;
      workers[active_queue[slot]].invalid = 1;
      workers[active_queue[slot]].pending_end = OASIS_LF_END_CAPTURE_ERROR;
    }
    lf_finish_due(state, stopped);
    return;
  }
  instruction = instruction_stack[--instruction_nesting];
  memset(&record, 0, sizeof(record));
  record.instruction_sequence = instruction.sequence;
  record.master_time = current_master_time;
  record.pc = instruction.pc;
  record.address = next_pc;
  record.value = (uint32_t)instruction.opcode;
  record.cpu_id = OASIS_LF_CPU_68K;
  record.kind_flags = OASIS_LF_INSTRUCTION;
  if (instruction.fetched)
    record.kind_flags |= OASIS_LF_FETCHED;
  if (instruction.exception_seen)
  {
    record.kind_flags |= OASIS_LF_EXCEPTION | OASIS_LF_CONTROL_FLOW;
    record.kind_flags |= instruction.exception_async ?
      OASIS_LF_ASYNCHRONOUS | OASIS_LF_COMPLETE : OASIS_LF_FAULTED;
    record.auxiliary = (instruction.exception_source_pc & 0x00ffffffu) |
      ((uint32_t)(instruction.exception_vector & 0xffu) << 24);
  }
  else
  {
    record.kind_flags |= OASIS_LF_COMPLETE | instruction.flow_flags;
    if (instruction.is_control_flow)
    {
      if ((instruction.opcode & 0xf0f8u) == 0x50c8u && state)
      {
        record.kind_flags &= ~(OASIS_LF_BRANCH_TAKEN |
                               OASIS_LF_BRANCH_NOT_TAKEN);
        oasis_lf_flow_flags(instruction.opcode, instruction.ccr,
          state->d[instruction.opcode & 7u], 1, &record.kind_flags);
      }
      record.kind_flags |= OASIS_LF_CONTROL_FLOW;
      control_flow_sequence++;
    }
  }
  if (instruction.exception_seen)
    record.kind_flags |= OASIS_LF_CONTROL_FLOW;
  lf_append_record(record);
  lf_finish_due(state, stopped);
}

void oasis_lf_exception(uint16_t vector, uint32_t source_pc,
                        uint32_t target_pc, uint32_t asynchronous,
                        const oasis_lf_cpu_state *state)
{
  if (!oasis_lf_recording_enabled)
    return;
  control_flow_sequence++;
  if (instruction_nesting)
  {
    lf_instruction *instruction = &instruction_stack[instruction_nesting - 1u];
    instruction->exception_seen = 1;
    instruction->exception_async = asynchronous;
    instruction->exception_source_pc = source_pc;
    instruction->exception_target_pc = target_pc;
    instruction->exception_vector = vector;
    if (!asynchronous)
    {
      uint32_t offset;
      for (offset = 0; offset < active_count; ++offset)
      {
        uint32_t slot = (active_head + offset) % worker_count;
        workers[active_queue[slot]].invalid = 1;
        workers[active_queue[slot]].pending_end = OASIS_LF_END_CAPTURE_ERROR;
      }
    }
    return;
  }
  {
    oasis_lf_record record;
    memset(&record, 0, sizeof(record));
    record.instruction_sequence = instruction_sequence;
    record.master_time = current_master_time;
    record.pc = source_pc;
    record.address = target_pc;
    record.value = (uint32_t)vector;
    record.cpu_id = OASIS_LF_CPU_68K;
    record.kind_flags = OASIS_LF_EXCEPTION | OASIS_LF_EXCEPTION_EVENT |
      OASIS_LF_CONTROL_FLOW |
      (asynchronous ? OASIS_LF_ASYNCHRONOUS : 0);
    record.auxiliary = (source_pc & 0x00ffffffu) |
      ((uint32_t)(vector & 0xffu) << 24);
    lf_append_record(record);
  }
  if (!asynchronous)
  {
    uint32_t offset;
    for (offset = 0; offset < active_count; ++offset)
    {
      uint32_t slot = (active_head + offset) % worker_count;
      workers[active_queue[slot]].invalid = 1;
      workers[active_queue[slot]].pending_end = OASIS_LF_END_CAPTURE_ERROR;
    }
  }
  lf_finish_due(state, 0);
  lf_boundary(state);
}

void oasis_lf_unwind(void)
{
  if (!oasis_lf_recording_enabled)
    return;
  while (instruction_nesting)
  {
    lf_instruction *instruction = &instruction_stack[--instruction_nesting];
    oasis_lf_record record;
    memset(&record, 0, sizeof(record));
    record.instruction_sequence = instruction->sequence;
    record.master_time = current_master_time;
    record.pc = instruction->pc;
    record.address = instruction->exception_target_pc;
    record.value = (uint32_t)instruction->opcode;
    record.cpu_id = OASIS_LF_CPU_68K;
    record.kind_flags = OASIS_LF_INSTRUCTION | OASIS_LF_EXCEPTION |
      OASIS_LF_CONTROL_FLOW | OASIS_LF_FAULTED;
    if (instruction->fetched)
      record.kind_flags |= OASIS_LF_FETCHED;
    record.auxiliary = (instruction->exception_source_pc & 0x00ffffffu) |
      ((uint32_t)(instruction->exception_vector & 0xffu) << 24);
    lf_append_record(record);
  }
  {
    uint32_t offset;
    for (offset = 0; offset < active_count; ++offset)
    {
      uint32_t slot = (active_head + offset) % worker_count;
      workers[active_queue[slot]].invalid = 1;
      workers[active_queue[slot]].pending_end = OASIS_LF_END_CAPTURE_ERROR;
    }
  }
}

int oasis_lf_ring_record(uint64_t sequence, oasis_lf_record *record)
{
  lf_ring_slot *slot;
  uint32_t cap = oasis_lf_ring_capacity ? oasis_lf_ring_capacity
                                        : OASIS_LF_RING_CAPACITY_DEFAULT;
  if (!oasis_lf_recording_enabled || !ring_storage || !record ||
      sequence == 0 || sequence > stream_sequence ||
      stream_sequence - sequence >= cap)
    return 0;
  if ((cap & (cap - 1u)) == 0)
    slot = &ring_storage[(sequence - 1u) & (cap - 1u)];
  else
    slot = &ring_storage[(sequence - 1u) % cap];
  if (!slot->valid || slot->sequence != sequence)
    return 0;
  *record = slot->record;
  return 1;
}

uint64_t oasis_lf_stream_sequence(void) { return stream_sequence; }
uint64_t oasis_lf_instruction_sequence(void) { return instruction_sequence; }
uint64_t oasis_lf_z80_instruction_seq(void) { return z80_instruction_sequence; }
uint64_t oasis_lf_control_flow_sequence(void) { return control_flow_sequence; }
uint64_t oasis_lf_epoch(void) { return runtime_epoch; }
uint64_t oasis_lf_current_master_time(void) { return current_master_time; }

uint64_t oasis_lf_latest_frame_boundary_record(void)
{
  uint64_t first;
  uint64_t sequence;
  oasis_lf_record record;
  uint32_t cap = oasis_lf_ring_capacity ? oasis_lf_ring_capacity
                                        : OASIS_LF_RING_CAPACITY_DEFAULT;
  if (!ring_storage || !stream_sequence)
    return 0;
  first = stream_sequence > cap ? stream_sequence - cap + 1u : 1u;
  for (sequence = stream_sequence;; --sequence)
  {
    if (oasis_lf_ring_record(sequence, &record) &&
        (record.kind_flags & OASIS_LF_EVENT) &&
        ((record.kind_flags & OASIS_LF_EVENT_SUBTYPE_MASK) >>
         OASIS_LF_EVENT_SUBTYPE_SHIFT) == OASIS_LF_EVENT_FRAME_BOUNDARY)
      return sequence;
    if (sequence == first)
      break;
  }
  return 0;
}
