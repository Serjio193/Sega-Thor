#include "../tools/bizhawk-native-ring/rom_properties_core.h"

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define CHECK(condition) do { \
  if (!(condition)) { \
    fprintf(stderr, "check failed at line %d: %s\n", __LINE__, #condition); \
    return 1; \
  } \
} while (0)

int main(void)
{
  thor_rom_property_map map;
  thor_rom_property_map audio_map;
  thor_rom_property_map graphics_map;
  thor_rom_encoding_collector collector;
  uint16_t copy[8];
  uint16_t low[4];
  uint8_t dirty[2] = {0, 0};
  memset(&map, 0, sizeof(map));
  memset(&audio_map, 0, sizeof(audio_map));
  memset(&graphics_map, 0, sizeof(graphics_map));
  CHECK(thor_rom_properties_init(&map, 8193u));
  CHECK(thor_rom_properties_changed_bytes(&map) == 0u);
  CHECK(thor_rom_properties_or_byte(&map, 4095u, 1u));
  CHECK(thor_rom_properties_or_byte(&map, 4096u, 2u));
  CHECK(thor_rom_properties_or_byte(&map, 4096u, 2u));
  CHECK(thor_rom_properties_or_byte(&map, 4096u, 4u));
  CHECK(thor_rom_properties_or_range(&map, 2u, 2u, 4u));
  CHECK(!thor_rom_properties_or_byte(&map, 8193u, 1u));
  CHECK(!thor_rom_properties_or_byte(&map, 8u, 0x200u));
  CHECK(thor_rom_properties_or_byte(&map, 8u,
    THOR_ROM_COMPRESSED_GRAPHICS_SOURCE));
  CHECK(!thor_rom_properties_or_range(&map, UINT32_MAX, 2u, 1u));
  CHECK(thor_rom_properties_changed_bytes(&map) == 5u);
  CHECK(thor_rom_properties_change_operations(&map) == 6u);
  CHECK(thor_rom_properties_copy(&map, 4094u, copy, 8u) == 8u);
  CHECK(copy[1] == 1u && copy[2] == 6u && copy[4] == 0u);
  CHECK(thor_rom_properties_copy(&map, 0u, low, 4u) == 4u);
  CHECK(low[2] == 4u && low[3] == 4u);
  thor_rom_encoding_begin(&collector, 2u, 8u);
  thor_rom_encoding_capture(&collector, 20u, 0x12u, 0x12u, 1u);
  thor_rom_encoding_capture(&collector, 21u, 0x34u, 0x34u, 1u);
  CHECK(thor_rom_encoding_finish(&map, &collector, 1u));
  CHECK(thor_rom_properties_copy(&map, 20u, low, 4u) == 4u);
  CHECK(low[0] == 2u && low[1] == 2u);
  thor_rom_encoding_begin(&collector, 2u, 8u);
  thor_rom_encoding_capture(&collector, 30u, 0x12u, 0x12u, 1u);
  CHECK(!thor_rom_encoding_finish(&map, &collector, 0u));
  thor_rom_encoding_begin(&collector, 2u, 8u);
  thor_rom_encoding_capture(&collector, 40u, 0x12u, 0x12u, 1u);
  thor_rom_encoding_capture(&collector, 42u, 0x34u, 0x34u, 1u);
  CHECK(!thor_rom_encoding_finish(&map, &collector, 1u));
  thor_rom_encoding_begin(&collector, 2u, 2u);
  thor_rom_encoding_capture(&collector, 50u, 0x12u, 0x12u, 1u);
  thor_rom_encoding_capture(&collector, 51u, 0x34u, 0x35u, 1u);
  CHECK(!thor_rom_encoding_finish(&map, &collector, 1u));
  CHECK(thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 16u,
    0x3018u, 100u, 0x1234u, 0x1234u, 0x01u, 0u));
  CHECK(thor_rom_properties_copy(&map, 100u, low, 2u) == 2u);
  CHECK(low[0] & THOR_ROM_VDP_VRAM_SOURCE);
  CHECK(thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 16u,
    0x3018u, 104u, 0x1234u, 0x1234u, 0x03u, 0u));
  CHECK(thor_rom_properties_copy(&map, 104u, low, 2u) == 2u);
  CHECK(low[0] & THOR_ROM_VDP_CRAM_SOURCE);
  CHECK(thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 16u,
    0x3018u, 108u, 0x1234u, 0x1234u, 0x05u, 0u));
  CHECK(thor_rom_properties_copy(&map, 108u, low, 2u) == 2u);
  CHECK(low[0] & THOR_ROM_VDP_VSRAM_SOURCE);
  CHECK(!thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 16u,
    0xD018u, 112u, 0x1234u, 0x1234u, 0x01u, 0u));
  CHECK(!thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 16u,
    0x3000u, 112u, 0x1234u, 0x1234u, 0x01u, 0u));
  CHECK(!thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 16u,
    0x3008u, 112u, 0x1234u, 0x1234u, 0x01u, 0u));
  CHECK(!thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 16u,
    0x303Cu, 112u, 0x1234u, 0x1234u, 0x01u, 0u));
  CHECK(!thor_rom_properties_mark_vdp_direct_move(&map, 0u, 1u, 16u,
    0x3018u, 112u, 0x1234u, 0x1234u, 0x01u, 0u));
  CHECK(!thor_rom_properties_mark_vdp_direct_move(&map, 1u, 2u, 16u,
    0x3018u, 112u, 0x1234u, 0x1234u, 0x01u, 0u));
  CHECK(!thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 8u,
    0x3018u, 112u, 0x1234u, 0x1234u, 0x01u, 0u));
  CHECK(!thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 16u,
    0x3018u, 112u, 0x1234u, 0x5678u, 0x01u, 0u));
  CHECK(!thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 16u,
    0x3018u, 112u, 0x1234u, 0x1234u, 0x01u, 1u));
  CHECK(!thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 16u,
    0x3018u, 8192u, 0x1234u, 0x1234u, 0x01u, 0u));
  CHECK(!thor_rom_properties_mark_vdp_direct_move(&map, 1u, 1u, 16u,
    0x3018u, UINT32_MAX - 1u, 0x1234u, 0x1234u, 0x01u, 0u));
  CHECK(thor_rom_properties_init(&audio_map, 0x0bf768u));
  CHECK(thor_rom_properties_mark_audio_payload(&audio_map, 0x080eu,
    0x0bc95cu));
  CHECK(thor_rom_properties_mark_audio_payload(&audio_map, 0x0855u,
    0x0bf767u));
  CHECK(thor_rom_properties_copy(&audio_map, 0x0bc95cu, low, 1u) == 1u);
  CHECK(low[0] == THOR_ROM_AUDIO_PAYLOAD_PROVEN);
  CHECK(thor_rom_properties_copy(&audio_map, 0x0bf767u, low, 1u) == 1u);
  CHECK(low[0] == THOR_ROM_AUDIO_PAYLOAD_PROVEN);
  CHECK(!thor_rom_properties_mark_audio_payload(&audio_map, 0x080eu,
    0x0bc95bu));
  CHECK(!thor_rom_properties_mark_audio_payload(&audio_map, 0x0855u,
    0x0bf768u));
  CHECK(!thor_rom_properties_mark_audio_payload(&audio_map, 0x0854u,
    0x0beDE4u));
  CHECK(thor_rom_properties_init(&graphics_map, 0x20000u));
  CHECK(thor_rom_properties_mark_compressed_graphics(&graphics_map,
    0x3820u, 1u, 0x1000u, 0x200u));
  CHECK(thor_rom_properties_copy(&graphics_map, 0x1000u, low, 2u) == 2u);
  CHECK(low[0] == THOR_ROM_COMPRESSED_GRAPHICS_SOURCE);
  CHECK(!thor_rom_properties_mark_compressed_graphics(&graphics_map,
    0x3822u, 1u, 0x2000u, 16u));
  CHECK(!thor_rom_properties_mark_compressed_graphics(&graphics_map,
    0x3820u, 0u, 0x2000u, 16u));
  CHECK(!thor_rom_properties_mark_compressed_graphics(&graphics_map,
    0x3820u, 1u, 0x2000u, 0u));
  CHECK(!thor_rom_properties_mark_compressed_graphics(&graphics_map,
    0x3820u, 1u, 0x2000u, 0x10001u));
  CHECK(!thor_rom_properties_mark_compressed_graphics(&graphics_map,
    0x3820u, 1u, 0x1fff0u, 32u));
  CHECK(thor_rom_properties_copy_dirty(&map, 0u, dirty, 3u) == 3u);
  CHECK((dirty[0] & 3u) == 3u);
  CHECK((dirty[1] & 1u) == 0u);
  memset(dirty, 0, sizeof(dirty));
  CHECK(thor_rom_properties_copy_dirty(&map, 1u, dirty, 2u) == 2u);
  CHECK((dirty[0] & 1u) == 1u && (dirty[0] & 2u) == 0u);
  thor_rom_properties_dispose(&map);
  thor_rom_properties_dispose(&audio_map);
  thor_rom_properties_dispose(&graphics_map);
  CHECK(map.values == 0 && map.dirty_pages == 0);
  return 0;
}
