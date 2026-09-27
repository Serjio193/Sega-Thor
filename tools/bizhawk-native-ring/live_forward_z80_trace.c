/* Z80 instruction and bus event capture for the live forward evidence
 * stream.  Z80 records use the same 48-byte oasis_lf_record as M68K
 * but with cpu_id = OASIS_LF_CPU_Z80 and an independent monotonic
 * z80_instruction_sequence counter.
 *
 * Z80 instruction bytes (up to 4 for DD CB d op) are stored in the
 * value field, little-endian packed.  length_or_width carries the
 * exact instruction byte count.
 *
 * Banked ROM reads carry both the logical Z80 address (in address)
 * and the resolved physical ROM address (in auxiliary). */

#include "live_forward_internal.h"

#include <string.h>

void oasis_lf_z80_instruction(uint16_t pc, uint16_t next_pc,
                              const uint8_t *bytes, uint8_t length)
{
  oasis_lf_record record;
  uint32_t packed_bytes = 0;
  uint8_t i;
  if (!oasis_lf_recording_enabled)
    return;
  if (length > 4u)
    length = 4u;
  for (i = 0; i < length; ++i)
    packed_bytes |= (uint32_t)bytes[i] << (i * 8u);
  memset(&record, 0, sizeof(record));
  record.instruction_sequence = ++z80_instruction_sequence;
  record.master_time = current_master_time;
  record.pc = (uint32_t)pc;
  record.address = (uint32_t)next_pc;
  record.value = packed_bytes;
  record.kind_flags = OASIS_LF_INSTRUCTION | OASIS_LF_COMPLETE |
    OASIS_LF_FETCHED;
  record.cpu_id = OASIS_LF_CPU_Z80;
  record.length_or_width = length;
  lf_append_record(record);
}

static void z80_bus_event(uint16_t subtype, uint16_t pc,
                          uint16_t z80_address, uint32_t value,
                          uint8_t width, uint16_t domain,
                          uint32_t resolved_address)
{
  oasis_lf_record record;
  if (!oasis_lf_recording_enabled)
    return;
  memset(&record, 0, sizeof(record));
  record.instruction_sequence = z80_instruction_sequence;
  record.master_time = current_master_time;
  record.pc = (uint32_t)pc;
  record.address = (uint32_t)z80_address;
  record.value = value;
  record.kind_flags = OASIS_LF_EVENT_KIND(subtype);
  record.cpu_id = OASIS_LF_CPU_Z80;
  record.length_or_width = width;
  record.domain = domain;
  record.auxiliary = resolved_address;
  lf_append_record(record);
}

void oasis_lf_z80_bus_read(uint16_t pc, uint16_t z80_address,
                           uint32_t value, uint8_t width,
                           uint16_t domain, uint32_t resolved_address)
{
  z80_bus_event(OASIS_LF_EVENT_BUS_READ, pc, z80_address, value,
                width, domain, resolved_address);
}

void oasis_lf_z80_bus_write(uint16_t pc, uint16_t z80_address,
                            uint32_t value, uint8_t width,
                            uint16_t domain, uint32_t resolved_address)
{
  z80_bus_event(OASIS_LF_EVENT_BUS_WRITE, pc, z80_address, value,
                width, domain, resolved_address);
}

void oasis_lf_z80_bank_register_change(uint16_t pc, uint32_t bank_value,
                                       uint32_t physical_base)
{
  oasis_lf_record record;
  if (!oasis_lf_recording_enabled)
    return;
  memset(&record, 0, sizeof(record));
  record.instruction_sequence = z80_instruction_sequence;
  record.master_time = current_master_time;
  record.pc = (uint32_t)pc;
  record.value = bank_value;
  record.kind_flags = OASIS_LF_EVENT_KIND(OASIS_LF_EVENT_BANK_REGISTER_CHANGE);
  record.cpu_id = OASIS_LF_CPU_Z80;
  record.auxiliary = physical_base;
  lf_append_record(record);
}
