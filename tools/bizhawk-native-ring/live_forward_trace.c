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

typedef char oasis_lf_record_must_be_32_bytes[
    sizeof(oasis_lf_record) == 32u ? 1 : -1];

uint64_t lf_instruction_stack_size(void)
{
  return sizeof(instruction_stack);
}

void lf_append_record(oasis_lf_record record)
{
  uint64_t sequence = ++stream_sequence;
  lf_ring_slot *slot = &ring_storage[(sequence - 1u) % OASIS_LF_RING_CAPACITY];
  record.stream_sequence = sequence;
  slot->record = record;
  slot->valid = 1;
  slot->sequence = sequence;
  if (sequence % OASIS_LF_RING_CAPACITY == 0)
    metrics.shared_ring_wraps++;
}

static void flag_all_active(uint32_t reason)
{
  uint32_t offset;
  for (offset = 0; offset < active_count; ++offset)
  {
    uint32_t slot = (active_head + offset) % worker_count;
    lf_worker *worker = &workers[active_queue[slot]];
    worker->invalid = 1;
    worker->pending_end = reason;
  }
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
    flag_all_active(OASIS_LF_END_UNSUPPORTED_PATH);
    return;
  }
  if (instruction_nesting)
    flag_all_active(OASIS_LF_END_UNSUPPORTED_PATH);
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
    flag_all_active(OASIS_LF_END_CAPTURE_ERROR);
    lf_finish_due(state, stopped);
    return;
  }
  instruction = instruction_stack[--instruction_nesting];
  memset(&record, 0, sizeof(record));
  record.instruction_sequence = instruction.sequence;
  record.pc = instruction.pc;
  record.next_pc = next_pc;
  record.opcode_or_vector = instruction.opcode;
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
      flag_all_active(OASIS_LF_END_CAPTURE_ERROR);
    return;
  }
  {
    oasis_lf_record record;
    memset(&record, 0, sizeof(record));
    record.instruction_sequence = instruction_sequence;
    record.pc = source_pc;
    record.next_pc = target_pc;
    record.opcode_or_vector = vector;
    record.kind_flags = OASIS_LF_EXCEPTION | OASIS_LF_EXCEPTION_EVENT |
      OASIS_LF_CONTROL_FLOW |
      (asynchronous ? OASIS_LF_ASYNCHRONOUS : 0);
    record.auxiliary = (source_pc & 0x00ffffffu) |
      ((uint32_t)(vector & 0xffu) << 24);
    lf_append_record(record);
  }
  if (!asynchronous)
    flag_all_active(OASIS_LF_END_CAPTURE_ERROR);
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
    record.pc = instruction->pc;
    record.next_pc = instruction->exception_target_pc;
    record.opcode_or_vector = instruction->opcode;
    record.kind_flags = OASIS_LF_INSTRUCTION | OASIS_LF_EXCEPTION |
      OASIS_LF_CONTROL_FLOW | OASIS_LF_FAULTED;
    if (instruction->fetched)
      record.kind_flags |= OASIS_LF_FETCHED;
    record.auxiliary = (instruction->exception_source_pc & 0x00ffffffu) |
      ((uint32_t)(instruction->exception_vector & 0xffu) << 24);
    lf_append_record(record);
  }
  flag_all_active(OASIS_LF_END_CAPTURE_ERROR);
}

int oasis_lf_ring_record(uint64_t sequence, oasis_lf_record *record)
{
  lf_ring_slot *slot;
  if (!oasis_lf_recording_enabled || !ring_storage || !record ||
      sequence == 0 || sequence > stream_sequence ||
      stream_sequence - sequence >= OASIS_LF_RING_CAPACITY)
    return 0;
  slot = &ring_storage[(sequence - 1u) % OASIS_LF_RING_CAPACITY];
  if (!slot->valid || slot->sequence != sequence)
    return 0;
  *record = slot->record;
  return 1;
}

uint64_t oasis_lf_stream_sequence(void) { return stream_sequence; }
uint64_t oasis_lf_instruction_sequence(void) { return instruction_sequence; }
uint64_t oasis_lf_control_flow_sequence(void) { return control_flow_sequence; }
uint64_t oasis_lf_epoch(void) { return runtime_epoch; }
