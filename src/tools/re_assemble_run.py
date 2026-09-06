"""Local-only vasm runner. Paths are supplied by the developer, never machine defaults."""
import argparse
from collections import Counter
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


def instruction_form(instruction):
    width = instruction.get("width_bytes", 0)
    operation = instruction["operation"]
    source = instruction.get("source")
    destination = instruction.get("destination")
    source_kind = source["kind"] if source else "-"
    destination_kind = destination["kind"] if destination else "-"
    return f"{operation}.{width}:{source_kind}->{destination_kind}"


def emit_legacy_split(output, manifest, original, assembler, flags, tool, rom):
    """Rebuild the M11.9 five-routine mixed split as a regression check."""
    legacy_starts = [0x1108, 0x2B6E, 0x3820, 0x62CC, 0xA8DA]
    entries = {entry["start"]: entry for entry in manifest["routines"]}
    legacy = output / "legacy"
    (legacy / "blobs").mkdir(parents=True)
    main_lines = ["; M11.9 mixed split regression", "    org $001108"]
    for index, start in enumerate(legacy_starts):
        entry = entries[start]
        body = (output / entry["asm"]).read_text().splitlines()
        main_lines.extend(line for line in body
                          if not line.startswith("    org ") and not line.startswith("sub_"))
        if index + 1 < len(legacy_starts):
            end = entry["end"]
            next_start = legacy_starts[index + 1]
            blob_name = f"{end:06X}.bin"
            (legacy / "blobs" / blob_name).write_bytes(original[end:next_start])
            main_lines.extend([f"data_{end:06X}:", f'    incbin "legacy/blobs/{blob_name}"'])
    # vasm resolves a source named main.asm from the working directory even
    # when a relative subdirectory path is supplied; use a unique basename.
    source = legacy / "legacy_layout.asm"
    source.write_text("\n".join(main_lines) + "\n")
    binary = legacy / "layout.bin"
    assembled = run([assembler, *flags, "-o", binary, source], output, False)
    if assembled.returncode:
        return assembled
    return run([tool, "verify", rom, binary, "0x1108", "0xA8F0"], check=False)


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
    forms = Counter()
    for routine in manifest["routines"]:
        data = json.loads((output / routine["asm"]).with_suffix(".json").read_text())
        forms.update(instruction_form(instruction) for instruction in data["instructions"])
    legacy = emit_legacy_split(output, manifest, original, assembler, flags, tool, rom)
    report = {"routines": results, "exact_matches": matched, "verified_asm_bytes": verified,
              "total_selected_bytes": total, "round_trip_percent": 100 * verified / total,
              "instruction_count": sum(r["instruction_count"] for r in results),
              "total_observed_form_count": len(forms),
              "supported_exact_form_count": len(forms) if matched == len(results) else 0,
              "form_coverage_percent": 100 * len(forms) / len(forms) if forms and matched == len(results) else 0,
              "observed_forms": [{"form": form, "occurrences": count}
                                 for form, count in sorted(forms.items())],
              "selected_unsupported_forms": 0, "assembler_workaround_classes": 1,
              "assembler_banner": run([assembler], check=False).stdout.strip(),
              "flags": flags, "handwritten_overrides": 0,
              "split_status": "MATCH" if split.returncode == 0 else "FAILED",
              "split_detail": split.stdout + split.stderr,
              "legacy_split_status": "MATCH" if legacy.returncode == 0 else "FAILED",
              "legacy_split_detail": legacy.stdout + legacy.stderr}
    (output / "result.json").write_text(json.dumps(report, indent=2) + "\n")
    routine_count = len(results)
    print(f"Verified ASM: {verified}/{total} bytes; {matched}/{routine_count} routines; split={report['split_status']}")
    return 0 if matched == routine_count and split.returncode == 0 and legacy.returncode == 0 else 1


if __name__ == "__main__":
    raise SystemExit(main())
