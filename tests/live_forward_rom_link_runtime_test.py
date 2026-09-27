"""Regression checks for interactive runtime terminal gating."""

from __future__ import annotations

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "tools" / "bizhawk-native-ring"))

from live_forward_rom_link_runtime import should_launch_postrun_analysis  # noqa: E402


def main() -> None:
    for status in ("STOPPED_AFTER_EMUHAWK_EXIT", "STOPPED_DISK_RESERVE", "STOPPED_END_GAME"):
        assert should_launch_postrun_analysis(status), status
    for status in ("RESOURCE_PREFLIGHT_REJECTED", "NATIVE_ALLOCATION_REJECTED",
                   "HOST_TRANSPORT_PREFLIGHT_REJECTED", "UNKNOWN"):
        assert not should_launch_postrun_analysis(status), status
    print("live_forward_rom_link_runtime_test: PASS")


if __name__ == "__main__":
    main()
