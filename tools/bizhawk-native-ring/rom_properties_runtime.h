#ifndef THOR_ROM_PROPERTIES_RUNTIME_H
#define THOR_ROM_PROPERTIES_RUNTIME_H

#include <stdint.h>

int oasis_romprops_init(uint32_t rom_size);
int oasis_romprops_clear(void);
void oasis_romprops_enable(uint32_t enabled);
uint32_t oasis_romprops_size(void);
uint64_t oasis_romprops_changed_bytes(void);
uint64_t oasis_romprops_change_operations(void);
uint32_t oasis_romprops_copy(uint32_t offset, uint16_t *output,
                             uint32_t capacity);
uint32_t oasis_romprops_copy_dirty(uint32_t first_page, uint8_t *output,
                                   uint32_t capacity);

void oasis_romprops_m68k_data_read(uint32_t address, uint32_t value,
                                   uint32_t width);
void oasis_romprops_z80_data_read(uint32_t address, uint8_t value);
void oasis_romprops_vdp_dma_source(uint32_t address, uint16_t value,
                                   uint32_t destination_code);
void oasis_romprops_vdp_cpu_source_write(uint16_t value,
                                         uint32_t destination_code,
                                         uint32_t dma_fill_pending);
void oasis_romprops_m68k_begin(void);
void oasis_romprops_m68k_prefetch(uint32_t address, uint16_t value);
void oasis_romprops_m68k_consume_word(uint32_t address, uint16_t value);
void oasis_romprops_m68k_finish(uint32_t completed);
void oasis_romprops_m68k_abort(void);

void oasis_romprops_z80_begin(void);
void oasis_romprops_z80_fetch(uint16_t address, const uint8_t *source,
                              uint8_t value);
void oasis_romprops_z80_finish(uint32_t completed);

#endif
