#include "live_forward_trace.h"

#include <cassert>

static oasis_lf_cpu_state state(uint32_t pc)
{
  oasis_lf_cpu_state value{};
  value.pc = pc;
  return value;
}

int main()
{
  assert(oasis_lf_configure(1, 20, 64u * 1024u));
  oasis_lf_cpu_state before = state(0x120u);
  oasis_lf_cpu_state after = before;
  oasis_lf_instruction_begin(before.pc, &before);
  oasis_lf_instruction_set_opcode(0x4e71u, &before);
  oasis_lf_bus_read(0x00f00010u, 0x89abcdefu, 32u, OASIS_LF_BUS_ROM);
  oasis_lf_bus_write(0x00ff0010u, 0x5au, 8u, OASIS_LF_BUS_68K_RAM);
  after.pc = 0x122u;
  oasis_lf_instruction_end(after.pc, &after, 0);
  oasis_lf_frame_boundary();

  oasis_lf_record records[4]{};
  for (uint64_t sequence = 1; sequence <= 4; ++sequence)
  {
    assert(oasis_lf_ring_record(sequence, &records[sequence - 1]));
    assert(records[sequence - 1].stream_sequence == sequence);
  }
  /* Bus read event. */
  assert((records[0].kind_flags & OASIS_LF_EVENT) != 0);
  assert(((records[0].kind_flags & OASIS_LF_EVENT_SUBTYPE_MASK) >>
          OASIS_LF_EVENT_SUBTYPE_SHIFT) == OASIS_LF_EVENT_BUS_READ);
  assert(records[0].instruction_sequence == 1 && records[0].pc == 0x120u);
  assert(records[0].address == 0x00f00010u);
  assert(records[0].value == 0x89abcdefu);
  assert(records[0].cpu_id == OASIS_LF_CPU_68K);
  assert(records[0].length_or_width == 32);
  assert(records[0].domain == OASIS_LF_BUS_ROM);
  /* Bus write event. */
  assert(((records[1].kind_flags & OASIS_LF_EVENT_SUBTYPE_MASK) ==
         (OASIS_LF_EVENT_BUS_WRITE << OASIS_LF_EVENT_SUBTYPE_SHIFT)));
  assert(records[1].instruction_sequence == records[0].instruction_sequence);
  assert(records[1].value == 0x5au);
  assert(records[1].length_or_width == 8);
  assert(records[1].domain == OASIS_LF_BUS_68K_RAM);
  /* Instruction record. */
  assert((records[2].kind_flags & OASIS_LF_INSTRUCTION) != 0);
  assert(records[2].instruction_sequence == 1);
  assert(records[2].value == 0x4e71u);
  assert(records[2].cpu_id == OASIS_LF_CPU_68K);
  /* Frame boundary. */
  assert((records[3].kind_flags & OASIS_LF_EVENT_SUBTYPE_MASK) ==
         (OASIS_LF_EVENT_FRAME_BOUNDARY << OASIS_LF_EVENT_SUBTYPE_SHIFT));
  assert(records[3].instruction_sequence == 0 && records[3].pc == 1u);
  assert(records[3].cpu_id == OASIS_LF_CPU_NONE);
}
