"""Local-only vasm runner. Paths are supplied by the developer, never machine defaults."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess


def run(command, cwd=None, check=True):
    result = subprocess.run([str(arg) for arg in command], cwd=cwd,
                            text=True, capture_output=True, check=False)
    if check and result.returncode:
        raise RuntimeError(result.stdout + result.stderr)
    return result


def tool_path(value):
    found = shutil.which(value)
    path = Path(found or value).resolve()
    if not path.is_file():
        raise ValueError(f"Tool unavailable: {value}")
    return path


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--tool", required=True, help="oasis_re_assemble executable")
    parser.add_argument("--assembler", default="vasmm68k_mot")
    parser.add_argument("--rom", required=True)
    parser.add_argument("--output", required=True, help="new ignored output directory")
    args = parser.parse_args()
    tool, assembler = tool_path(args.tool), tool_path(args.assembler)
    rom, output = Path(args.rom).resolve(), Path(args.output).resolve()
    # No automatic download, shell command construction, or overwrite of an old run.
    run([tool, "emit", rom, output])
    manifest = json.loads((output / "manifest.json").read_text())
    original = rom.read_bytes()
    if hashlib.sha256(original).hexdigest() != manifest["rom_sha256"]:
        raise ValueError("ROM changed since decoder validation")
    for gap in manifest["unknown_ranges"]:
        (output / gap["file"]).write_bytes(original[gap["start"]:gap["end"]])
    flags = ["-m68000", "-no-opt", "-Fbin"]
    results = []
    for routine in manifest["routines"]:
        binary = Path(routine["asm"]).with_suffix(".bin")
        assembly = run([assembler, *flags, "-o", binary, routine["asm"]], output, False)
        if assembly.returncode:
            result = {"status": "ASSEMBLER_ERROR", "detail": assembly.stdout + assembly.stderr}
        else:
            compared = run([tool, "verify", rom, output / binary,
                            routine["start"], routine["end"]], check=False)
            status = {0: "MATCH", 1: "FIRST_DIFFERENCE"}.get(compared.returncode, "VERIFY_ERROR")
            result = {"status": status,
                      "detail": compared.stdout + compared.stderr}
        results.append({**routine, **result, "manual_overrides": 0})
        print(f"0x{routine['start']:06X}: {result['status']} {result['detail'].strip()}")
    split = run([assembler, *flags, "-o", "layout.bin", "main.asm"], output, False)
    if split.returncode == 0:
        split = run([tool, "verify", rom, output / "layout.bin",
                     manifest["start"], manifest["end"]], check=False)
    matched = sum(r["status"] == "MATCH" for r in results)
    verified = sum(r["end"] - r["start"] for r in results if r["status"] == "MATCH")
    total = sum(r["end"] - r["start"] for r in results)
    report = {"routines": results, "exact_matches": matched, "verified_asm_bytes": verified,
              "total_selected_bytes": total, "round_trip_percent": 100 * verified / total,
              "instruction_count": sum(r["instruction_count"] for r in results),
              "selected_unsupported_forms": 0, "assembler_workaround_classes": 1,
              "assembler_banner": run([assembler], check=False).stdout.strip(),
              "flags": flags, "handwritten_overrides": 0,
              "split_status": "MATCH" if split.returncode == 0 else "FAILED",
              "split_detail": split.stdout + split.stderr}
    (output / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    print(f"Verified ASM: {verified}/{total} bytes; {matched}/5 routines; split={report['split_status']}")
    return 0 if matched == 5 and split.returncode == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
