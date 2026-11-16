"""Extract Event VM Opcode 0x12 conditional branch sub-actions (0x00FAB2..0x00FC6B).

Milestone: M15.279.
Evidence anchors in Beyond Oasis (USA).md:
  - 0x00FAB2: Opcode 0x12 dispatcher (reads sub-action at +2(a0), indexes 0x00FAC2)
  - 0x00FAC2: 12-entry signed self-relative jump table (sub-actions 0x00..0x0B)
  - 0x00FB64: 6-entry signed self-relative comparator table (==, !=, >, >=, <=, <)
  - 0x00FBA0: True branch (adda.w 6(a0), a0)
  - 0x00FBA6: False branch (lea 8(a0), a0)
  - 0x0101EC: Count active non-spirit entities in $FF1CD8 (17 slots, stride 0xBC)
  - 0x010232: Search entity ID in $FF1CD8 (carry set if found)
  - 0x010254: Compute player HP percentage ($FF1A72 * 100 / $FF1A70)
  - 0x010268: Compute player SP percentage ($FF1A76 * 100 / $FF1A74)
"""

from __future__ import annotations

import json
import struct
import sys
from pathlib import Path

DEFAULT_ROM_PATH = Path("local-roms/Beyond Oasis (USA).md")
DEFAULT_OUT_PATH = Path("data/thor-evidence/rom_event_vm_op12_m15_279.json")

DISPATCH_START = 0x00FAB2
SUB_ACTION_TABLE_ADDR = 0x00FAC2
SUB_ACTION_COUNT = 12
COMPARATOR_TABLE_ADDR = 0x00FB64
COMPARATOR_COUNT = 6

SUB_ACTION_METADATA = [
    (0x00, "active_non_spirit_entity_count", "0x0101EC", "Compare count of active non-spirit entities ($FF1CD8)"),
    (0x01, "global_event_flag_bit", "0xFF0DBC", "Test bit imm16 in global event flags $FF0DBC"),
    (0x02, "chest_pickup_flag_bit", "0xFF0E1C", "Test bit imm16 in chest/pickup flags $FF0E1C"),
    (0x03, "secondary_flag_bit", "0xFF0E2C", "Test bit imm16 in secondary flags $FF0E2C"),
    (0x04, "entity_id_present", "0x010232", "Test if entity ID imm16 is present in $FF1CD8"),
    (0x05, "player_hp_percent", "0x010254", "Compare player HP% ($FF1A72*100/$FF1A70)"),
    (0x06, "player_sp_percent", "0x010268", "Compare player SP% ($FF1A76*100/$FF1A74)"),
    (0x07, "active_spirit_id", "0xFF1AA4", "Compare active spirit ID $FF1AA4 against imm16"),
    (0x08, "player_x", "0xFF19F0", "Compare player X ($FF19F0)"),
    (0x09, "player_y", "0xFF19F4", "Compare player Y ($FF19F4)"),
    (0x0A, "spirit_x_if_1a", "0xFF1AAC", "If $FF1AA4 == 0x1A, compare spirit X ($FF1AAC)"),
    (0x0B, "spirit_y_if_1a", "0xFF1AB0", "If $FF1AA4 == 0x1A, compare spirit Y ($FF1AB0)"),
]

COMPARATOR_OPS = [
    (0, "eq", "beq.w 0x00FBA0"),
    (2, "ne", "bne.w 0x00FBA0"),
    (4, "gt", "bgt.w 0x00FBA0"),
    (6, "ge", "bge.w 0x00FBA0"),
    (8, "le", "ble.w 0x00FBA0"),
    (10, "lt", "blt.w 0x00FBA0"),
]


def read_s16(rom: bytes, addr: int) -> int:
    """Read big-endian signed 16-bit integer from ROM."""
    return struct.unpack_from(">h", rom, addr)[0]


def read_u16(rom: bytes, addr: int) -> int:
    """Read big-endian unsigned 16-bit integer from ROM."""
    return struct.unpack_from(">H", rom, addr)[0]


