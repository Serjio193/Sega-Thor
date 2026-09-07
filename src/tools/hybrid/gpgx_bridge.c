/* Developer-only bridge compiled inside an external HOOK_CPU GPGX build.
 * No CPU mutation API: override remains impossible until its contract exists.
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
