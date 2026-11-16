#!/usr/bin/env python3
"""
rom_dytto_abilities_test.py — Milestone M15.263

Automated verification suite for Dytto Spirit Abilities, State Machine
(0x00..0x1A), Water Projectile Spawning, and Elemental Damage extraction.
Runs `extract_rom_dytto_abilities.py` and validates the emitted JSON and
canonical ROM contracts.
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_dytto_abilities_m15_263.json")
EXTRACTOR_PATH = Path("tools/extract_rom_dytto_abilities.py")


def main() -> int:
    # 1. Run extractor deterministically
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    # 2. Verify all 14 state table entries (0x00..0x1A)
    st = data["state_table"]
    assert st["address"] == "0x014914"
    assert st["entry_count"] == 14
    handlers = st["handlers"]
    assert handlers["0x00"] == "0x014932"
    assert handlers["0x04"] == "0x014A5C"
    assert handlers["0x06"] == "0x014ABC"
    assert handlers["0x08"] == "0x014AEA"
    assert handlers["0x0A"] == "0x014B50"
    assert handlers["0x0C"] == "0x014D1A"
    assert handlers["0x0E"] == "0x014EE6"
    assert handlers["0x10"] == "0x014A72"
    assert handlers["0x12"] == "0x014E1A"
    assert handlers["0x14"] == "0x014E0C"
    assert handlers["0x16"] == "0x014B7C"
    assert handlers["0x18"] == "0x014BB8"
    assert handlers["0x1A"] == "0x014F60"

    # 3. Verify PRNG autonomous dispatch thresholds
    contracts = data["verified_rom_contracts"]
    assert contracts["prng_thresholds"] == ["0x2000", "0x6000", "0x9000"]

    # 4. Verify Dytto 3-way spread attack (State 0x10 -> 0x015214)
    assert contracts["dytto_special_guard_type"] == "0x0018"
    assert contracts["spread_angles"] == [0, 28, -28]
    assert contracts["projectile_entity_type"] == "0x0024"

    # 5. Verify Water Elemental Damage word (0x2008 / 0xA008) & knockback
    assert contracts["water_damage_ground"] == "0x2008"
    assert contracts["water_damage_airborne"] == "0xA008"
    assert contracts["knockback_xy_damping"] == "0x00008000"
    assert contracts["knockback_z_damping"] == "0x0000A000"

    print("ALL 5 M15.263 DYTTO ABILITIES TESTS PASSED (5/5)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
