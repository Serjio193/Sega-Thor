#!/usr/bin/env python3
"""
rom_item_effects_test.py — Milestone M15.266

Verifies the extracted ROM Consumable Item Effects (0x1A..0x2A),
Eligibility Rules (0x005236), and Spirit Summon Dispatch (0x00530C).
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_item_effects_m15_266.json")
EXTRACTOR_PATH = Path("tools/extract_rom_item_effects.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    # 1. Verify jump table & 17 items (0x1A..0x2A)
    assert data["jump_table_addr"] == "0x00532C"
    jt = data["jump_targets"]
    assert len(jt) == 17
    assert jt["0x1A"] == "0x005378"
    assert jt["0x1E"] == "0x0053A2"
    assert jt["0x22"] == "0x0053FA"
    assert jt["0x26"] == "0x005452"
    assert jt["0x2A"] == "0x00546A"

    # 2. Verify item effect percentages
    fx = data["item_effects"]
    assert fx["0x1A"]["spirit"] == "Dytto" and fx["0x1A"]["spirit_index"] == 0
    assert fx["0x1E"]["hp_pct"] == 25 and fx["0x21"]["hp_pct"] == 100
    assert fx["0x22"]["sp_pct"] == 25 and fx["0x25"]["sp_pct"] == 100
    assert fx["0x28"]["hp_pct"] == 75 and fx["0x28"]["sp_pct"] == 75
    assert fx["0x2A"]["hp_pct"] == 100 and fx["0x2A"]["sp_pct"] == 100

    # 3. Verify ROM contracts
    c = data["verified_rom_contracts"]
    assert c["heal_sfx"] == "0xA00234"
    assert c["consume_sfx"] == "0xA00256"
    assert c["attribute_table_addr"] == "0x05CE16"

    print("ALL 3 M15.266 ITEM EFFECTS TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
