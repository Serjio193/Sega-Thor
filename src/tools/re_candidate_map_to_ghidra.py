"""Rebuild the minimal Ghidra function view from the preserved candidate census."""
import argparse
import json
from pathlib import Path


def rebuild(candidate_map):
    functions = []
    for item in candidate_map["candidates"]:
        span = item.get("ghidra_range") or {}
        if not item.get("ghidra_function") or not span.get("start"):
            continue
        functions.append({
            "entry": item["entry"],
            "name": "GHIDRA_RECOGNIZED_FUNCTION",
            "range": f"{span['start']}..{span['end']}",
            "source": "PRESERVED_CANDIDATE_MAP_GHIDRA_RANGE",
        })
    functions.sort(key=lambda item: int(item["entry"], 0))
    return {
        "schema": "oasis.m68k.ghidra-map.v1",
        "metadata": {
            "program_name": candidate_map["ghidra"].get("program", "BeyondOasisUSA.bin"),
            "language_id": candidate_map["ghidra"].get("language", "68000:BE:32:default"),
            "semantic_status": "GHIDRA_CANDIDATE_ONLY_RECONSTRUCTED",
        },
        "functions": functions,
        "source": "candidate-map-b.json; ranges only, no new decompiler claims",
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("candidate_map")
    parser.add_argument("output")
    args = parser.parse_args()
    source = json.loads(Path(args.candidate_map).read_text())
    Path(args.output).write_text(json.dumps(rebuild(source), indent=2) + "\n")
    print(f"reconstructed_functions={len(rebuild(source)['functions'])}")


if __name__ == "__main__":
    main()
