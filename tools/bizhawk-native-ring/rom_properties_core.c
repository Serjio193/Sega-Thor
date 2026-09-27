#include "rom_properties_core.h"

#include <stdlib.h>
#include <string.h>

int thor_rom_properties_init(thor_rom_property_map *map, uint32_t rom_size)
{
  uint32_t page_count;
  uint8_t *dirty;
  uint16_t *values;

  if (!map || !rom_size ||
      rom_size > UINT32_MAX - (THOR_ROM_PROPERTY_PAGE_BYTES - 1u))
    return 0;
  page_count = (rom_size + THOR_ROM_PROPERTY_PAGE_BYTES - 1u) /
    THOR_ROM_PROPERTY_PAGE_BYTES;
  dirty = (uint8_t *)calloc((page_count + 7u) / 8u, 1u);
  values = (uint16_t *)calloc(rom_size, sizeof(uint16_t));
  if (!dirty || !values)
  {
    free(dirty);
    free(values);
    return 0;
  }
  thor_rom_properties_dispose(map);
  map->values = values;
  map->dirty_pages = dirty;
  map->rom_size = rom_size;
  map->page_count = page_count;
  map->changed_bytes = 0;
  map->change_operations = 0;
  return 1;
}

void thor_rom_properties_dispose(thor_rom_property_map *map)
{
  if (!map)
    return;
  free(map->values);
  free(map->dirty_pages);
  memset(map, 0, sizeof(*map));
}

int thor_rom_properties_or_byte(thor_rom_property_map *map,
                                uint32_t offset, uint16_t properties)
{
  uint16_t previous;
  uint32_t page;

  if (!map || !map->values || offset >= map->rom_size || !properties ||
      (properties & ~THOR_ROM_PROPERTY_MASK_ALL))
    return 0;
  previous = map->values[offset];
  if ((previous | properties) == previous)
    return 1;
  map->values[offset] = (uint16_t)(previous | properties);
  page = offset / THOR_ROM_PROPERTY_PAGE_BYTES;
  map->dirty_pages[page / 8u] |= (uint8_t)(1u << (page % 8u));
  if (!previous)
    map->changed_bytes++;
  map->change_operations++;
  return 1;
}

int thor_rom_properties_or_range(thor_rom_property_map *map,
                                 uint32_t start, uint32_t length,
                                 uint16_t properties)
{
  uint32_t i;
  if (!length || start > UINT32_MAX - length || !map ||
      start + length > map->rom_size)
    return 0;
  for (i = 0; i < length; ++i)
    if (!thor_rom_properties_or_byte(map, start + i, properties))
      return 0;
  return 1;
}

int thor_rom_properties_mark_vdp_direct_move(
  thor_rom_property_map *map, uint8_t instruction_completed,
  uint8_t read_count, uint32_t read_width, uint16_t opcode,
  uint32_t source_offset, uint16_t source_value, uint16_t output_value,
  uint32_t destination_code, uint8_t dma_fill_pending)
{
  uint16_t property;
  uint32_t source_mode;
  uint32_t source_register;

  if (!map || !map->values || !instruction_completed || read_count != 1u ||
      read_width != 16u || (opcode & 0xf000u) != 0x3000u ||
      source_value != output_value || dma_fill_pending)
    return 0;

  source_mode = (opcode >> 3u) & 7u;
  source_register = opcode & 7u;
  if (source_mode < 2u ||
      (source_mode == 7u && source_register > 3u))
    return 0;

  switch (destination_code & 0x0fu)
  {
    case 0x01u: property = THOR_ROM_VDP_VRAM_SOURCE; break;
    case 0x03u: property = THOR_ROM_VDP_CRAM_SOURCE; break;
    case 0x05u: property = THOR_ROM_VDP_VSRAM_SOURCE; break;
    default: return 0;
  }

  if (source_offset > UINT32_MAX - 2u || source_offset + 2u > map->rom_size)
    return 0;
  return thor_rom_properties_or_range(map, source_offset, 2u, property);
}

int thor_rom_properties_mark_audio_payload(
  thor_rom_property_map *map, uint16_t z80_pc, uint32_t source_offset)
{
  if ((z80_pc != 0x080eu && z80_pc != 0x0855u) ||
      source_offset < 0x0bc95cu || source_offset >= 0x0bf768u)
    return 0;
  return thor_rom_properties_or_byte(map, source_offset,
                                     THOR_ROM_AUDIO_PAYLOAD_PROVEN);
}

