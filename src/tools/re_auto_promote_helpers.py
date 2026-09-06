"""Reporting and failure metadata helpers for automatic promotion."""
import json
import re
from pathlib import Path


def classify_error(text):
    if "no exact IR" in text or "UNSUPPORTED" in text:
        return "UNSUPPORTED_FORM"
    if ("illegal opcode extension" in text or "unknown mnemonic" in text or
            "immediate operand out of range" in text or "extension for unsized" in text):
        return "ASSEMBLER_SYNTAX"
    if "optimization" in text.lower():
        return "ASSEMBLER_OPTIMIZATION"
    if "BOUNDARY" in text or "incomplete selected slice" in text:
        return "BOUNDARY_UNCERTAIN"
    return "OTHER"


def clean_detail(text, output):
    return text.replace(str(output), "<output>").replace(str(Path.cwd()), "<workspace>")


def instruction_form(instruction):
    operation = instruction.get("operation", "?")
    branch_width = instruction.get("branch_width_bytes", 0)
    width = instruction.get("width_bytes", 0)
    suffix = ("s" if branch_width == 1 else "w" if branch_width == 2 else
              "b" if width == 1 else "w" if width == 2 else "l" if width == 4 else "-")
    source = instruction.get("source") or {}
    destination = instruction.get("destination") or {}
    return (f"{operation}.{suffix}:{source.get('kind', '-')}>"
            f"{destination.get('kind', '-')}")


def instruction_family(instruction):
    operation = instruction.get("operation", "?")
    branch_width = instruction.get("branch_width_bytes", 0)
    width = instruction.get("width_bytes", 0)
    suffix = ("s" if branch_width == 1 else "w" if branch_width == 2 else
              "b" if width == 1 else "w" if width == 2 else "l" if width == 4 else "-")
    return f"{operation}.{suffix}"


def forms_from_json(path):
    return sorted({instruction_form(item) for item in
                   json.loads(path.read_text()).get("instructions", [])})


def forms_from_asm(path):
    forms = set()
    for line in path.read_text().splitlines():
        match = re.match(r"\s+([a-z]+)(?:\.([bwl]))?\s+(.*)$", line, re.IGNORECASE)
        if match:
            mnemonic = match.group(1).lower()
            suffix = (match.group(2) or "-").lower()
            forms.add(f"{mnemonic}.{suffix}")
    return forms


def resolve_artifact(manifest_path, artifact):
    direct = manifest_path.parent / artifact
    if direct.exists():
        return direct
    final = manifest_path.parent / "final" / artifact
    if final.exists():
        return final
    regression = manifest_path.parent / "regression" / artifact
    if regression.exists():
        return regression
    raise FileNotFoundError(f"manifest artifact is missing: {artifact}")


def reject_form(record):
    detail = record.get("detail", "")
    match = re.search(r">\s+([^\r\n]+)", detail)
    if match:
        return match.group(1).strip()
    if record.get("reason") == "UNSUPPORTED_FORM":
        return "unsupported exact IR"
    if record.get("reason") == "BOUNDARY_UNCERTAIN":
        return "uncertain boundary"
    return record.get("reason", "OTHER")


def failure_metadata(record, rom, data_path=None):
    start = record["range"][0]
    metadata = {"raw_opcode": f"0x{int.from_bytes(rom[start:start + 2], 'big'):04X}",
                "mnemonic": "unsupported", "operand_forms": ["unknown"]}
    asm_match = re.search(r">\s+([a-z]+)(?:\.([bwl]))?\s+([^\r\n]+)",
                          record.get("detail", ""), re.IGNORECASE)
    if asm_match:
        metadata["mnemonic"] = asm_match.group(1).lower()
        metadata["operand_forms"] = [asm_match.group(3).strip()]
    unsupported = re.search(r"no exact IR at 0x([0-9A-Fa-f]+)", record.get("detail", ""))
    if unsupported:
        address = int(unsupported.group(1), 16)
        metadata["raw_opcode"] = f"0x{int.from_bytes(rom[address:address + 2], 'big'):04X}"
    if data_path and data_path.exists():
        instructions = json.loads(data_path.read_text()).get("instructions", [])
        if instructions:
            offset = (record.get("first_difference") or {}).get("rom_offset", start)
            selected = min(instructions, key=lambda item: abs(item["address"] - offset))
            metadata["mnemonic"] = selected.get("operation", "unknown")
            metadata["operand_forms"] = [instruction_form(selected)]
            metadata["raw_opcode"] = f"0x{selected.get('opcode', int(metadata['raw_opcode'], 16)):04X}"
    return metadata


def finalize_rejection(record, rom, data_path=None):
    record["failure_class"] = record["reason"]
    record.update(failure_metadata(record, rom, data_path))
    return record


def acceptance_windows(attempts, width=25):
    windows = []
    for offset in range(0, len(attempts), width):
        batch = attempts[offset:offset + width]
        accepted = sum(item["accepted"] for item in batch)
        windows.append({"start_attempt": offset + 1, "end_attempt": offset + len(batch),
                        "attempted": len(batch), "accepted": accepted,
                        "acceptance_percent": 100.0 * accepted / len(batch)})
    return windows


def reject_clusters(attempts):
    grouped = {}
    for item in attempts:
        if item["accepted"]:
            continue
        key = (item["reason"], item.get("mnemonic", "unknown"),
               tuple(item.get("operand_forms", ["unknown"])))
        cluster = grouped.setdefault(key, {"failure_class": key[0], "mnemonic": key[1],
                                           "operand_forms": list(key[2]), "count": 0,
                                           "examples": []})
        cluster["count"] += 1
        if len(cluster["examples"]) < 3:
            cluster["examples"].append(item["address"])
    return sorted(grouped.values(), key=lambda item: (-item["count"],
                                                       item["failure_class"],
                                                       item["mnemonic"]))


def legacy_mismatch_analysis(prior, attempts):
    if not prior:
        return []
    current = {item["address"]: item for item in attempts}
    result = []
    for item in prior.get("attempts", []):
        if item.get("reason") != "SLICE_MISMATCH":
            continue
        difference = item.get("first_difference") or {}
        root = ("IR_OPERAND" if difference.get("expected") == 255 and
                difference.get("actual") == 0 else "ASM_ENCODING")
        rerun = current.get(item["address"], {})
        result.append({"address": item["address"], "root_cause": root,
                       "resolved": bool(rerun.get("accepted", False))})
    return result
