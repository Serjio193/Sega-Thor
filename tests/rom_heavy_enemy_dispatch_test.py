#!/usr/bin/env python3
"""
rom_heavy_enemy_dispatch_test.py — Milestone M15.277

Verifies extracted ROM substate dispatch contracts for Heavy/Boss Enemy
Families 0x0068, 0x0074, 0x0076, and 0x0080.
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_heavy_enemy_dispatch_m15_277.json")
EXTRACTOR_PATH = Path("tools/extract_rom_heavy_enemy_dispatch.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
    fam = data["families"]
    assert fam["0x0068"]["substate_0_target"] == "0x025D30"
    assert fam["0x0074"]["substate_0_target"] == "0x027DDA"
    assert fam["0x0076"]["substate_mask"] == "0x7F" and fam["0x0076"]["init_timer_frames"] == 20
    assert fam["0x0080"]["init_timer_frames"] == 40

    print("ALL 3 M15.277 HEAVY ENEMY DISPATCH TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
