#!/usr/bin/env python3
"""
rom_sound_command_dispatch_test.py — Milestone M15.270

Verifies extracted ROM Sound Driver Entrypoints (0x060000..0x060008) and
Command Dispatcher (0x06042A) contracts.
"""

import json
import subprocess
import sys
from pathlib import Path

EVIDENCE_PATH = Path("data/thor-evidence/rom_sound_command_dispatch_m15_270.json")
EXTRACTOR_PATH = Path("tools/extract_rom_sound_command_dispatch.py")


def main() -> int:
    res = subprocess.run([sys.executable, str(EXTRACTOR_PATH)], check=False)
    assert res.returncode == 0, f"Extractor failed with code {res.returncode}"
    assert EVIDENCE_PATH.exists(), f"Missing output {EVIDENCE_PATH}"

    data = json.loads(EVIDENCE_PATH.read_text(encoding="utf-8"))

    ep = data["entrypoints"]
    assert ep["driver_init_or_tick"] == "0x060090"
    assert ep["enqueue_command"] == "0x06042A"
    assert ep["hardware_tick"] == "0x061232"

    sys_cmds = data["system_commands_0x00_0x08"]
    assert len(sys_cmds) == 9
    assert sys_cmds["0x02"] == "0x06064C"

    cat = data["category_commands_0x0100_0x0500"]
    assert cat["0x0100_bgm"] == "0x060B50"
    assert cat["0x0200_sfx_bank2"] == "0x06112E"
    assert cat["0x0300_sfx_bank3"] == "0x060F1C"
    assert cat["0x0400_sfx_bank4"] == "0x060F54"
    assert cat["0x0500_category5"] == "0x060CDA"

    print("ALL 3 M15.270 SOUND COMMAND DISPATCH TESTS PASSED (3/3)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
