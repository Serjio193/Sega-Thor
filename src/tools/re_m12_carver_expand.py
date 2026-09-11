"""Run the deterministic graph-guided M12 Carver evidence expansion."""

import argparse
import json
import sys
from pathlib import Path

HERE = Path(__file__).resolve().parent
if str(HERE) not in sys.path:
    sys.path.insert(0, str(HERE))

from m12_carver_expansion import run


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--rom", required=True)
    parser.add_argument("--evidence", action="append", default=[])
    parser.add_argument("--census", action="append", required=True)
    parser.add_argument("--output", required=True)
    args = parser.parse_args()
    report = run(args.manifest, args.rom, args.evidence, args.census, args.output)
    print(json.dumps({
        "output": str(Path(args.output).resolve()),
        "source_owned_bytes": report["final_source_owned_bytes"],
        "gained_source_owned_bytes": report["gained_source_owned_bytes"],
        "census_matches": report["graphics_decoder_closure"]["census_matches"],
        "raw_size_hypothesis_bytes": report["raw_size_hypotheses"]["bytes"],
        "fixed_point": report["fixed_point"],
        "stop_reason": report["stop_reason"],
    }, sort_keys=True))


if __name__ == "__main__":
    main()
