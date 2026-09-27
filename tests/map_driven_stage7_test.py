"""Focused regressions for the current-generation Stage 7 adapter."""

import sys
import time
from pathlib import Path

TOOL_ROOT = Path(__file__).parents[1] / "src" / "tools"
TOOLS = TOOL_ROOT / "thor_evidence"
for path in (TOOL_ROOT, TOOLS):
    if str(path) not in sys.path:
        sys.path.insert(0, str(path))

from map_driven_asm_stage7 import (STOP_STAGE8_SUBPROCESS_TIMEOUT,
    _normalize_split_artifacts, _run)  # noqa: E402


def main() -> int:
    entries = [{"start": 0x2992, "end": 0x2AA4, "emitted_artifact_type": "blob",
                "artifact": "blobs/002992_002ADE.bin"},
               {"start": 0x2AA4, "end": 0x2ACE, "emitted_artifact_type": "asm",
                "artifact": "code/sub_002AA4.asm"},
               {"start": 0x2ACE, "end": 0x2ADE, "emitted_artifact_type": "blob",
                "artifact": "blobs/002992_002ADE.bin"}]
    normalized = _normalize_split_artifacts(entries)
    assert normalized[0]["artifact"] == "blobs/002992_002AA4.bin"
    assert normalized[1]["artifact"] == "code/sub_002AA4.asm"
    assert normalized[2]["artifact"] == "blobs/002ACE_002ADE.bin"
    assert entries[0]["artifact"] != normalized[0]["artifact"]
    beats = []
    completed = _run([sys.executable, "-c", "import time; time.sleep(0.55)"],
                      heartbeat=lambda: beats.append(time.monotonic()))
    assert completed.returncode == 0
    assert beats
    try:
        _run([sys.executable, "-c", "import time; time.sleep(2)"],
             timeout_seconds=0.2, label="timeout regression")
    except ValueError as error:
        assert str(error).startswith("STOP_STAGE7_SUBPROCESS_TIMEOUT:")
    else:
        raise AssertionError("Stage 7 timeout did not fail closed")
    try:
        _run([sys.executable, "-c", "import time; time.sleep(2)"],
             timeout_seconds=0.2, timeout_code=STOP_STAGE8_SUBPROCESS_TIMEOUT,
             label="stage8 timeout regression")
    except ValueError as error:
        assert str(error).startswith("STOP_STAGE8_SUBPROCESS_TIMEOUT:")
    else:
        raise AssertionError("Stage 8 timeout did not fail closed")
    print("Stage 7 focused regressions 4/4 passed")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
