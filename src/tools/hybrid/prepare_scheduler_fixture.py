"""Create an isolated GPGX core build with scheduler-observation hooks."""
from __future__ import annotations

import argparse
from pathlib import Path
import shutil

from prepare_gpgx import prepare


EVENTS = {
    "HYBRID_SCHED_M68K_BEGIN": 1,
    "HYBRID_SCHED_M68K_END": 2,
    "HYBRID_SCHED_Z80_BEGIN": 3,
    "HYBRID_SCHED_Z80_END": 4,
    "HYBRID_SCHED_VDP_ADVANCE": 5,
    "HYBRID_SCHED_IRQ_PENDING": 6,
    "HYBRID_SCHED_Z80_IRQ": 7,
    "HYBRID_SCHED_IRQ_ENTRY": 8,
    "HYBRID_SCHED_IRQ_EXIT": 9,
}


def replace_once(text: str, old: str, new: str, label: str) -> str:
    if text.count(old) != 1:
        raise ValueError(f"expected one {label} anchor, got {text.count(old)}")
    return text.replace(old, new)


def instrument_scheduler(root: Path) -> None:
    header = root / "core/debug/cpuhook.h"
    source = root / "core/system.c"
    cpu_header = root / "core/m68k/m68kcpu.h"
    cpu_source = root / "core/m68k/m68kcpu.c"
    htext = header.read_text(encoding="utf-8")
    decl = "void retro_hybrid_scheduler_event(int kind, int arg0, int arg1);"
    if decl not in htext:
        anchor = "void set_cpu_block_hook(cpu_block_hook_t hook);"
        event_defs = """enum {
  HYBRID_SCHED_M68K_BEGIN = 1, HYBRID_SCHED_M68K_END = 2,
  HYBRID_SCHED_Z80_BEGIN = 3, HYBRID_SCHED_Z80_END = 4,
  HYBRID_SCHED_VDP_ADVANCE = 5, HYBRID_SCHED_IRQ_PENDING = 6,
  HYBRID_SCHED_Z80_IRQ = 7, HYBRID_SCHED_IRQ_ENTRY = 8,
  HYBRID_SCHED_IRQ_EXIT = 9, HYBRID_SCHED_REFRESH = 10
};
unsigned int m68k_hybrid_boundary_reason(void);
int m68k_hybrid_bcc_not_taken_adjustment(void);
int m68k_hybrid_patch_rom(unsigned int address, unsigned int value);
"""
        htext = replace_once(htext, anchor, anchor + "\n" + event_defs + decl, "scheduler API")
        header.write_text(htext, encoding="utf-8")

    text = source.read_text(encoding="utf-8")
    include_anchor = '#include "shared.h"'
    if '#include "debug/cpuhook.h"' not in text:
        text = replace_once(text, include_anchor,
                            include_anchor + '\n#ifdef HOOK_CPU\n#include "debug/cpuhook.h"\n#endif',
                            "scheduler hook include")
    start = text.index("void system_frame_gen(int do_skip)")
    end = text.index("void system_frame_scd(int do_skip)", start)
    frame = text[start:end]
    run_map = {
        "m68k_run(vint_cycle);": ("HYBRID_SCHED_M68K_BEGIN", "HYBRID_SCHED_M68K_END", "vint_cycle"),
        "m68k_run(MCYCLES_PER_LINE);": ("HYBRID_SCHED_M68K_BEGIN", "HYBRID_SCHED_M68K_END", "MCYCLES_PER_LINE"),
        "m68k_run(mcycles_vdp + MCYCLES_PER_LINE);": ("HYBRID_SCHED_M68K_BEGIN", "HYBRID_SCHED_M68K_END", "mcycles_vdp + MCYCLES_PER_LINE"),
        "z80_run(vint_cycle);": ("HYBRID_SCHED_Z80_BEGIN", "HYBRID_SCHED_Z80_END", "vint_cycle"),
        "z80_run(MCYCLES_PER_LINE);": ("HYBRID_SCHED_Z80_BEGIN", "HYBRID_SCHED_Z80_END", "MCYCLES_PER_LINE"),
        "z80_run(mcycles_vdp + MCYCLES_PER_LINE);": ("HYBRID_SCHED_Z80_BEGIN", "HYBRID_SCHED_Z80_END", "mcycles_vdp + MCYCLES_PER_LINE"),
    }
    for call, (before, after, target) in run_map.items():
        wrapped = (
            f"retro_hybrid_scheduler_event({before}, {target}, line);\n    {call}\n"
            f"    retro_hybrid_scheduler_event({after}, {target}, line);"
        )
        if call in frame:
            frame = frame.replace(call, wrapped)

    if frame.count("retro_hybrid_scheduler_event(HYBRID_SCHED_M68K_BEGIN") != 5:
        raise ValueError("system_frame_gen M68K run census changed")
    if frame.count("retro_hybrid_scheduler_event(HYBRID_SCHED_Z80_BEGIN") != 5:
        raise ValueError("system_frame_gen Z80 run census changed")

    frame = frame.replace(
        "mcycles_vdp = MCYCLES_PER_LINE;",
        "mcycles_vdp = MCYCLES_PER_LINE;\n  retro_hybrid_scheduler_event(HYBRID_SCHED_VDP_ADVANCE, mcycles_vdp, line);",
    )
    frame = frame.replace(
        "mcycles_vdp += MCYCLES_PER_LINE;",
        "mcycles_vdp += MCYCLES_PER_LINE;\n    retro_hybrid_scheduler_event(HYBRID_SCHED_VDP_ADVANCE, mcycles_vdp, line);",
    )
    frame = frame.replace(
        "m68k_update_irq(4);",
        "retro_hybrid_scheduler_event(HYBRID_SCHED_IRQ_PENDING, 4, line);\n      m68k_update_irq(4);",
    )
    frame = frame.replace(
        "m68k_set_irq(6);",
        "retro_hybrid_scheduler_event(HYBRID_SCHED_IRQ_PENDING, 6, line);\n      m68k_set_irq(6);",
    )
    frame = frame.replace(
        "Z80.irq_state = ASSERT_LINE;",
        "Z80.irq_state = ASSERT_LINE;\n    retro_hybrid_scheduler_event(HYBRID_SCHED_Z80_IRQ, ASSERT_LINE, line);",
    )
    frame = frame.replace(
        "Z80.irq_state = CLEAR_LINE;",
        "Z80.irq_state = CLEAR_LINE;\n  retro_hybrid_scheduler_event(HYBRID_SCHED_Z80_IRQ, CLEAR_LINE, line);",
    )
    frame = frame.replace(", line);", ", v_counter);")
    source.write_text(text[:start] + frame + text[end:], encoding="utf-8")

    ctext = cpu_header.read_text(encoding="utf-8")
    entry_anchor = "INLINE void m68ki_exception_interrupt(uint int_level)\n{"
    entry = entry_anchor + "\n#ifdef HOOK_CPU\n  retro_hybrid_scheduler_event(8, int_level, 0);\n#endif"
    ctext = replace_once(ctext, entry_anchor, entry, "IRQ entry")
    end_anchor = "USE_CYCLES(m68ki_cycle_interrupts[(m68ki_cpu.cycles / MUL) % 10]);\n}"
    end = "USE_CYCLES(m68ki_cycle_interrupts[(m68ki_cpu.cycles / MUL) % 10]);\n#ifdef HOOK_CPU\n  retro_hybrid_scheduler_event(9, int_level, new_pc);\n#endif\n}"
    ctext = replace_once(ctext, end_anchor, end, "IRQ exit")
    boundary = """unsigned int m68k_hybrid_boundary_reason(void)
{
  if (CPU_INT_LEVEL > FLAG_INT_MASK) return 3;
  if (FLAG_T1) return 4;
  if (m68k.cycles >= m68k.cycle_end) return 2;
  return 0;
}

int m68k_hybrid_bcc_not_taken_adjustment(void)
{
  return CYC_BCC_NOTAKE_B;
}

"""
    cpu_header.write_text(ctext, encoding="utf-8")

    begin_anchor = "if (m68k.cycles >= m68k.refresh_cycles)\n  {\n    m68k.refresh_cycles = m68k.cycles + (128 * 7);\n    m68k.cycles += (2 * 7);\n  }"
    begin_replacement = "if (m68k.cycles >= m68k.refresh_cycles)\n  {\n    retro_hybrid_scheduler_event(HYBRID_SCHED_REFRESH, m68k.cycles, m68k.refresh_cycles);\n    m68k.refresh_cycles = m68k.cycles + (128 * 7);\n    m68k.cycles += (2 * 7);\n    retro_hybrid_scheduler_event(HYBRID_SCHED_REFRESH, m68k.cycles, m68k.refresh_cycles);\n  }"
    ctext = cpu_source.read_text(encoding="utf-8")
    patch_anchor = "unsigned int m68k_hybrid_instruction_cycles(unsigned int opcode)"
    patch_helper = """int m68k_hybrid_patch_rom(unsigned int address, unsigned int value)
{
  cpu_memory_map *map;
  if (address >= 0x1000000) return 0;
  map = &m68ki_cpu.memory_map[(address >> 16) & 0xff];
  if (!map->base || map->read8 || map->read16) return 0;
  WRITE_BYTE(map->base, address & 0xffff, value);
  return 1;
}

"""
    ctext = replace_once(ctext, patch_anchor, patch_helper + patch_anchor,
                         "fixture patch against internal M68K map")
    cycle_anchor = "int m68k_cycles(void)\n{\n  return CYC_INSTRUCTION[REG_IR];\n}"
    ctext = replace_once(ctext, cycle_anchor, boundary + cycle_anchor, "M68K boundary provider")
    ctext = replace_once(ctext, begin_anchor, begin_replacement, "refresh event")
    run_refresh = "if (m68k.cycles >= m68k.refresh_cycles)\n    {\n      m68k.refresh_cycles = m68k.cycles + (128*7);\n      m68k.cycles += (2*7);\n    }"
    run_refresh_events = "if (m68k.cycles >= m68k.refresh_cycles)\n    {\n      retro_hybrid_scheduler_event(HYBRID_SCHED_REFRESH, m68k.cycles, m68k.refresh_cycles);\n      m68k.refresh_cycles = m68k.cycles + (128*7);\n      m68k.cycles += (2*7);\n      retro_hybrid_scheduler_event(HYBRID_SCHED_REFRESH, m68k.cycles, m68k.refresh_cycles);\n    }"
    ctext = replace_once(ctext, run_refresh, run_refresh_events, "ROM instruction refresh event")
    cpu_source.write_text(ctext, encoding="utf-8")


def prepare_fixture(source: Path, destination: Path) -> None:
    source = source.resolve()
    destination = destination.resolve()
    if not (source / "Makefile.libretro").is_file():
        raise FileNotFoundError(f"GPGX source not found: {source}")
    if destination.exists():
        shutil.rmtree(destination)
    shutil.copytree(source, destination, ignore=shutil.ignore_patterns(
        ".git", "*.dll", "*.so", "*.dylib", "*.a", "*.o", "*.obj", "*.exe"
    ))
    prepare(destination)
    instrument_scheduler(destination)
    makefile = destination / "Makefile.libretro"
    text = makefile.read_text(encoding="utf-8")
    text = replace_once(text, "HOOK_CPU = 0", "HOOK_CPU = 1", "isolated HOOK_CPU build flag")
    makefile.write_text(text, encoding="utf-8")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("source", type=Path)
    parser.add_argument("destination", type=Path)
    args = parser.parse_args()
    prepare_fixture(args.source, args.destination)


if __name__ == "__main__":
    main()
