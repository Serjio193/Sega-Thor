"""Machine-readable MAP-1 importers; no runtime capture or truth tables."""

from __future__ import annotations

import hashlib
import json
from pathlib import Path
from typing import Any

from .cartographer import digest


def _hexkey(value: str) -> str:
    try:
        return f"0x{int(value, 0):X}"
    except (TypeError, ValueError):
        return value


def _read(path: Path) -> tuple[dict[str, Any], str]:
    raw = path.read_bytes()
    return json.loads(raw.decode("utf-8")), hashlib.sha256(raw).hexdigest()


def _ref(repo: Path, path: Path) -> dict[str, str]:
    return {"source": str(path.relative_to(repo)).replace("\\", "/"), "sha256": hashlib.sha256(path.read_bytes()).hexdigest()}


def _node(kind: str, key: str, attributes: dict[str, Any], lineage: list[Any], status: str = "PROVEN", scope: str = "global") -> dict[str, Any]:
    return {"kind": kind, "key": key, "scope": scope, "status": status, "attributes": attributes, "lineage": lineage}


def _nid(node: dict[str, Any]) -> str:
    return digest({"kind": node["kind"], "key": node["key"], "scope": node.get("scope", "global")})


def _edge(source: dict[str, Any], target: dict[str, Any], relation: str, lineage: list[Any], rule: str) -> dict[str, Any]:
    return {"source": _nid(source), "target": _nid(target), "relation": relation, "status": "PROVEN",
            "rule": rule, "lineage": lineage}


def _bundle(ref: dict[str, str]) -> dict[str, Any]:
    return {"nodes": [], "edges": [], "frontiers": [], "resolves_frontiers": [], "source": ref}


def load_cpu_r3b(repo: Path) -> tuple[str, str, dict[str, Any]]:
    path = repo / "docs/reports/THOR_M12_AUTO67_6R3B_TARGETED_BURST_FEASIBILITY.json"
    data, sha = _read(path)
    if data.get("result") != "PASS" or data["canary"].get("join_status") != "EXACT":
        raise ValueError("CPU/register source is not an accepted exact proof")
    ref = _ref(repo, path)
    bundle = _bundle(ref)
    seed = data["canary"]["seed"]
    consumer = _node("ROM_INSTRUCTION", _hexkey(seed["consumer_pc"]), {"pc": _hexkey(seed["consumer_pc"]), "kind": seed["kind"], "address": seed["address"]}, [ref])
    bundle["nodes"].append(consumer)
    epoch = data["canary"]["epoch"]
    for step in data["canary"]["steps"]:
        producer = _node("ROM_INSTRUCTION", _hexkey(step["producer_pc"]), {"pc": _hexkey(step["producer_pc"]), "opcode": step["producer_opcode"], "semantics": step["producer_semantics"]}, [ref])
        version = _node("REGISTER_VERSION", f"{step['register']}:epoch={epoch}:sequence={step['producer_sequence']}",
                        {"register": step["register"], "epoch": epoch, "sequence": step["producer_sequence"],
                         "value": step["consumer_register_value"], "producer_pc": step["producer_pc"]}, [ref], scope=f"epoch:{epoch}")
        bundle["nodes"] += [producer, version]
        bundle["edges"] += [_edge(producer, version, "PRODUCES", [ref], "exact captured reaching-definition step"),
                             _edge(version, consumer, "VALUE_DEPENDENCY", [ref], "exact captured register value at consumer")]
    return "cpu-register-r3b", sha, bundle


