#!/usr/bin/env python3
"""
rom_enemy_ai_0030_0048_test.py — Milestone M15.269

Verifies extracted ROM AI contracts for Entity 0x0048 (Charger) and
Entity 0x0030 (Room 0x0105 Waterfall Gate Boss).
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_enemy_ai_0030_0048_m15_269.json")
EXTRACTOR_PATH = Path("tools/extract_rom_enemy_ai_0030_0048.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    c48 = data["entity_0048_charger"]
    assert c48["update_handler"] == "0x01778C"
    assert c48["trigger_sfx"] == "0x800399"
    assert len(c48["aabb_boxes"]) == 4
    assert c48["aabb_boxes"][0] == {"facing": 0, "dx1": -1, "dy1": -24, "dx2": 1, "dy2": -1}

    b30 = data["entity_0030_gate_boss"]
    assert b30["update_handler"] == "0x015AE2"
    assert b30["vram_base"] == "0x4200"
    assert b30["child_slots_spawned"] == 5

    print("ALL 3 M15.269 ENEMY AI (0x0030 & 0x0048) TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
