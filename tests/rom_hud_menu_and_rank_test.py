#!/usr/bin/env python3
"""
rom_hud_menu_and_rank_test.py — Milestone M15.268

Verifies extracted ROM Pause Ring Menu Icon Renderer (0x0042E8, 0x00155D76)
and Player Rank / MaxHP Scaling Formula (0x007BF6).
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_hud_menu_and_rank_m15_268.json")
EXTRACTOR_PATH = Path("tools/extract_rom_hud_menu_and_rank.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    rm = data["ring_menu"]
    assert rm["icon_tileset_rom_addr"] == "0x155D76"
    assert rm["tiles_per_icon"] == 9
    assert rm["weapon_slot_count"] == 8
    assert rm["item_slot_count"] == 16

    rs = data["rank_and_stats"]
    assert rs["max_rank"] == 200
    assert rs["base_max_hp"] == 198
    assert rs["sp_pickup_step"] == 5

    print("ALL 3 M15.268 HUD MENU & RANK TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