def load_sat_dma(repo: Path) -> tuple[str, str, dict[str, Any]]:
    path = repo / "build/m12-gfx-runtime/sat-provenance-current-v5.json"
    data, sha = _read(path)
    ref = _ref(repo, path)
    bundle = _bundle(ref)
    unique: dict[tuple[Any, ...], dict[str, Any]] = {}
    for launch in sorted(data.get("sat_dma_launches", []), key=lambda item: (item.get("frame", 0), item.get("pc", ""))):
        key = (launch.get("pc"), launch.get("source_address"), launch.get("destination"), launch.get("length_words"))
        unique.setdefault(key, launch)
    for launch in list(unique.values())[:3]:
        pc = launch["pc"]
        src = launch["source_address"]
        dst = launch["destination"]
        dma = _node("DMA", f"frame={launch['frame']}:pc={_hexkey(pc)}:src={_hexkey(src)}:dst={_hexkey(dst)}", launch, [ref], scope=f"frame:{launch['frame']}")
        source = _node("RAM_VALUE_VERSION", f"frame={launch['frame']}:{_hexkey(src)}", {"address": src, "source_bytes": launch.get("source_bytes", [])}, [ref], scope=f"frame:{launch['frame']}")
        vram = _node("VRAM_RANGE", f"frame={launch['frame']}:{_hexkey(dst)}", {"address": dst, "length_words": launch["length_words"]}, [ref], scope=f"frame:{launch['frame']}")
        sink = _node("VDP_HARDWARE_SINK", f"SAT:{_hexkey(dst)}", {"destination": dst, "control_value": launch["control_value"]}, [ref])
        instruction = _node("ROM_INSTRUCTION", _hexkey(pc), {"pc": _hexkey(pc), "runtime": "sat_dma_launch"}, [ref])
        bundle["nodes"] += [dma, source, vram, sink, instruction]
        bundle["edges"] += [_edge(instruction, dma, "LAUNCHES", [ref], "runtime SAT/DMA launch record"),
                             _edge(source, dma, "READS_FROM", [ref], "runtime source address"),
                             _edge(dma, vram, "DMA_TO", [ref], "runtime destination and length"),
                             _edge(dma, sink, "WRITES_TO", [ref], "runtime VDP/SAT sink")]
    return "sat-dma-v5", sha, bundle


def load_graphics(repo: Path) -> tuple[str, str, dict[str, Any]]:
    manifest_path = repo / "build/m12-auto60-contiguous-islands-a/materialized/manifest.json"
    program_path = repo / "build/thor-evidence/auto64-static/program.json"
    manifest, manifest_sha = _read(manifest_path)
    program, program_sha = _read(program_path)
    ref = {"source": "manifest+auto64-static-program", "manifest_sha256": manifest_sha, "program_sha256": program_sha}
    bundle = _bundle(ref)
    functions = [item for item in program["functions"] if item.get("boundary") == "confirmed"]
    decompressor = next((item for item in functions if item.get("entry_point") == "0x00003820"), None)
    if decompressor is None:
        raise ValueError("confirmed decompressor evidence is absent")
    decoder = _node("DECOMPRESSOR", decompressor["entry_point"], decompressor, [ref])
    bundle["nodes"].append(decoder)
    selected = [item for item in manifest["entries"] if item.get("classification") in {"GRAPHICS_COMPRESSED_STREAM", "LOCAL_ROM_DERIVED_ASSET"} and "decompressor" in item.get("ownership_reason", "").lower()]
    for item in selected[:5]:
        stream = _node("ROM_RANGE", f"{item['start']}:{item['end']}", item, [ref])
        bundle["nodes"].append(stream)
        bundle["edges"].append(_edge(decoder, stream, "DECOMPRESSES", [ref], "manifest exact stream plus confirmed static decoder"))
    return "graphics-manifest-program", digest(ref), bundle


def load_selector(repo: Path) -> tuple[str, str, dict[str, Any]]:
    path = repo / "build/m12-auto60-contiguous-islands-a/materialized/manifest.json"
    data, sha = _read(path)
    ref = _ref(repo, path)
    bundle = _bundle(ref)
    kinds = {"CCB0_RELATIVE_TARGET_TABLE_256X16", "SELECTED_RELATIVE_POINTER_SLOT_16BIT", "STATIC_MULTI_DISPATCH_TARGET"}
    entries = [item for item in data["entries"] if item.get("classification") in kinds and item.get("confidence") == "CONFIRMED"]
    for item in entries[:6]:
        table = _node("TABLE", f"{item['start']}:{item['end']}", item, [ref])
        selector = _node("SELECTOR", f"manifest:{item['manifest_index']}", {"classification": item["classification"], "reason": item["ownership_reason"]}, [ref])
        target = _node("ROM_RANGE", f"{item['start']}:{item['end']}", item, [ref])
        bundle["nodes"] += [table, selector, target]
        bundle["edges"] += [_edge(selector, table, "SELECTS", [ref], "confirmed manifest selector/table semantics"),
                             _edge(table, target, "TARGETS", [ref], "exact manifest range")]
    return "selector-manifest", sha, bundle


def load_sources(repo: str | Path) -> list[tuple[str, str, dict[str, Any]]]:
    root = Path(repo)
    loaders = [load_cpu_r3b, load_sat_dma, load_graphics, load_selector]
    return sorted((loader(root) for loader in loaders), key=lambda item: item[0])
