#!/usr/bin/env python3
"""
M15.243 Connected Entity State Dispatch Subsystem Generator.
Decodes subsystem and generates IR manifests:
  1. src/tools/thor_ir/entity_dispatch_subsystem_ir.json
  2. data/thor-fixtures/m15_243_ir_manifest.json
Verifies src/tools/thor_ir/generated/entity_dispatch_portable_routines.hpp <= 500 lines.
Strictly <= 500 lines per AGENTS.md Rule 3.
"""

import json
from pathlib import Path
import sys

sys.path.insert(0, str(Path(__file__).resolve().parent.parent))
from tools.thor_recompiler.decoder import decode_at


def decode_subsystem(rom):
    ranges = [
        ("dispatcher", 0x0059A8, 0x0059BA),
        ("helper_5c82", 0x005C82, 0x005CC8),
        ("handler_12", 0x005CC8, 0x005CF8),
        ("handler_13", 0x005CF8, 0x005DA6),
        ("handler_17", 0x005DA6, 0x005DC0),
        ("handler_15", 0x005DC6, 0x005DC8),
    ]

    all_insns = []
    blocks = []
    for section_name, s, e in ranges:
        pc = s
        sec_insns = []
        while pc < e:
            insn = decode_at(rom, pc)
            op_dict = {
                "pc": f"0x{pc:06X}",
                "mnemonic": insn.mnemonic,
                "size": insn.size,
                "cycles": insn.base_cycles,
                "operands": [{"kind": op.kind, "val": hex(op.val)} for op in insn.ops],
            }
            sec_insns.append(op_dict)
            all_insns.append(op_dict)
            pc += len(insn.raw_words) * 2
        blocks.append({
            "section": section_name,
            "start_pc": f"0x{s:06X}",
            "end_pc": f"0x{e:06X}",
            "instruction_count": len(sec_insns),
            "instructions": sec_insns,
        })
    return all_insns, blocks


def main():
    root = Path(__file__).resolve().parent.parent
    rom_path = root / "local-roms" / "Beyond Oasis (USA).md"
    with open(rom_path, "rb") as f:
        rom = f.read()

    insns, blocks = decode_subsystem(rom)
    print(f"Decoded {len(insns)} distinct instructions across {len(blocks)} CFG blocks.")

    ir_manifest = {
        "schema": "oasis.thor-ir.subsystem.v1",
        "milestone": "M15.243",
        "name": "connected_entity_state_dispatch_subsystem",
        "entry_pc": "0x0059A8",
        "jump_table_pc": "0x0059BA",
        "instruction_count": len(insns),
        "block_count": len(blocks),
        "blocks": blocks,
    }

    ir_path = root / "src" / "tools" / "thor_ir" / "entity_dispatch_subsystem_ir.json"
    ir_path.parent.mkdir(parents=True, exist_ok=True)
    ir_path.write_text(json.dumps(ir_manifest, indent=2), encoding="utf-8")
    print(f"Wrote IR manifest to {ir_path}")

    fixtures_manifest_path = root / "data" / "thor-fixtures" / "m15_243_ir_manifest.json"
    fixtures_manifest_path.parent.mkdir(parents=True, exist_ok=True)
    fixtures_manifest_path.write_text(json.dumps(ir_manifest, indent=2), encoding="utf-8")
    print(f"Wrote fixture manifest to {fixtures_manifest_path}")

    header_path = root / "src" / "tools" / "thor_ir" / "generated" / "entity_dispatch_portable_routines.hpp"
    if not header_path.exists():
        print(f"FATAL: header not found at {header_path}")
        sys.exit(1)
    lines = len(header_path.read_text(encoding="utf-8").splitlines())
    print(f"Verified generated header at {header_path}: {lines} lines <= 500")
    if lines > 500:
        print(f"FATAL: header exceeds 500 lines: {lines}")
        sys.exit(1)


if __name__ == "__main__":
    main()
