#include "rom_properties_runtime.h"

#include "rom_properties_core.h"
#define m68ki_cpu m68k
#define MUL (7)
#ifndef BUILD_TABLES
#include "../m68k/m68ki_cycles.h"
#endif
#include "../m68k/m68kconf.h"
#include "../m68k/m68kcpu.h"
#include "../m68k/m68kops.h"
#include "../shared.h"
#include "../z80/z80.h"

#include <string.h>

#define THOR_ROM_PREFETCH_SLOTS 16u
#define THOR_ROM_ENCODING_MAX 256u

typedef struct
{
  uint32_t address;
  uint32_t offset;
  uint64_t sequence;
  uint16_t value;
  uint8_t valid;
} thor_rom_prefetch_entry;

static thor_rom_property_map property_map;
static thor_rom_encoding_collector m68k_collector;
static thor_rom_encoding_collector z80_collector;
static thor_rom_prefetch_entry prefetch_entries[THOR_ROM_PREFETCH_SLOTS];
static uint64_t prefetch_sequence;
static uint32_t property_enabled;
static uint32_t prefetch_cursor;
static uint32_t m68k_instruction_pc;
static uint32_t gfx_source_address;
static uint8_t gfx_decoder_active;
static uint8_t gfx_source_valid;

typedef struct
{
  uint32_t source_offset;
  uint32_t read_width;
  uint16_t source_value;
  uint16_t output_value;
  uint32_t destination_code;
  uint32_t dma_fill_pending;
  uint8_t read_count;
  uint8_t source_valid;
  uint8_t vdp_write_seen;
} thor_rom_vdp_move_candidate;

static thor_rom_vdp_move_candidate vdp_move_candidate;

static int resolve_m68k_word(uint32_t address, uint16_t value,
                             uint32_t *first_offset);

int oasis_romprops_init(uint32_t rom_size)
{
  thor_rom_properties_dispose(&property_map);
  memset(prefetch_entries, 0, sizeof(prefetch_entries));
  memset(&m68k_collector, 0, sizeof(m68k_collector));
  memset(&z80_collector, 0, sizeof(z80_collector));
  memset(&vdp_move_candidate, 0, sizeof(vdp_move_candidate));
  prefetch_sequence = 0;
  prefetch_cursor = 0;
  m68k_instruction_pc = gfx_source_address = 0;
  gfx_decoder_active = gfx_source_valid = 0;
  property_enabled = 1u;
  return thor_rom_properties_init(&property_map, rom_size);
}

int oasis_romprops_clear(void)
{
  return property_map.rom_size ? oasis_romprops_init(property_map.rom_size) : 0;
}

void oasis_romprops_enable(uint32_t enabled)
{
  property_enabled = enabled != 0;
}

uint32_t oasis_romprops_size(void)
{
  return property_map.rom_size;
}

uint64_t oasis_romprops_changed_bytes(void)
{
  return thor_rom_properties_changed_bytes(&property_map);
}

uint64_t oasis_romprops_change_operations(void)
{
  return thor_rom_properties_change_operations(&property_map);
}

uint32_t oasis_romprops_copy(uint32_t offset, uint16_t *output,
                             uint32_t capacity)
{
  return thor_rom_properties_copy(&property_map, offset, output, capacity);
}

uint32_t oasis_romprops_copy_dirty(uint32_t first_page, uint8_t *output,
                                   uint32_t capacity)
{
  return thor_rom_properties_copy_dirty(&property_map, first_page, output,
                                        capacity);
}

static int resolve_m68k_byte(uint32_t address, uint32_t *offset,
                             uint8_t *backing)
{
  cpu_memory_map *map;
  uintptr_t pointer;
  uintptr_t rom_start = (uintptr_t)cart.rom;
  uintptr_t rom_end = rom_start + cart.romsize;
  uint32_t logical_offset;
  if (address > 0x00ffffffu || !offset || !backing)
    return 0;
  map = &m68ki_cpu.memory_map[(address >> 16u) & 0xffu];
  if (map->read8 || map->read16 || !map->base)
    return 0;
  logical_offset = address & 0xffffu;
#ifdef LSB_FIRST
  pointer = (uintptr_t)(map->base + (logical_offset ^ 1u));
#else
  pointer = (uintptr_t)(map->base + logical_offset);
#endif
  if (pointer < rom_start || pointer >= rom_end)
    return 0;
  *backing = *(const uint8_t *)pointer;
  *offset = (uint32_t)(pointer - rom_start);
#ifdef LSB_FIRST
  *offset ^= 1u;
#endif
  return *offset < cart.romsize;
}

