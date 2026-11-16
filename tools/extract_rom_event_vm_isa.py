"""Extract Event VM main interpreter & 31-opcode ISA (0x00F7B4..0x00F80D).

Milestone: M15.280.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00F788: Event VM tick entry (checks $FF17B8 bit 1, $FF17B6 wait timer, $FF17BD pan)
  - 0x00F7B4: Event VM fetch/dispatch loop ($FF17B2 PC, jump table 0x00F7D0)
  - 0x00F7D0: 31-entry signed self-relative jump table (opcodes 0x00..0x1E)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_event_vm_isa_m15_280.json")

TICK_ENTRY_ADDR = 0x00F788
LOOP_ENTRY_ADDR = 0x00F7B4
JUMP_TABLE_ADDR = 0x00F7D0
OPCODE_COUNT = 31

OPCODE_NAMES = [
    (0x00, "end_script_yield", "Pop return addr (0x00F91C) and yield"),
    (0x01, "spawn_enemy_if_cap", "Spawn entity if active count < $FF17B7"),
    (0x02, "set_spawn_table_1", "Load u16 list (0xFFFF-terminated) into $FF17C6"),
    (0x03, "set_spawn_table_2", "Load (u16,u16) pairs (0xFFFF-terminated) into $FF17E6"),
    (0x04, "wait_until_condition", "Poll condition 0..7; yield without advancing PC if false"),
    (0x05, "jump_rel16", "Unconditional relative jump a0 += s16(+2(a0))"),
    (0x06, "loop_counter_dec_branch", "Dec $FF1826[+1(a0)]; branch if nonzero else a0 += 4"),
    (0x07, "loop_counter_set", "Set $FF1826[+1(a0)] = u16(+2(a0))"),
    (0x08, "wait_frames", "Set $FF17B6 = +1(a0), save PC, and yield"),
    (0x09, "spawn_effect_09", "Allocate slot via 0x00D9BE, set entity type 0x0008"),
    (0x0A, "call_entity_handler_0A", "Call 0x00D9A4 with a6=u32(+2), d6=u16(+6)"),
    (0x0B, "set_max_active_enemies", "Set $FF17B7 = +1(a0)"),
    (0x0C, "load_palette_0C", "Load palette to $FF136C from 0x05D046 (id in +2(a0))"),
    (0x0D, "load_palette_0D", "Load palette to $FF136C from 0x05D046 (id in +2(a0))"),
    (0x0E, "spawn_entity_0E", "Spawn entity if active count < $FF17B7 via 0x0102BE"),
    (0x0F, "set_clear_global_flag", "Set/clear bit u16(+2(a0)) in $FF0DBC"),
    (0x10, "set_clear_chest_flag", "Set/clear bit u16(+2(a0)) in $FF0E1C"),
    (0x11, "set_clear_secondary_flag", "Set/clear bit u16(+2(a0)) in $FF0E2C"),
    (0x12, "branch_if_condition", "Conditional branch (12 sub-actions at 0x00FAC2, M15.279)"),
    (0x13, "call_subroutine_24bit", "Save return PC in $FF1836; jump to 24-bit ROM ptr"),
    (0x14, "return_from_subroutine", "Restore PC from $FF1836"),
    (0x15, "branch_if_flag_15", "Branch on flag bit in $FF0DBC/$FF0E1C/$FF0E2C"),
    (0x16, "spawn_item_or_trigger_16", "Allocate slot via 0x00DA2A; set type/+0xA2/+0xA4"),
    (0x17, "spawn_entity_cap_minus_1", "Spawn entity if active count < ($FF17B7 - 1)"),
    (0x18, "spawn_special_18", "Allocate slot via 0x00D9F0 and init entity"),
    (0x19, "spawn_special_19", "Allocate slot via 0x00D9F0 and init entity"),
    (0x1A, "trigger_vdp_or_tile_1A", "Jump to 0x00E78E with d1=u16(+2), d2=u16(+4), d0=u32(+6)"),
    (0x1B, "play_sound_cmd", "Jump to 0x060004 with sound command d0=u32(+2(a0))"),
    (0x1C, "set_event_var_1C", "Set $FF17C4 = u16(+2(a0))"),
    (0x1D, "set_camera_target_1D", "Update camera/scroll table $FF1716[+1(a0)]"),
    (0x1E, "camera_pan_or_fade_1E", "Set $FF17BD, $FF183A, $FF183C, $FF1982=0xFF"),
]


def read_s16(rom: bytes, addr: int) -> int:
    """Read big-endian signed 16-bit integer from ROM."""
    return struct.unpack_from(">h", rom, addr)[0]


def extract_event_vm_isa(rom_path: Path) -> dict:
    """Extract all 31 Event VM main opcodes from jump table 0x00F7D0."""
    rom = rom_path.read_bytes()
    opcodes = []
    for opcode, name, desc in OPCODE_NAMES:
        entry_addr = JUMP_TABLE_ADDR + opcode * 2
        offset = read_s16(rom, entry_addr)
        target_addr = entry_addr + offset
        opcodes.append({
            "opcode": opcode,
            "opcode_hex": f"0x{opcode:02X}",
            "name": name,
            "entry_addr": f"0x{entry_addr:06X}",
            "rel_offset": offset,
            "target_addr": f"0x{target_addr:06X}",
            "description": desc,
        })

    return {
        "milestone": "M15.280",
        "rom_path": str(rom_path),
        "vm_state": {
            "tick_entry_addr": f"0x{TICK_ENTRY_ADDR:06X}",
            "loop_entry_addr": f"0x{LOOP_ENTRY_ADDR:06X}",
            "jump_table_addr": f"0x{JUMP_TABLE_ADDR:06X}",
            "opcode_count": OPCODE_COUNT,
            "ram_pc": "0xFF17B2",
            "ram_wait_frames": "0xFF17B6",
            "ram_max_active_enemies": "0xFF17B7",
            "ram_vm_flags": "0xFF17B8",
            "ram_pan_state": "0xFF17BD",
            "ram_loop_counters": "0xFF1826",
            "ram_return_pc": "0xFF1836",
        },
        "opcodes": opcodes,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_event_vm_isa(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['opcodes'])} Event VM opcodes to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
