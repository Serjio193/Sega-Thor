"""CLI for the deterministic whole-ROM M12 Carver runtime sweep."""

import argparse
import json
from pathlib import Path

from m12_carver_global_sweep import run


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--manifest", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--input-root", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    result = run(args.manifest, args.rom, args.input_root, args.output)
    print(json.dumps({"output": str(args.output.resolve()),
                      "source_owned_bytes": result["final_source_owned_bytes"],
                      "observed_rom_bytes": result["runtime"]["total_rom_bytes_observed"],
                      "unknown_ranges": len(result["blocker_census"]["ranges"]),
                      "fixed_point": result["fixed_point"]}, sort_keys=True))


if __name__ == "__main__":
    main()
