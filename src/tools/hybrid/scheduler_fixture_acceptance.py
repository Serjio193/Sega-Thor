"""Run and strictly compare ROM/stepped scheduler fixture cases."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys


CASES = ("A_BEFORE", "B_EXACT", "B_NEAR", "C_CROSS", "D_TAKEN",
         "D_NOT_TAKEN", "E_REFRESH", "F_REFRESH_CROSS", "G_IRQ", "H_RESUME")
COMPARE = ("events", "instructions", "writes", "final_m68k", "final_scheduler",
           "final_z80", "final_vdp", "work_ram_sha256")


def sha256(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def first_difference(left, right):
    if type(left) is not type(right):
        return {"path": "$", "rom": type(left).__name__, "step": type(right).__name__}
    if isinstance(left, dict):
        for key in sorted(set(left) | set(right)):
            if key not in left or key not in right:
                return {"path": key, "rom_present": key in left, "step_present": key in right}
            diff = first_difference(left[key], right[key])
            if diff:
                diff["path"] = f"{key}.{diff['path']}"
                return diff
        return None
    if isinstance(left, list):
        if len(left) != len(right):
            return {"path": "$", "rom_length": len(left), "step_length": len(right)}
        for index, (a, b) in enumerate(zip(left, right)):
            diff = first_difference(a, b)
            if diff:
                diff["path"] = f"[{index}].{diff['path']}"
                return diff
        return None
    return None if left == right else {"path": "$", "rom": left, "step": right}


def run_case(runner: Path, core: Path, rom: Path, name: str, mode: str, out: Path):
    result = subprocess.run([str(runner), str(core), str(rom), name, mode],
                            check=True, capture_output=True, text=True)
    data = json.loads(result.stdout)
    out.write_text(json.dumps(data, separators=(",", ":")), encoding="utf-8")
    return data


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--core", type=Path, required=True)
    parser.add_argument("--rom", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    for path in (args.runner, args.core, args.rom):
        if not path.is_file():
            raise FileNotFoundError(path)
    args.output.mkdir(parents=True, exist_ok=True)
    makefile = args.core.parent / "Makefile.libretro"
    if not makefile.is_file() or not re.search(r"^HOOK_CPU\s*=\s*1\s*$", makefile.read_text(encoding="utf-8"), re.M):
        raise RuntimeError("fixture core configuration does not prove HOOK_CPU=1")
    rom_hash = sha256(args.rom)
    cases = []
    passed = True
    for name in CASES:
        rom = run_case(args.runner, args.core, args.rom, name, "ROM",
                       args.output / f"{name.lower()}-rom.json")
        step = run_case(args.runner, args.core, args.rom, name, "STEP",
                        args.output / f"{name.lower()}-step.json")
        mismatch = None
        if rom.get("canonical_rom_sha256") != rom_hash or step.get("canonical_rom_sha256") != rom_hash:
            mismatch = {"path": "canonical_rom_sha256", "expected": rom_hash,
                        "rom": rom.get("canonical_rom_sha256"), "step": step.get("canonical_rom_sha256")}
        for field in COMPARE:
            if mismatch:
                break
            mismatch = first_difference(rom[field], step[field])
            if mismatch:
                mismatch["field"] = field
        event_kinds = {event["kind"] for event in rom["events"]}
        required = {1, 2, 3, 4, 5, 11, 12}
        if name == "E_REFRESH" or name == "F_REFRESH_CROSS":
            required.add(10)
        if name == "G_IRQ":
            required.update((6, 8, 9))
        missing = sorted(required - event_kinds)
        if not mismatch and missing:
            mismatch = {"field": "event_kinds", "missing": missing}
        case_pass = mismatch is None
        passed &= case_pass
        cases.append({"case": name, "status": "PASS" if case_pass else "FAIL",
                      "rom_events": len(rom["events"]), "step_events": len(step["events"]),
                      "rom_instructions": len(rom["instructions"]),
                      "step_instructions": len(step["instructions"]), "first_mismatch": mismatch})
    receipt = {
        "status": "SCHEDULER_FIXTURE_VERIFIED" if passed else "SCHEDULER_FIXTURE_UNVERIFIED",
        "canonical_rom_sha256": rom_hash, "core_sha256": sha256(args.core),
        "runner_sha256": sha256(args.runner), "hook_cpu": "HOOK_CPU=1",
        "comparison_fields": list(COMPARE), "cases": cases,
        "authority_deltas": 0,
    }
    (args.output / "acceptance.json").write_text(json.dumps(receipt, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(receipt, separators=(",", ":")))
    return 0 if passed else 1


if __name__ == "__main__":
    try:
        raise SystemExit(main())
    except Exception as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(2)
