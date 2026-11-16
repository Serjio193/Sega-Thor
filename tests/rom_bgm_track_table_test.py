#!/usr/bin/env python3
"""
rom_bgm_track_table_test.py — Milestone M15.278

Verifies extracted ROM BGM Sequence Offset Table (0x0638D4) and Z80 Bus
Handshake (0x060B50).
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_bgm_track_table_m15_278.json")
EXTRACTOR_PATH = Path("tools/extract_rom_bgm_track_table.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
    assert data["table_base"] == "0x0638D4"
    assert data["valid_track_count"] >= 34
    assert data["tracks"][0]["rom_addr"] == "0x063CD4"
    assert data["z80_handshake"]["z80_stop_byte"] == "0x83"

    print("ALL 3 M15.278 BGM TRACK TABLE TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