def extract_event_vm_op12(rom_path: Path) -> dict:
    """Extract Opcode 0x12 sub-action and comparator jump tables from ROM."""
    rom = rom_path.read_bytes()

    sub_actions = []
    for idx, name, ref, desc in SUB_ACTION_METADATA:
        entry_addr = SUB_ACTION_TABLE_ADDR + idx * 2
        offset = read_s16(rom, entry_addr)
        target_addr = entry_addr + offset
        sub_actions.append({
            "index": idx,
            "name": name,
            "entry_addr": f"0x{entry_addr:06X}",
            "rel_offset": offset,
            "target_addr": f"0x{target_addr:06X}",
            "reference": ref,
            "description": desc,
        })

    comparators = []
    for i, (selector, op_name, branch_insn) in enumerate(COMPARATOR_OPS):
        entry_addr = COMPARATOR_TABLE_ADDR + i * 2
        offset = read_s16(rom, entry_addr)
        target_addr = entry_addr + offset
        comparators.append({
            "selector_byte": selector,
            "operator": op_name,
            "entry_addr": f"0x{entry_addr:06X}",
            "rel_offset": offset,
            "target_addr": f"0x{target_addr:06X}",
            "branch_insn": branch_insn,
        })

    return {
        "milestone": "M15.279",
        "rom_path": str(rom_path),
        "dispatcher": {
            "addr": f"0x{DISPATCH_START:06X}",
            "sub_action_table_addr": f"0x{SUB_ACTION_TABLE_ADDR:06X}",
            "comparator_table_addr": f"0x{COMPARATOR_TABLE_ADDR:06X}",
            "true_branch_addr": "0x00FBA0",
            "false_branch_addr": "0x00FBA6",
            "instruction_format": {
                "size_bytes_on_false": 8,
                "offset_0_u8": "opcode (0x12)",
                "offset_1_u8": "padding/unused",
                "offset_2_u8": "sub_action (0x00..0x0B)",
                "offset_3_u8": "comparator_selector (0,2,4,6,8,10) or negate_flag (0=true, !=0=false)",
                "offset_4_u16": "imm16 (threshold, bit_index, or entity_id)",
                "offset_6_s16": "true_branch_rel_offset (added to a0 at 0x00FBA0)",
            },
        },
        "helpers": {
            "count_active_non_spirit_entities": {
                "addr": "0x0101EC",
                "entity_table_base": "0xFF1CD8",
                "slot_count": 17,
                "slot_stride_bytes": 0xBC,
                "condition": "word[+0] > 0 && (byte[+0x3A] & 1) == 0",
            },
            "find_entity_id": {
                "addr": "0x010232",
                "entity_table_base": "0xFF1CD8",
                "slot_count": 17,
                "slot_stride_bytes": 0xBC,
                "condition": "word[+0] == d1 (returns carry=1 if found)",
            },
            "player_hp_percent": {
                "addr": "0x010254",
                "formula": "($FF1A72 * 100) / $FF1A70",
            },
            "player_sp_percent": {
                "addr": "0x010268",
                "formula": "($FF1A76 * 100) / $FF1A74",
            },
        },
        "sub_actions": sub_actions,
        "comparators": comparators,
    }


def main(argv: list[str]) -> int:
    rom_path = Path(argv[1]) if len(argv) > 1 else DEFAULT_ROM_PATH
    out_path = Path(argv[2]) if len(argv) > 2 else DEFAULT_OUT_PATH
    if not rom_path.exists():
        print(f"ROM not found: {rom_path}", file=sys.stderr)
        return 1
    data = extract_event_vm_op12(rom_path)
    out_path.parent.mkdir(parents=True, exist_ok=True)
    out_path.write_text(json.dumps(data, indent=2) + "\n", encoding="utf-8")
    print(f"Wrote {len(data['sub_actions'])} sub-actions and {len(data['comparators'])} comparators to {out_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main(sys.argv))
