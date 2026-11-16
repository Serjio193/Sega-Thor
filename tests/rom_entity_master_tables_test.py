#!/usr/bin/env python3
"""
rom_entity_master_tables_test.py — Milestone M15.264

Verifies the extracted ROM Entity Master Update Dispatch Table (0x00DF54),
32-byte Entity Descriptors (0x03F2FA), and Damage/Elemental Pipeline (0x017E30).
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_entity_master_tables_m15_264.json")
EXTRACTOR_PATH = Path("tools/extract_rom_entity_master_tables.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    # 1. Verify update dispatch table bounds and known handlers
    ud = data["update_dispatch_table"]
    assert ud["address"] == "0x00DF54"
    assert ud["end_sentinel_rts"] == "0x00E0B8"
    assert ud["count"] == 89
    h = ud["handlers"]
    assert h["0x0024"] == "0x014848"  # Dytto projectile
    assert h["0x002C"] == "0x0254EA"  # Town Guard
    assert h["0x0034"] == "0x00EA04"  # Player Arrow
    assert h["0x003C"] == "0x017D12"  # Village Elder
    assert h["0x0048"] == "0x01778C"  # Charging Enemy

    # 2. Verify descriptor table count and Type 0x0048 / 0x002C dimensions
    dt = data["descriptor_table"]
    assert dt["address"] == "0x03F2FA"
    assert dt["count"] == 99
    e48 = dt["entities"]["0x0048"]
    assert e48["xy_radius_46"] == 8 and e48["dim_x_48"] == 8 and e48["dim_y_49"] == 4
    assert e48["update_handler"] == "0x01778C"

    # 3. Verify damage & elemental bit 13 pipeline
    dp = data["damage_pipeline"]
    assert dp["routine_addr"] == "0x017E30"
    assert dp["elemental_bit_test"] == 13
    assert dp["elemental_sfx"] == "0x8004D9"
    assert dp["damage_magnitude_mask"] == "0x0FFF"
    assert dp["defense_field"] == "$92(A6)"
    assert dp["min_damage_clamp"] == 1

    print("ALL 3 M15.264 ENTITY MASTER TABLES TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
