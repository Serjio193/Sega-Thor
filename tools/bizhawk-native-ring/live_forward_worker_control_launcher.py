#!/usr/bin/env python3
"""Desktop Worker Control launcher for the W6/W3 coherent runtime build.

Launches BizHawk with 128 workers, depth 512, 512 KiB memory, native coherent ring,
and opens the separate 2H operator status window without requiring legacy
disassembler tools (--decoder, --range-tool) or Stage 5-9 prerequisites.
"""

from __future__ import annotations

import sys
from pathlib import Path

# Ensure this directory is in sys.path
RING_DIR = Path(__file__).parent.resolve()
if str(RING_DIR) not in sys.path:
    sys.path.insert(0, str(RING_DIR))

from live_forward_rom_link_runtime import build_parser, main as runtime_main


def main(argv: list[str] | None = None) -> int:
    """Launch Desktop Worker Control interactive session."""
    return runtime_main(argv)


if __name__ == "__main__":
    raise SystemExit(main())
