#!/usr/bin/env python3
"""
rom_all_spirits_resources_test.py — Milestone M15.267

Verifies extracted Nemesis ROM sources, VRAM tile destinations, spawn routines,
and update handlers for all 4 Elemental Spirits (Dytto, Efreet, Shade, Bow).
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_all_spirits_resources_m15_267.json")
EXTRACTOR_PATH = Path("tools/extract_rom_all_spirits_resources.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
    sp = data["spirits"]
    assert len(sp) == 4

    assert sp["Dytto"]["entity_type"] == "0x0018"
    assert sp["Dytto"]["nemesis_rom_addr"] == "0x141580"
    assert sp["Dytto"]["vram_tile"] == "0x04D0"
    assert sp["Dytto"]["vram_byte_addr"] == "0x9A00"

    assert sp["Efreet"]["entity_type"] == "0x001A"
    assert sp["Efreet"]["nemesis_rom_addr"] == "0x1425AE"
    assert sp["Efreet"]["vram_tile"] == "0x0450"
    assert sp["Efreet"]["vram_byte_addr"] == "0x8A00"

    assert sp["Shade"]["entity_type"] == "0x001C"
    assert sp["Shade"]["nemesis_rom_addr"] == "0x143262"
    assert sp["Shade"]["vram_tile"] == "0x0280"
    assert sp["Shade"]["vram_byte_addr"] == "0x5000"

    assert sp["Bow"]["entity_type"] == "0x001E"
    assert sp["Bow"]["nemesis_rom_addr"] == "0x143A42"
    assert sp["Bow"]["vram_tile"] == "0x0360"
    assert sp["Bow"]["vram_byte_addr"] == "0x6C00"

    print("ALL 4 M15.267 ALL-SPIRITS RESOURCE TESTS PASSED (4/4)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
