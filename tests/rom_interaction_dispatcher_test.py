#!/usr/bin/env python3
"""
rom_interaction_dispatcher_test.py — Milestone M15.275

Verifies extracted ROM Universal Interaction Dispatcher (0x008330) and
Story NPC Dialogue Handler (0x018EBE) contracts.
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_interaction_dispatcher_m15_275.json")
EXTRACTOR_PATH = Path("tools/extract_rom_interaction_dispatcher.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))
    routes = data["routes"]
    assert routes["0x0032"]["target"] == "0x018BBC"
    assert routes["0x0020"]["target"] == "0x018CAA"
    assert routes["0x0094"]["target"] == "0x018EBE"
    assert routes["0x00B0"]["sfx"] == "0xA00264"

    print("ALL 3 M15.275 INTERACTION DISPATCHER TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
