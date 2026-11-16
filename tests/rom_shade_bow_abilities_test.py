#!/usr/bin/env python3
"""
rom_shade_bow_abilities_test.py — Milestone M15.274

Verifies extracted ROM Shade (0x001C) and Bow (0x001E) abilities & SP costs.
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_shade_bow_abilities_m15_274.json")
EXTRACTOR_PATH = Path("tools/extract_rom_shade_bow_abilities.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
    sh = data["shade_0x001C"]["abilities"]
    assert sh["shadow_tether"]["sp_cost"] == 7 and sh["shadow_tether"]["next_state"] == "0x06"
    assert sh["shadow_surge"]["next_state"] == "0x0C"

    bw = data["bow_0x001E"]["abilities"]
    assert bw["bite"]["sp_cost"] == 4
    assert bw["pollen_vine"]["sp_cost"] == 9 and bw["pollen_vine"]["next_state"] == "0x04"
    assert bw["plant_storm"]["sp_cost"] == 28 and bw["plant_storm"]["next_state"] == "0x0A"

    print("ALL 3 M15.274 SHADE & BOW ABILITIES TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
