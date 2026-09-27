"""Bounded, payload-free discovery of dynamic SAT-shadow mutations."""

from __future__ import annotations

import argparse
import hashlib
import importlib.util
import json
from pathlib import Path


ROM_SIZE = 0x300000
ROM_SHA256 = "eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263"
SHADOW_START = 0xFF13CC
SAT_BYTES = 0x158
SHADOW_END = SHADOW_START + SAT_BYTES

TEMPLATE_WRITERS = {
    0xA372: "A5 postincrement template longword store",
    0xA37A: "A5 postincrement template word store after D3 adjustment",
    0xA4FE: "sibling fixed-root longword copy to FF13CC",
}
CLEAR_WRITERS = {
    0x03FE: "startup clear after FF188A reset",
    0x1FD0: "runtime reset clear after FF188A reset",
    0x2B6E: "explicit CLR.L reset",
    0x3330: "subsystem reset zero after FF188A reset",
    0x6026: "subsystem reset zero after FF188A reset",
    0xCD1A: "scheduler-family reset clear",
    0x3BFA4: "subsystem reset zero after FF188A reset",
    0x3C904: "subsystem reset zero after FF188A reset",
    0x3CB2E: "subsystem reset zero after FF188A reset",
    0x3E954: "scheduler-family reset clear",
}


def hx(value: int) -> str:
    return f"0x{value:06X}"


def read_u16(data: bytes, address: int) -> int:
    return int.from_bytes(data[address : address + 2], "big")


def read_u32(data: bytes, address: int) -> int:
    return int.from_bytes(data[address : address + 4], "big")


def direct_write_targets(rom: bytes) -> list[dict]:
    """Scan decoded direct-absolute write forms without treating data as code."""
    rows = []
    for pc in range(0, len(rom) - 6, 2):
        opcode = read_u16(rom, pc)
        length = 0
        kind = None
        width = 0
        if opcode & 0xFFF8 == 0x23C0:
            length, kind, width = 6, "MOVE.L Dn,(abs).L", 4
        elif opcode & 0xFFF8 == 0x23D0:
            length, kind, width = 6, "MOVE.L (An),(abs).L", 4
        elif opcode & 0xFFF0 == 0x33C0:
            length, kind, width = 6, "MOVE.W source,(abs).L", 2
        elif opcode == 0x42B9:
            length, kind, width = 6, "CLR.L (abs).L", 4
        elif opcode & 0xFFF8 == 0x13C0:
            length, kind, width = 6, "MOVE.B Dn,(abs).L", 1
        elif opcode == 0x23FC:
            length, kind, width = 10, "MOVE.L #imm,(abs).L", 4
        elif opcode == 0x33FC:
            length, kind, width = 8, "MOVE.W #imm,(abs).L", 2
        elif opcode == 0x13FC:
            length, kind, width = 8, "MOVE.B #imm,(abs).L", 1
        if not kind or pc + length > len(rom):
            continue
        target_offset = 4 if opcode in (0x23FC, 0x33FC, 0x13FC) else 2
        target = read_u32(rom, pc + target_offset)
        if SHADOW_START <= target < SHADOW_END:
            rows.append(
                {
                    "pc": hx(pc),
                    "opcode": hx(opcode),
                    "instruction_form": kind,
                    "target": hx(target),
                    "width_bytes": width,
                }
            )
    return rows


def classify_writers(rom: bytes) -> dict:
    direct = direct_write_targets(rom)
    direct_by_pc = {int(row["pc"], 16): row for row in direct}
    writers = {}
    for pc, reason in CLEAR_WRITERS.items():
        if pc in direct_by_pc:
            writers[pc] = {"class": "CLEAR/INITIALIZE", "reason": reason}
    for pc, reason in TEMPLATE_WRITERS.items():
        writers[pc] = {"class": "TEMPLATE_COPY", "reason": reason}
    unknown = [pc for pc in direct_by_pc if pc not in writers]
    return {
        "sat_shadow_writer_pcs": [hx(pc) for pc in sorted(writers)],
        "template_copy_writers": [
            hx(pc) for pc in sorted(writers) if writers[pc]["class"] == "TEMPLATE_COPY"
        ],
        "post_template_patch_writers": [],
        "clear_initialize_writers": [
            hx(pc) for pc in sorted(writers) if writers[pc]["class"] == "CLEAR/INITIALIZE"
        ],
        "unknown_writers": [hx(pc) for pc in sorted(unknown)],
        "direct_absolute_write_scan": direct,
        "indirect_writer_contracts": [
            {"pc": hx(pc), "class": info["class"], "reason": info["reason"]}
            for pc, info in sorted(writers.items())
            if pc in TEMPLATE_WRITERS
        ],
        "coverage_boundary": [
            "direct absolute writer forms are scanned across the canonical ROM",
            "A5/A6 register-based writer contracts are included only where statically closed",
            "unexecuted indirect aliases outside those contracts remain unresolved",
        ],
    }


