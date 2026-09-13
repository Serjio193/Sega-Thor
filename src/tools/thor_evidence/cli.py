"""Small bounded V9 cycle command-line workflow."""
import argparse
from pathlib import Path

from .orchestrator import OperationalCycle


def main(argv=None):
    parser = argparse.ArgumentParser(description="run one THOR evidence cycle")
    parser.add_argument("--database", required=True)
    parser.add_argument("--capture", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--start", required=True, type=lambda value: int(value, 0))
    parser.add_argument("--end", required=True, type=lambda value: int(value, 0))
    parser.add_argument("--query-kind", required=True)
    args = parser.parse_args(argv)
    cycle = OperationalCycle(args.database)
    try:
        cycle.capture(args.capture)
        cycle.seed_frontier(args.query_kind, args.start, args.end,
                            information_gain=1, confidence=0, cost=1, risk=0,
                            evidence_classes=("STATIC",))
        cycle.next_request()
        Path(args.manifest).write_bytes(cycle.manifest_bytes())
    finally:
        cycle.close()
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
