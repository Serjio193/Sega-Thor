#!/usr/bin/env python3
"""
rom_item_attribute_tables_test.py — Milestone M15.272

Verifies extracted 64-byte ROM Item Attribute Table (0x05CE16) and
Pickup Fanfare Tier Table (0x05CE56).
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_item_attribute_tables_m15_272.json")
EXTRACTOR_PATH = Path("tools/extract_rom_item_attribute_tables.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
    items = data["items"]
    assert len(items) == 64

    # Knife (0x01) is non-discardable (0xA1); Swords/Bows (0x02..0x0C) are 0x01
    assert items[0x01]["attr_byte"] == "0xA1" and items[0x01]["non_discardable"] is True
    assert items[0x05]["attr_byte"] == "0x01" and items[0x05]["non_discardable"] is False

    # Consumables (0x1E..0x2A) are discardable (0x00) and pickup tier 2
    assert items[0x1E]["attr_byte"] == "0x00" and items[0x1E]["pickup_tier"] == 2
    assert items[0x12]["pickup_tier"] == 4  # Major treasure fanfare 0x050D

    print("ALL 3 M15.272 ITEM ATTRIBUTE TABLE TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
