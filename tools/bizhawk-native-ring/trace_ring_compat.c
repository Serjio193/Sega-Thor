#include "trace_ring.h"
#include "live_forward_trace.h"

static uint64_t first_retained_stream(void)
{
  uint64_t latest = oasis_lf_stream_sequence();
  return latest > GPGX_TRACE_RING_CAPACITY
    ? latest - GPGX_TRACE_RING_CAPACITY + 1u
    : (latest ? 1u : 0u);
}

static uint32_t scan_instructions(uint64_t start_instruction,
                                  gpgx_trace_record *destination,
                                  uint32_t capacity,
                                  uint64_t *latest_instruction)
{
  uint64_t stream = first_retained_stream();
  uint64_t latest_stream = oasis_lf_stream_sequence();
  uint32_t count = 0;

  if (latest_instruction)
    *latest_instruction = 0;
  for (; stream && stream <= latest_stream; ++stream)
  {
    oasis_lf_record native_record;
    if (!oasis_lf_ring_record(stream, &native_record) ||
        !(native_record.kind_flags & OASIS_LF_INSTRUCTION))
      continue;
    if (latest_instruction)
      *latest_instruction = native_record.instruction_sequence;
    if (!destination)
    {
      ++count;
      continue;
    }
    if (native_record.instruction_sequence < start_instruction)
      continue;
    if (count == capacity)
      break;
    destination[count].sequence = native_record.instruction_sequence;
    destination[count].pc = native_record.pc;
    destination[count].opcode = native_record.opcode_or_vector;
    destination[count].reserved = 0;
    ++count;
  }
  return count;
}

void gpgx_trace_ring_record(uint32_t pc, uint16_t opcode)
{
  (void)pc;
  (void)opcode;
}

uint64_t gpgx_trace_ring_latest(void)
{
  uint64_t latest;
  scan_instructions(UINT64_MAX, 0, 0, &latest);
  return latest;
}

uint32_t gpgx_trace_ring_count(void)
{
  return scan_instructions(UINT64_MAX, 0, 0, 0);
}

uint32_t gpgx_trace_ring_copy(uint64_t start_sequence,
                              gpgx_trace_record *destination,
                              uint32_t capacity)
{
  if (!destination || !capacity || !start_sequence)
    return 0;
  return scan_instructions(start_sequence, destination, capacity, 0);
}

void gpgx_trace_ring_reset(void)
{
  oasis_lf_epoch_break(0);
}