int thor_rom_properties_mark_compressed_graphics(
  thor_rom_property_map *map, uint32_t decoder_pc, uint8_t call_completed,
  uint32_t source_offset, uint32_t source_bytes)
{
  if (decoder_pc != 0x3820u || !call_completed || !source_bytes ||
      source_bytes > 0x10000u || source_offset > UINT32_MAX - source_bytes ||
      !map || !map->values || source_offset + source_bytes > map->rom_size)
    return 0;
  return thor_rom_properties_or_range(map, source_offset, source_bytes,
    THOR_ROM_COMPRESSED_GRAPHICS_SOURCE);
}

uint32_t thor_rom_properties_copy(const thor_rom_property_map *map,
                                  uint32_t offset, uint16_t *output,
                                  uint32_t capacity)
{
  uint32_t count;
  if (!map || !map->values || !output || offset >= map->rom_size)
    return 0;
  count = map->rom_size - offset;
  if (count > capacity)
    count = capacity;
  memcpy(output, map->values + offset, count * sizeof(uint16_t));
  return count;
}

uint32_t thor_rom_properties_copy_dirty(const thor_rom_property_map *map,
                                        uint32_t first_page,
                                        uint8_t *output,
                                        uint32_t page_capacity)
{
  uint32_t count;
  uint32_t page;
  if (!map || !map->dirty_pages || !output || first_page >= map->page_count)
    return 0;
  count = map->page_count - first_page;
  if (count > page_capacity)
    count = page_capacity;
  memset(output, 0, (count + 7u) / 8u);
  for (page = 0; page < count; ++page)
  {
    uint32_t source_page = first_page + page;
    if (map->dirty_pages[source_page / 8u] &
        (uint8_t)(1u << (source_page % 8u)))
      output[page / 8u] |= (uint8_t)(1u << (page % 8u));
  }
  return count;
}

uint64_t thor_rom_properties_changed_bytes(const thor_rom_property_map *map)
{
  return map ? map->changed_bytes : 0;
}

uint64_t thor_rom_properties_change_operations(const thor_rom_property_map *map)
{
  return map ? map->change_operations : 0;
}

void thor_rom_encoding_begin(thor_rom_encoding_collector *collector,
                             uint16_t property_bit,
                             uint16_t maximum_bytes)
{
  if (!collector)
    return;
  memset(collector, 0, sizeof(*collector));
  collector->property_bit = property_bit;
  collector->maximum_bytes = maximum_bytes;
  collector->active = 1u;
  collector->valid = maximum_bytes && property_bit &&
    !(property_bit & (property_bit - 1u)) &&
    !(property_bit & ~THOR_ROM_PROPERTY_MASK_ALL);
}

void thor_rom_encoding_capture(thor_rom_encoding_collector *collector,
                               uint32_t physical_offset,
                               uint8_t fetched_byte,
                               uint8_t backing_byte,
                               uint8_t origin_proven)
{
  if (!collector || !collector->active)
    return;
  if (!collector->valid || !origin_proven || fetched_byte != backing_byte ||
      collector->byte_count >= collector->maximum_bytes ||
      (collector->byte_count &&
       collector->first_offset > UINT32_MAX - collector->byte_count) ||
      (collector->byte_count &&
       physical_offset != collector->first_offset + collector->byte_count))
  {
    collector->valid = 0u;
    return;
  }
  if (!collector->byte_count)
    collector->first_offset = physical_offset;
  collector->byte_count++;
}

int thor_rom_encoding_finish(thor_rom_property_map *map,
                             thor_rom_encoding_collector *collector,
                             uint8_t instruction_completed)
{
  int accepted = 0;
  if (collector && collector->active && collector->valid &&
      instruction_completed && collector->byte_count &&
      collector->first_offset <= UINT32_MAX - collector->byte_count && map &&
      collector->first_offset + collector->byte_count <= map->rom_size)
    accepted = thor_rom_properties_or_range(map, collector->first_offset,
      collector->byte_count, collector->property_bit);
  if (collector)
    memset(collector, 0, sizeof(*collector));
  return accepted;
}
