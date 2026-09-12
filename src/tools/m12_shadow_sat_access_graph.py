"""Enumerate direct absolute accesses to the three M12 shadow fields.

The input is the existing payload-free GPGX decoded-instruction census.  This
tool deliberately does not infer register aliases or function names: those
are reported as an explicit closure boundary.
"""
import argparse
import hashlib
import json
import re
from pathlib import Path


TARGETS = ("FF1858", "FF188A", "FF188C")
OPERAND = re.compile(r"\(\$00FF(1858|188A|188C)\)\.L", re.IGNORECASE)


def operation(decoded: str, target: str) -> tuple[str, str, int, str]:
    text = decoded.upper()
    width = 1 if ".B" in text else 2 if ".W" in text else 4 if ".L" in text else 0
    if text.startswith("TST.") or text.startswith("ADDA."):
        return "reader", "read", width, "none"
    if text.startswith("MOVE."):
        left, right = text.split(",", 1)
        if target in left:
            return "reader", "copy", width, "none"
        return "writer", "copy", width, "overlaps FF188C" if target == "FF188A" and width == 4 else "none"
    if text.startswith("CLR.") or text.startswith("SF."):
        return "writer", "clear", width, "overlaps FF188C" if target == "FF188A" and width == 4 else "none"
    if text.startswith("ST."):
        return "writer", "set", width, "none"
    if text.startswith("ADDQ."):
        return "writer", "increment", width, "none"
    if text.startswith("ADDI."):
        return "writer", "add", width, "none"
    return "unknown", "unknown", width, "unclassified"


def build_graph(census: dict, census_sha256: str | None = None) -> dict:
    if census.get("instruction_count") != len(census.get("instructions", [])):
        raise ValueError("instruction_count does not match instructions")
    accesses = {target: [] for target in TARGETS}
    for item in census["instructions"]:
        decoded = item.get("decoded_instruction", "")
        if item.get("status") != "DECODED":
            continue
        for match in OPERAND.finditer(decoded):
            target = "FF" + match.group(1).upper()
            direction, kind, width, alias = operation(decoded, target)
            accesses[target].append({
                "pc": item["address"].upper(),
                "raw_bytes": item["raw_bytes"].upper(),
                "instruction": decoded,
                "direction": direction,
                "operation": kind,
                "width_bytes": width,
                "alias": alias,
                "classification": item.get("classification", "UNKNOWN"),
            })
    return {
        "schema": "oasis.m68k.m12-shadow-sat-access-graph.v1",
        "source": {
            "kind": "decoded_instruction_census",
            "instruction_count": census["instruction_count"],
            "sha256": census_sha256,
            "decoded_only": True,
        },
        "targets": {
            target: {
                "direct_access_count": len(accesses[target]),
                "readers": sum(a["direction"] == "reader" for a in accesses[target]),
                "writers": sum(a["direction"] == "writer" for a in accesses[target]),
                "accesses": accesses[target],
            }
            for target in TARGETS
        },
        "closure_boundary": [
            "direct absolute operands in DECODED census are closed",
            "register-based aliases and indirect pointer accesses are unresolved",
            "unexecuted or DECODE_UNSUPPORTED bytes are outside this census",
        ],
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("census", type=Path)
    parser.add_argument("output", type=Path)
    args = parser.parse_args()
    raw = args.census.read_bytes()
    graph = build_graph(json.loads(raw), hashlib.sha256(raw).hexdigest())
    args.output.write_text(json.dumps(graph, indent=2) + "\n", encoding="utf-8")
    for target, result in graph["targets"].items():
        print(target, result["direct_access_count"], "direct accesses",
              result["readers"], "readers", result["writers"], "writers")


if __name__ == "__main__":
    main()
