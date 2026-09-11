"""CLI for the deterministic M12 whole-ROM format/container pass."""

import argparse
import json
from pathlib import Path

from m12_carver_format_reconstruction import run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--static-report", type=Path)
    args = parser.parse_args()
    report = run(args.manifest, args.rom, args.output, args.static_report)
    print(json.dumps({"output": str(args.output.resolve()),
                      "source_owned_bytes": report["final"]["source_owned_bytes"],
                      "unknown_bytes": report["final"]["unknown_bytes"],
                      "format_candidates": report["formats"]["candidate_count"],
                      "fixed_point": report["carver"]["fixed_point"]}, sort_keys=True))


if __name__ == "__main__":
    main()
