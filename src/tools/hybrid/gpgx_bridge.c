/* Developer-only bridge compiled inside an external HOOK_CPU GPGX build.
 * The block API is only for mechanical timing experiments and is never linked
 * into the production Sega-Thor target.
 * retro_ prefix uses the existing libretro export map; this is not libretro ABI.
 */
#include "shared.h"
#include "cpuhook.h"
#include "libretro.h"

RETRO_API unsigned int retro_hybrid_abi(void) { return 1; }

RETRO_API void retro_hybrid_install(void (*callback)(hook_type_t, int, unsigned int, unsigned int))
{
    set_cpu_hook(callback);
}

RETRO_API void retro_hybrid_install_block(int (*callback)(unsigned int))
{
    set_cpu_block_hook(callback);
}

RETRO_API unsigned int retro_hybrid_register(unsigned int reg)
{
    return reg < 18 ? m68k_get_reg((m68k_register_t)reg) : 0;
}

/* Read-only, side-effect-free ROM/work-RAM access; reject all mapped I/O. */
RETRO_API int retro_hybrid_peek(unsigned int address)
{
    cpu_memory_map *map;
    address &= 0xFFFFFF;
    if (!(address < cart.romsize || address >= 0xFF0000)) return -1;
    map = &m68k.memory_map[address >> 16];
    if (!map->base || map->read8 || map->read16) return -1;
    return READ_BYTE(map->base, address & 0xFFFF);
}

RETRO_API void retro_hybrid_set_register(unsigned int reg, unsigned int value)
{
    if (reg < 18) m68k_set_reg((m68k_register_t)reg, value);
}

RETRO_API int retro_hybrid_cycles(void) { return m68k.cycles; }

RETRO_API void retro_hybrid_add_cycles(int delta) { m68k.cycles += delta; }
RETRO_API void retro_hybrid_skip_bus_refresh(void)
{
    if (m68k.cycles >= m68k.refresh_cycles) m68k.refresh_cycles += 128 * 7;
}
RETRO_API int retro_hybrid_refresh_cycles(void) { return m68k.refresh_cycles; }

RETRO_API unsigned int retro_hybrid_fetch16(void)
{
    return m68k_hybrid_fetch16();
}

RETRO_API unsigned int retro_hybrid_read(unsigned int address, int width)
{
    return m68k_hybrid_read(address, width);
}

RETRO_API void retro_hybrid_write(unsigned int address, int width, unsigned int value)
{
    m68k_hybrid_write(address, width, value);
}

RETRO_API void retro_hybrid_begin_instruction(unsigned int opcode)
{
    m68k_hybrid_begin_instruction(opcode);
}

RETRO_API void retro_hybrid_finish_instruction(unsigned int opcode)
{
    m68k_hybrid_finish_instruction(opcode);
}

RETRO_API unsigned int retro_hybrid_instruction_cycles(unsigned int opcode)
{
    return m68k_hybrid_instruction_cycles(opcode);
}

RETRO_API unsigned int retro_hybrid_cpu_field(unsigned int field)
{
    return m68k_hybrid_cpu_field(field);
}

RETRO_API unsigned int retro_hybrid_refresh_period(void) { return m68k_hybrid_refresh_period(); }
RETRO_API unsigned int retro_hybrid_refresh_penalty(void) { return m68k_hybrid_refresh_penalty(); }

RETRO_API void retro_hybrid_set_return_state(unsigned int pc, unsigned int pref_addr,
                                              unsigned int pref_data)
{
    m68k.pc = pc;
    m68k.pref_addr = pref_addr;
    m68k.pref_data = pref_data;
    m68k.ir = 0x4E75;
}

RETRO_API void retro_hybrid_poke(unsigned int address, int width, unsigned int value)
{
    cpu_memory_map *map;
    unsigned int index;
    address &= 0xFFFFFF;
    if (width != 1 && width != 2 && width != 4) return;
    if (address < cart.romsize || address < 0xFF0000) return;
    map = &m68k.memory_map[address >> 16];
    if (!map->base || map->read8 || map->read16 || map->write8 || map->write16) return;
    for (index = 0; index < (unsigned int)width; index++)
      WRITE_BYTE(map->base, (address + index) & 0xffff,
                 (value >> (8 * (width - 1 - index))) & 0xff);
}
