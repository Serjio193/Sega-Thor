#!/usr/bin/env python3
"""
rom_efreet_abilities_test.py — Milestone M15.273

Verifies extracted ROM Efreet (0x001A) Abilities, State Table (0x0125AE),
and Shared Spirit SP-Cost Guard (0x011B28).
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_efreet_abilities_m15_273.json")
EXTRACTOR_PATH = Path("tools/extract_rom_efreet_abilities.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    assert data["shared_sp_guard"]["routine"] == "0x011B28"
    assert data["shared_sp_guard"]["min_remaining_sp"] == 1

    ab = data["efreet_abilities"]
    assert ab["fire_dash"]["sp_cost"] == 8 and ab["fire_dash"]["sfx"] == "0x800258"
    assert ab["flame_pillar"]["sp_cost"] == 6 and ab["flame_pillar"]["next_state"] == "0x0A"
    assert ab["meltdown"]["sp_cost"] == 25 and ab["meltdown"]["base_duration"] == 96

    st = data["efreet_state_table"]["states"]
    assert len(st) == 12
    assert st["0x08"] == "0x0126AC" and st["0x0A"] == "0x0127A4" and st["0x0C"] == "0x012802"

    print("ALL 3 M15.273 EFREET ABILITIES TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
