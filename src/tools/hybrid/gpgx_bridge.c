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