void oasis_romprops_m68k_data_read(uint32_t address, uint32_t value,
                                   uint32_t width)
{
  uint32_t count;
  uint32_t first = 0;
  uint32_t i;
  if (!property_enabled || !property_map.values ||
      (width != 8u && width != 16u && width != 32u))
    return;
  if (vdp_move_candidate.read_count < 2u)
    ++vdp_move_candidate.read_count;
  vdp_move_candidate.source_valid = 0u;
  if (vdp_move_candidate.read_count == 1u && width == 16u &&
      resolve_m68k_word(address, (uint16_t)value,
                        &vdp_move_candidate.source_offset))
  {
    vdp_move_candidate.source_value = (uint16_t)value;
    vdp_move_candidate.read_width = width;
    vdp_move_candidate.source_valid = 1u;
  }
  count = width / 8u;
  for (i = 0; i < count; ++i)
  {
    uint32_t offset;
    uint32_t shift = (count - i - 1u) * 8u;
    uint8_t backing;
    if (address > UINT32_MAX - i ||
        !resolve_m68k_byte(address + i, &offset, &backing) ||
        backing != (uint8_t)(value >> shift) ||
        (i && offset != first + i))
      return;
    if (!i)
      first = offset;
  }
  (void)thor_rom_properties_or_range(&property_map, first, count,
                                     THOR_ROM_M68K_DATA_READ);
}

void oasis_romprops_z80_data_read(uint32_t address, uint8_t value)
{
  uint32_t offset;
  uint8_t backing;
  if (!property_enabled || !property_map.values ||
      !resolve_m68k_byte(address, &offset, &backing) || backing != value)
    return;
  (void)thor_rom_properties_or_range(&property_map, offset, 1u,
                                     THOR_ROM_Z80_DATA_READ);
  (void)thor_rom_properties_mark_audio_payload(&property_map,
    (uint16_t)Z80.pc.w.l, offset);
}

void oasis_romprops_vdp_dma_source(uint32_t address, uint16_t value,
                                  uint32_t destination_code)
{
  uint32_t offset;
  uint16_t property;
  if (!property_enabled || !property_map.values ||
      !resolve_m68k_word(address, value, &offset))
    return;
  switch (destination_code & 0x0fu)
  {
    case 0x01u: property = THOR_ROM_VDP_VRAM_SOURCE; break;
    case 0x03u: property = THOR_ROM_VDP_CRAM_SOURCE; break;
    case 0x05u: property = THOR_ROM_VDP_VSRAM_SOURCE; break;
    default: return;
  }
  (void)thor_rom_properties_or_range(&property_map, offset, 2u, property);
}

void oasis_romprops_vdp_cpu_source_write(uint16_t value,
                                         uint32_t destination_code,
                                         uint32_t dma_fill_pending)
{
  if (!property_enabled || !property_map.values ||
      !vdp_move_candidate.read_count || !vdp_move_candidate.source_valid ||
      vdp_move_candidate.vdp_write_seen)
  {
    if (vdp_move_candidate.read_count)
      vdp_move_candidate.source_valid = 0u;
    return;
  }
  vdp_move_candidate.output_value = value;
  vdp_move_candidate.destination_code = destination_code;
  vdp_move_candidate.dma_fill_pending = dma_fill_pending;
  vdp_move_candidate.vdp_write_seen = 1u;
}

static int resolve_m68k_word(uint32_t address, uint16_t value,
                             uint32_t *first_offset)
{
  uint32_t offset0;
  uint32_t offset1;
  uint8_t byte0;
  uint8_t byte1;
  if ((address & 1u) || !resolve_m68k_byte(address, &offset0, &byte0) ||
      !resolve_m68k_byte(address + 1u, &offset1, &byte1) ||
      offset1 != offset0 + 1u || byte0 != (uint8_t)(value >> 8u) ||
      byte1 != (uint8_t)value)
    return 0;
  *first_offset = offset0;
  return 1;
}

void oasis_romprops_m68k_begin(void)
{
  m68k_instruction_pc = (uint32_t)REG_PC;
  if (property_enabled && property_map.values && m68k_instruction_pc == 0x3820u)
  {
    gfx_decoder_active = 1u;
    gfx_source_address = (uint32_t)REG_A[0];
    gfx_source_valid = 1u;
  }
  memset(&vdp_move_candidate, 0, sizeof(vdp_move_candidate));
  if (property_enabled && property_map.values)
    thor_rom_encoding_begin(&m68k_collector,
      THOR_ROM_M68K_EXECUTED_ENCODING, THOR_ROM_ENCODING_MAX);
}

