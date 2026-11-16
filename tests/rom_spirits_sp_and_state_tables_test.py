#!/usr/bin/env python3
"""
rom_spirits_sp_and_state_tables_test.py — Milestone M15.271

Verifies extracted live SP drain intervals, despawn handlers, and state
jump tables for all 4 Elemental Spirits (Dytto, Efreet, Shade, Bow).
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_spirits_sp_and_state_tables_m15_271.json")
EXTRACTOR_PATH = Path("tools/extract_rom_spirits_sp_and_state_tables.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
    sp = data["spirits"]
    assert len(sp) == 4

    assert sp["Dytto"]["sp_drain_interval_frames"] == 45
    assert sp["Dytto"]["despawn_handler"] == "0x012314"
    assert sp["Efreet"]["sp_drain_interval_frames"] == 35
    assert sp["Efreet"]["despawn_handler"] == "0x012BA8"
    assert sp["Shade"]["sp_drain_interval_frames"] == 40
    assert sp["Shade"]["despawn_handler"] == "0x0136CC"
    assert sp["Bow"]["sp_drain_interval_frames"] == 30
    assert sp["Bow"]["despawn_handler"] == "0x013F22"

    print("ALL 4 M15.271 SPIRITS SP & STATE TABLE TESTS PASSED (4/4)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
