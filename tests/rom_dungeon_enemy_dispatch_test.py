#!/usr/bin/env python3
"""
rom_dungeon_enemy_dispatch_test.py — Milestone M15.276

Verifies extracted ROM substate dispatch contracts for Dungeon Enemy
Families 0x006C, 0x006E, 0x0070, and 0x0072.
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_dungeon_enemy_dispatch_m15_276.json")
EXTRACTOR_PATH = Path("tools/extract_rom_dungeon_enemy_dispatch.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
    fam = data["families"]
    assert fam["0x006C"]["variants"] == [3, 6, 9, 12]
    assert fam["0x006E"]["substate_count"] == 10
    assert fam["0x0070"]["substate_count"] == 6
    assert fam["0x0072"]["substate_count"] == 10
    assert fam["0x0072"]["substates"]["0"] == "0x01D412"

    print("ALL 3 M15.276 DUNGEON ENEMY DISPATCH TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