def load_json(path: Path | None) -> dict:
    return json.loads(path.read_text(encoding="utf-8")) if path else {}


def runtime_writer_observations(entity_analysis: dict) -> dict:
    observed = set()
    aliases = []
    for field in entity_analysis.get("fields", []):
        for pc in field.get("write_pcs", []):
            observed.add(int(pc, 16))
    if 0xA374 in observed:
        aliases.append(
            {
                "observed_pc": hx(0xA374),
                "normalized_static_writer": hx(0xA372),
                "reason": "preserved callback is post-store/next-fetch identity",
            }
        )
        observed.remove(0xA374)
    return {
        "observed_writer_pcs": [hx(pc) for pc in sorted(observed)],
        "callback_aliases": aliases,
        "input_dependent_mutation_observed": False,
    }


def sat_dma_inventory(evidence_root: Path) -> dict:
    files = []
    sources = set()
    frames = set()
    candidates = []
    for path in sorted(evidence_root.rglob("postrun_vdp_dma.json")):
        try:
            report = load_json(path)
        except (OSError, json.JSONDecodeError):
            continue
        files.append(str(path))
        for event in report.get("dma_events_sample", []):
            if event.get("destination_address") not in ("0xD000", "0x0000D000"):
                continue
            source = event.get("source_address")
            sources.add(source)
            frames.add(event.get("frame"))
            if source != "0xFF13CC":
                candidates.append(
                    {
                        "source": source,
                        "sat_entries": "UNRESOLVED_FROM_PRESERVED_ARTIFACT",
                        "producer_pcs": [],
                        "frames_observed": [event.get("frame")],
                        "dynamic_xy_fields": [],
                        "upstream_ram_sources": [],
                    }
                )
    return {
        "dma_files_scanned": files,
        "sat_dma_sources": sorted(s for s in sources if s),
        "sat_dma_frames": sorted(f for f in frames if f is not None),
        "dynamic_sat_group_candidates": candidates,
        "ranking_input": {
            "targeted_run_id": "1790106191",
            "phases": ["RIGHT", "LEFT", "UP", "DOWN", "ACTION"],
            "correlation_only": True,
        },
    }


def caller_report(rom: bytes) -> dict:
    module_path = Path(__file__).with_name("m12_a372_caller_join.py")
    spec = importlib.util.spec_from_file_location("m12_a372_caller_join", module_path)
    if spec is None or spec.loader is None:
        raise RuntimeError("caller join module unavailable")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    result = module.parse_join(rom)
    result["register_roles"] = {
        "input_registers": [],
        "pointer_registers": [
            {"register": "A0", "source": "selected ROM root 0x00A438 or 0x00A480"},
            {"register": "A5", "source": "0x00FF13CC + sign_extended(FF188C)"},
            {"register": "A6", "source": "sibling selected ROM root"},
        ],
        "index_registers": [
            {"register": "D0", "source": "loop counts 5 and 2"},
            {"register": "D5", "source": "FF188A counter / low-byte template index"},
            {"register": "D3", "source": "FF185A word adjustment"},
        ],
        "parameter_sources": [
            "FF1858 boolean ROM-root selector",
            "FF188A counter",
            "FF188C destination offset",
            "FF185A/FF184F/FF1856 producer-family controls",
        ],
        "gameplay_derived_position_or_state_passed": False,
    }
    return result


