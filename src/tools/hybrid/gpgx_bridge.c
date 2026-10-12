/* Developer-only bridge compiled inside an external HOOK_CPU GPGX build.
 * The block API is only for mechanical timing experiments and is never linked
 * into the production Sega-Thor target.
 * retro_ prefix uses the existing libretro export map; this is not libretro ABI.
 */
#include "shared.h"
#include "cpuhook.h"
#include "libretro.h"

extern int m68k_hybrid_patch_rom(unsigned int address, unsigned int value);

static void (*scheduler_callback)(int, int, int);
static void (*cpu_callback)(hook_type_t, int, unsigned int, unsigned int);

static unsigned int hybrid_cpu_hook(hook_type_t type, int width,
                                    unsigned int address, unsigned int value)
{
    if (cpu_callback) cpu_callback(type, width, address, value);
    return value;
}

RETRO_API unsigned int retro_hybrid_abi(void) { return 2; }
RETRO_API unsigned int retro_hybrid_scheduler_abi(void) { return 1; }

RETRO_API void retro_hybrid_install_scheduler(void (*callback)(int, int, int))
{
    scheduler_callback = callback;
}

void retro_hybrid_scheduler_event(int kind, int arg0, int arg1)
{
    if (scheduler_callback) scheduler_callback(kind, arg0, arg1);
}

RETRO_API unsigned int retro_hybrid_boundary_reason(void)
{
    return m68k_hybrid_boundary_reason();
}

RETRO_API int retro_hybrid_bcc_not_taken_adjustment(void)
{
    return m68k_hybrid_bcc_not_taken_adjustment();
}

/* Read the scheduler's native GPGX fields; no derived timing aliases. */
RETRO_API int retro_hybrid_scheduler_field(unsigned int field)
{
    switch (field)
    {
    case 0: return m68k.cycles;
    case 1: return m68k.cycle_end;
    case 2: return m68k.refresh_cycles;
    case 3: return m68k_get_reg(M68K_REG_PC);
    case 4: return m68k_get_reg(M68K_REG_SR);
    case 5: return Z80.cycles;
    case 6: return v_counter;
    case 7: return mcycles_vdp % MCYCLES_PER_LINE;
    case 8: return m68k.int_level;
    case 9: return m68k.int_mask;
    case 10: return Z80.irq_state;
    case 11: return mcycles_vdp;
    case 12: return m68k.int_level;
    case 13: return m68k.int_mask;
    case 14: return m68k.stopped;
    case 15: return zstate;
    default: return -1;
    }
}

RETRO_API int retro_hybrid_z80_field(unsigned int field)
{
    switch (field)
    {
    case 0: return Z80.pc.w.l;
    case 1: return Z80.sp.w.l;
    case 2: return Z80.af.w.l;
    case 3: return Z80.bc.w.l;
    case 4: return Z80.de.w.l;
    case 5: return Z80.hl.w.l;
    case 6: return Z80.ix.w.l;
    case 7: return Z80.iy.w.l;
    case 8: return Z80.r;
    case 9: return Z80.i;
    case 10: return Z80.im;
    case 11: return Z80.iff1;
    case 12: return Z80.iff2;
    case 13: return Z80.halt;
    case 14: return Z80.irq_state;
    case 15: return Z80.nmi_state;
    case 16: return Z80.nmi_pending;
    default: return -1;
    }
}

RETRO_API int retro_hybrid_vdp_field(unsigned int field)
{
    switch (field)
    {
    case 0: return mcycles_vdp;
    case 1: return v_counter;
    case 2: return h_counter;
    case 3: return status;
    case 4: return hint_pending;
    case 5: return vint_pending;
    case 6: return reg[0];
    case 7: return reg[1];
    case 8: return reg[10];
    default: return -1;
    }
}

RETRO_API int retro_hybrid_fixture_prepare(unsigned int pc, unsigned int sp,
                                            unsigned int sr, int cycles,
                                            int refresh, int z80_cycles,
                                            unsigned int scanline)
{
    cpu_memory_map *map;
    unsigned int offset;
    unsigned int first_word;
    if (!cart.romsize || scanline >= lines_per_frame) return 0;
    pc &= 0xFFFFFF;
    map = &m68k.memory_map[pc >> 16];
    if (!map->base || map->read8 || map->read16) return 0;
    offset = pc & 0xFFFF;
    first_word = (READ_BYTE(map->base, offset) << 8) |
                 READ_BYTE(map->base, (offset + 1) & 0xFFFF);
    m68k_set_reg(M68K_REG_SR, sr);
    m68k_set_reg(M68K_REG_A7, sp);
    m68k.pc = pc;
    m68k.pref_addr = pc + 2;
    m68k.pref_data = first_word;
    m68k.ir = 0;
    m68k.cycles = cycles;
    m68k.refresh_cycles = refresh;
    m68k.cycle_end = 0;
    m68k.int_level = 0;
    m68k.stopped = 0;
    Z80.cycles = z80_cycles;
    v_counter = scanline;
    h_counter = 1;
    return 1;
}

RETRO_API void retro_hybrid_fixture_h_irq(unsigned int enable)
{
    reg[0] = enable ? 0x10 : 0;
    reg[10] = 0xFF;
    h_counter = enable ? 0 : 1;
}

RETRO_API int retro_hybrid_fixture_patch_rom(unsigned int address,
                                              unsigned int value)
{
    if (address >= cart.romsize || address >= MAXROMSIZE) return 0;
    return m68k_hybrid_patch_rom(address, value);
}

RETRO_API void retro_hybrid_fixture_run_frame(void)
{
    system_frame_gen(1);
}

RETRO_API unsigned int retro_hybrid_fixture_viewport_height(void)
{
    return bitmap.viewport.h;
}

RETRO_API void retro_hybrid_fixture_enable_z80(void)
{
    zstate = 1;
}

RETRO_API void retro_hybrid_install(void (*callback)(hook_type_t, int, unsigned int, unsigned int))
{
    cpu_callback = callback;
    set_cpu_hook(hybrid_cpu_hook);
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
