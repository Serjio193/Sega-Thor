#ifndef THOR_ROM_PROPERTIES_CORE_H
#define THOR_ROM_PROPERTIES_CORE_H

#include <stdint.h>

#define THOR_ROM_PROPERTY_PAGE_BYTES 4096u
#define THOR_ROM_PROPERTY_MASK_ALL 0x01ffu

enum thor_rom_property_bit
{
  THOR_ROM_M68K_EXECUTED_ENCODING = 1u << 0,
  THOR_ROM_Z80_EXECUTED_ENCODING = 1u << 1,
  THOR_ROM_M68K_DATA_READ = 1u << 2,
  THOR_ROM_Z80_DATA_READ = 1u << 3,
  THOR_ROM_VDP_VRAM_SOURCE = 1u << 4,
  THOR_ROM_VDP_CRAM_SOURCE = 1u << 5,
  THOR_ROM_VDP_VSRAM_SOURCE = 1u << 6,
  THOR_ROM_AUDIO_PAYLOAD_PROVEN = 1u << 7,
  THOR_ROM_COMPRESSED_GRAPHICS_SOURCE = 1u << 8
};

typedef struct
{
  uint16_t *values;
  uint8_t *dirty_pages;
  uint32_t rom_size;
  uint32_t page_count;
  uint64_t changed_bytes;
  uint64_t change_operations;
} thor_rom_property_map;

typedef struct
{
  uint32_t first_offset;
  uint32_t byte_count;
  uint16_t property_bit;
  uint16_t maximum_bytes;
  uint8_t active;
  uint8_t valid;
} thor_rom_encoding_collector;

/* Map must be zero-initialized or disposed before initialization. */
int thor_rom_properties_init(thor_rom_property_map *map, uint32_t rom_size);
void thor_rom_properties_dispose(thor_rom_property_map *map);
int thor_rom_properties_or_byte(thor_rom_property_map *map,
                                uint32_t offset, uint16_t properties);
int thor_rom_properties_or_range(thor_rom_property_map *map,
                                 uint32_t start, uint32_t length,
                                 uint16_t properties);
int thor_rom_properties_mark_vdp_direct_move(
  thor_rom_property_map *map, uint8_t instruction_completed,
  uint8_t read_count, uint32_t read_width, uint16_t opcode,
  uint32_t source_offset, uint16_t source_value, uint16_t output_value,
  uint32_t destination_code, uint8_t dma_fill_pending);
int thor_rom_properties_mark_audio_payload(
  thor_rom_property_map *map, uint16_t z80_pc, uint32_t source_offset);
int thor_rom_properties_mark_compressed_graphics(
  thor_rom_property_map *map, uint32_t decoder_pc, uint8_t call_completed,
  uint32_t source_offset, uint32_t source_bytes);
uint32_t thor_rom_properties_copy(const thor_rom_property_map *map,
                                  uint32_t offset, uint16_t *output,
                                  uint32_t capacity);
uint32_t thor_rom_properties_copy_dirty(const thor_rom_property_map *map,
                                        uint32_t first_page,
                                        uint8_t *output,
                                        uint32_t page_capacity);
uint64_t thor_rom_properties_changed_bytes(const thor_rom_property_map *map);
uint64_t thor_rom_properties_change_operations(const thor_rom_property_map *map);
void thor_rom_encoding_begin(thor_rom_encoding_collector *collector,
                             uint16_t property_bit,
                             uint16_t maximum_bytes);
void thor_rom_encoding_capture(thor_rom_encoding_collector *collector,
                               uint32_t physical_offset,
                               uint8_t fetched_byte,
                               uint8_t backing_byte,
                               uint8_t origin_proven);
int thor_rom_encoding_finish(thor_rom_property_map *map,
                             thor_rom_encoding_collector *collector,
                             uint8_t instruction_completed);

#endif
