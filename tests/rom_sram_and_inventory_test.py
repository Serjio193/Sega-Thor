#!/usr/bin/env python3
"""
rom_sram_and_inventory_test.py — Milestone M15.265

Verifies the extracted SRAM Save/Load System (0x001182..0x001360) and
Persistent WRAM Inventory/Progression Layout ($FF0BFE..$FF0E51).
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_sram_and_inventory_m15_265.json")
EXTRACTOR_PATH = Path("tools/extract_rom_sram_and_inventory.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    # 1. Verify SRAM validation and slot layout
    sram = data["sram_system"]
    assert sram["magic_hex"] == "1abc00022a4e"
    assert sram["payload_bytes"] == 596
    assert sram["sram_ctrl_reg"] == "0x00A130F1"
    assert len(sram["slots"]) == 4
    assert sram["slots"][0] == {"slot": 0, "primary": "0x200009", "backup": "0x2012E9"}
    assert sram["slots"][3] == {"slot": 3, "primary": "0x200E31", "backup": "0x202111"}
    assert sram["last_slot_sram"] == "0x2025C9"

    # 2. Verify persistent WRAM block bounds ($FF0BFE..$FF0E51 = 596 bytes)
    wram = data["persistent_wram_block"]
    assert wram["base_addr"] == "0x00FF0BFE"
    assert wram["end_addr_inclusive"] == "0x00FF0E51"
    assert wram["size_bytes"] == 596

    # 3. Verify 7 weapon slots and 16 item slots
    inv = data["inventory_defaults_0x001450"]
    assert inv["weapon_slots_count"] == 7
    assert inv["weapon_init_words"] == ["0x05FE", "0x07FE", "0x08FE", "0x0CFE", "0x09FE", "0x0AFE", "0x0BFE"]
    assert inv["item_slots_count"] == 16

    print("ALL 3 M15.265 SRAM & INVENTORY TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
