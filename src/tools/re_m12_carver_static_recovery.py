"""CLI for the deterministic M12 Carver static consumer recovery pass."""

import argparse
import json
from pathlib import Path

from m12_carver_static_recovery import run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--input-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    report = run(args.manifest, args.rom, args.input_root, args.output)
    print(json.dumps({"output": str(args.output.resolve()),
                      "source_owned_bytes": report["final"]["source_owned_bytes"],
                      "references": report["reference_census"]["total"],
                      "unknown_ranges": len(report["remaining_unknown"]["ranges"]),
                      "fixed_point": report["fixed_point"]}, sort_keys=True))


if __name__ == "__main__":
    main()