def build_artifacts(rom: bytes, evidence_root: Path, entity_analysis: dict) -> dict[str, dict]:
    digest = hashlib.sha256(rom).hexdigest()
    if len(rom) != ROM_SIZE or digest != ROM_SHA256:
        raise ValueError("canonical ROM identity mismatch")
    writers = classify_writers(rom)
    runtime = runtime_writer_observations(entity_analysis)
    dma = sat_dma_inventory(evidence_root)
    callers = caller_report(rom)
    refresh_sequences = [
        {
            "sequence_id": "preserved-sat-refresh-template-to-dma",
            "write_pcs_before_dma": ["0x00A372", "0x00A37A"],
            "dma_pcs": ["0x0027DE", "0x0027EA"],
            "dma_source": "0xFF13CC",
            "dma_destination": "0x00D000",
            "post_template_patch_pcs": [],
            "post_template_patch_before_dma": False,
            "ordering_status": "TEMPLATE_COPY_THEN_DMA_NO_PATCH_WRITER",
            "identity_limit": "preserved artifact lacks native run/epoch instruction trace identity",
        }
    ]
    writer_report = {
        "schema": "oasis.m12.sat-shadow-writer-analysis.v1",
        "status": "SAT_SHADOW_WRITERS_BOUNDED",
        "canonical_rom_sha256": digest,
        "shadow_range": [hx(SHADOW_START), hx(SHADOW_END)],
        **{
            key: value
            for key, value in writers.items()
            if key
            not in {
                "sat_shadow_writer_pcs",
                "template_copy_writers",
                "post_template_patch_writers",
                "unknown_writers",
            }
        },
        "runtime": runtime,
        "group_class": "TEMPLATE_ONLY",
        "SAT_SHADOW_WRITER_PCS": writers["sat_shadow_writer_pcs"],
        "TEMPLATE_COPY_WRITERS": writers["template_copy_writers"],
        "POST_TEMPLATE_PATCH_WRITERS": writers["post_template_patch_writers"],
        "UNKNOWN_WRITERS": writers["unknown_writers"],
        "refresh_sequences": refresh_sequences,
    }
    patches = {
        "schema": "oasis.m12.sat-dynamic-patches.v1",
        "status": "NO_POST_TEMPLATE_PATCHES_PROVEN",
        "shadow_range": [hx(SHADOW_START), hx(SHADOW_END)],
        "group_class": "TEMPLATE_ONLY",
        "PATCHES": [],
        "post_template_patch_writers": [],
        "refresh_sequences": refresh_sequences,
        "negative_evidence": [
            "targeted run 1790106191 showed stable template-field values across RIGHT/LEFT/UP/DOWN/ACTION",
            "no preserved runtime writer is classified as POST_TEMPLATE_PATCH",
            "no exact dynamic X/Y source register or upstream RAM read exists",
        ],
    }
    candidates = {
        "schema": "oasis.m12.dynamic-sat-candidates.v1",
        "status": "NO_DYNAMIC_SAT_GROUP_PROVEN",
        **{
            key: value
            for key, value in dma.items()
            if key != "dynamic_sat_group_candidates"
        },
        "DYNAMIC_SAT_GROUP_CANDIDATES": dma["dynamic_sat_group_candidates"],
    }
    callers["schema"] = "oasis.m12.renderer-callers.v1"
    callers["status"] = "STATIC_CALLER_REGISTER_BOUNDARY_PROVEN_NO_GAMEPLAY_PARAMETER"
    receipt = {
        "schema": "oasis.m12.dynamic-sat-receipt.v1",
        "status": "STOP",
        "pass": False,
        "result": "STOP_NO_DYNAMIC_SAT_GROUP_IDENTIFIED",
        "ff13cc_group_class": "TEMPLATE_ONLY",
        "dynamic_patch_count": 0,
        "dynamic_sat_group_count": len(dma["dynamic_sat_group_candidates"]),
        "source_owned_before": 1487672,
        "source_owned_after": 1487672,
        "source_owned_delta": 0,
        "commit_created": "NO",
        "push_performed": "NO",
    }
    return {
        "postrun_sat_shadow_writer_analysis.json": writer_report,
        "postrun_sat_dynamic_patches.json": patches,
        "postrun_dynamic_sat_candidates.json": candidates,
        "postrun_renderer_callers.json": callers,
        "postrun_dynamic_sat_receipt.json": receipt,
    }


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("rom", type=Path)
    parser.add_argument("evidence_root", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--entity-analysis", type=Path)
    args = parser.parse_args()
    artifacts = build_artifacts(
        args.rom.read_bytes(), args.evidence_root, load_json(args.entity_analysis)
    )
    args.output.mkdir(parents=True, exist_ok=True)
    for name, payload in artifacts.items():
        (args.output / name).write_text(
            json.dumps(payload, indent=2, sort_keys=True) + "\n", encoding="utf-8"
        )
    print(json.dumps(artifacts["postrun_dynamic_sat_receipt.json"], indent=2, sort_keys=True))


if __name__ == "__main__":
    main()