void oasis_romprops_m68k_prefetch(uint32_t address, uint16_t value)
{
  thor_rom_prefetch_entry *entry;
  uint32_t offset = 0;
  uint8_t valid;
  if (!property_enabled || !property_map.values)
    return;
  entry = &prefetch_entries[prefetch_cursor++ % THOR_ROM_PREFETCH_SLOTS];
  valid = (uint8_t)resolve_m68k_word(address, value, &offset);
  entry->address = address;
  entry->value = value;
  entry->offset = offset;
  entry->valid = valid;
  entry->sequence = ++prefetch_sequence;
}

void oasis_romprops_m68k_consume_word(uint32_t address, uint16_t value)
{
  thor_rom_prefetch_entry *best = 0;
  uint32_t i;
  if (!property_enabled || !m68k_collector.active)
    return;
  for (i = 0; i < THOR_ROM_PREFETCH_SLOTS; ++i)
  {
    thor_rom_prefetch_entry *entry = &prefetch_entries[i];
    if (entry->sequence > (best ? best->sequence : 0) &&
        entry->address == address && entry->value == value)
      best = entry;
  }
  if (!best || !best->valid)
  {
    thor_rom_encoding_capture(&m68k_collector, 0, (uint8_t)(value >> 8u),
                              0, 0);
    return;
  }
  best->sequence = 0;
  thor_rom_encoding_capture(&m68k_collector, best->offset,
                            (uint8_t)(value >> 8u),
                            (uint8_t)(value >> 8u), 1);
  thor_rom_encoding_capture(&m68k_collector, best->offset + 1u,
                            (uint8_t)value, (uint8_t)value, 1);
}

void oasis_romprops_m68k_finish(uint32_t completed)
{
  if (gfx_decoder_active)
  {
    if (!completed)
      gfx_source_valid = 0u;
    if ((uint16_t)m68k.ir == 0x4e75u)
    {
      uint32_t end = (uint32_t)REG_A[0];
      uint32_t length = end - gfx_source_address;
      uint32_t first_offset = 0;
      uint32_t i;
      uint8_t byte_value;
      if (gfx_source_valid && end > gfx_source_address && length <= 0x10000u &&
          resolve_m68k_byte(gfx_source_address, &first_offset, &byte_value))
      {
        for (i = 0; i < length; ++i)
        {
          uint32_t offset;
          if (!resolve_m68k_byte(gfx_source_address + i, &offset, &byte_value) ||
              offset != first_offset + i)
            break;
        }
        if (i == length)
          (void)thor_rom_properties_mark_compressed_graphics(&property_map,
            0x3820u, 1u, first_offset, length);
      }
      gfx_decoder_active = gfx_source_valid = 0u;
    }
  }
  if (vdp_move_candidate.vdp_write_seen)
    (void)thor_rom_properties_mark_vdp_direct_move(&property_map,
      (uint8_t)(completed != 0u), vdp_move_candidate.read_count,
      vdp_move_candidate.read_width, (uint16_t)m68k.ir,
      vdp_move_candidate.source_offset, vdp_move_candidate.source_value,
      vdp_move_candidate.output_value, vdp_move_candidate.destination_code,
      (uint8_t)(vdp_move_candidate.dma_fill_pending != 0u));
  memset(&vdp_move_candidate, 0, sizeof(vdp_move_candidate));
  (void)thor_rom_encoding_finish(&property_map, &m68k_collector,
                                 completed != 0);
}

void oasis_romprops_m68k_abort(void)
{
  oasis_romprops_m68k_finish(0);
}

void oasis_romprops_z80_begin(void)
{
  if (property_enabled && property_map.values)
    thor_rom_encoding_begin(&z80_collector,
      THOR_ROM_Z80_EXECUTED_ENCODING, THOR_ROM_ENCODING_MAX);
}

void oasis_romprops_z80_fetch(uint16_t address, const uint8_t *source,
                              uint8_t value)
{
  uintptr_t pointer = (uintptr_t)source;
  uintptr_t start = (uintptr_t)cart.rom;
  uintptr_t end = start + cart.romsize;
  uint32_t offset = 0;
  uint8_t valid = 0;
  (void)address;
  if (!property_enabled || !z80_collector.active)
    return;
  if (source && pointer >= start && pointer < end && *source == value)
  {
    offset = (uint32_t)(pointer - start);
#ifdef LSB_FIRST
    offset ^= 1u;
#endif
    valid = offset < cart.romsize;
  }
  thor_rom_encoding_capture(&z80_collector, offset, value,
    source ? *source : 0, valid);
}

void oasis_romprops_z80_finish(uint32_t completed)
{
  (void)thor_rom_encoding_finish(&property_map, &z80_collector,
                                 completed != 0);
}
