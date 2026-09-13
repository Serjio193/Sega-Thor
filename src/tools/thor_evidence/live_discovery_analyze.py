"""Compare one real live capture with persisted M12 evidence and persist leads."""
import argparse
import hashlib
import json
import sqlite3
from pathlib import Path


def known_addresses(paths, kind):
    result = set()
    for path in paths:
        db = sqlite3.connect(f"file:{Path(path).resolve()}?mode=ro", uri=True)
        for (payload,) in db.execute("SELECT payload FROM event WHERE kind=?", (kind,)):
            address = json.loads(payload).get("data", {}).get("address")
            if isinstance(address, int):
                result.add(address)
        db.close()
    return result


def manifest_lookup(manifest):
    return manifest["entries"]


def classify(entries, address):
    for entry in entries:
        if entry["start"] <= address < entry["end"]:
            return entry["kind"], entry["start"], entry["end"]
    return "OUTSIDE_ROM_MAP", None, None


def static_follow_up(selected, path):
    if not selected:
        return {"status": "NOT_SELECTED"}
    if not path:
        return {"status": "PENDING", "selected_investigation_ids": [selected["id"]]}
    report = json.loads(Path(path).read_text())
    absolute_rom = [item for item in report.get("instructions", [])
                    if any(ref.get("kind") == "rom" for ref in item.get("memory_references", []))]
    return {
        "status": "STATIC_SLICE_COMPLETE_INCONCLUSIVE",
        "selected_investigation_ids": [selected["id"]],
        "static_report": str(Path(path).resolve()),
        "entry_point": report.get("entry_point"),
        "instruction_count": len(report.get("instructions", [])),
        "absolute_rom_reference_count": len(absolute_rom),
        "absolute_rom_references": absolute_rom,
        "conclusion": "selected live ROM address remains register-based; no direct static ROM reference or new root/consumer is proven",
    }


def investigation(event, classification, span, capture_id):
    data = event["data"]
    registers = data.get("registers", {})
    pc = registers.get("PC")
    key = f"{capture_id}:{event['kind']}:{pc}:{data['address']}"
    ident = "INV-AUTO62-" + hashlib.sha256(key.encode()).hexdigest()[:16].upper()
    edge_kind = "ROM_READ" if event["kind"] == "ROM_READ_NOVELTY" else "RAM_WRITE"
    edge = {"source_pc": pc, "target_address": data["address"], "kind": edge_kind}
    return {
        "id": ident, "first_observed_event": event["kind"], "trace_id": capture_id,
        "epoch": event["epoch"], "frame": event["frame"], "pc": pc,
        "rom_ram_address": data["address"], "current_classification": classification,
        "known_incoming_edges": [], "known_outgoing_edges": [], "new_edges": [edge],
        "new_roots": [], "new_consumers": [], "domain_expansions": [], "conflicts": [],
        "missing_explanation": "live event is absent from persisted Evidence Engine observations",
        "potential_value": "connects real execution to an unexplained M12 map frontier",
        "next_evidence_required": "focused static slice and/or repeated state-bound capture",
        "status": "OPEN", "span": span, "occurrences": 1,
    }


def run(args):
    manifest = json.loads(Path(args.manifest).read_text())
    gpgx = json.loads(Path(args.gpgx).read_text())
    known_exec = known_addresses(args.database, "EXEC")
    known_read = known_addresses(args.database, "READ")
    known_write = known_addresses(args.database, "WRITE")
    gpgx_exec = {int(value, 0) for value in gpgx.get("executed_addresses", [])}
    lines = [json.loads(line) for line in Path(args.raw).read_text().splitlines() if line.strip()]
    header = next(item for item in lines if item["kind"] == "RAW_HEADER")
    events = [item for item in lines if item["kind"] in {
        "EXEC_NOVELTY", "ROM_READ_NOVELTY", "RAM_WRITE_NOVELTY"}]
    counts = {"KNOWN": 0, "KNOWN_STRUCTURE_ACTIVITY": 0,
              "ALREADY_KNOWN_EXTERNAL": 0, "NEW_ROM_ACTIVITY": 0,
              "NEW_WRITER": 0, "UNKNOWN": 0}
    investigations = {}
    new_observations = []
    new_edges = {}
    unresolved_novelties = []
    for event in events:
        address = event["data"]["address"]
        kind = event["kind"]
        known = address in ({"EXEC_NOVELTY": known_exec, "ROM_READ_NOVELTY": known_read,
                             "RAM_WRITE_NOVELTY": known_write}[kind])
        if known:
            counts["KNOWN"] += 1
            continue
        if kind == "EXEC_NOVELTY" and address in gpgx_exec:
            counts["ALREADY_KNOWN_EXTERNAL"] += 1
            continue
        if kind == "RAM_WRITE_NOVELTY":
            classification, start, end = "NEW_WRITER", None, None
        else:
            classification, start, end = classify(manifest_lookup(manifest), address)
            if classification == "OUTSIDE_ROM_MAP" or classification == "UNKNOWN":
                classification = "NEW_ROM_ACTIVITY"
            else:
                classification = "KNOWN_STRUCTURE_ACTIVITY"
        counts[classification] = counts.get(classification, 0) + 1
        if classification in {"NEW_ROM_ACTIVITY", "NEW_WRITER", "UNKNOWN"}:
            item = investigation(event, classification, [start, end], header["capture_id"])
            existing = investigations.get(item["id"])
            if existing:
                existing["occurrences"] += 1
            else:
                investigations[item["id"]] = item
            edge = item["new_edges"][0]
            edge_key = (edge["source_pc"], edge["target_address"], edge["kind"])
            new_edges[edge_key] = edge
            new_observations.append({"kind": kind, "address": address,
                                     "classification": classification, "frame": event["frame"],
                                     "pc": item["pc"]})
        elif classification not in {"KNOWN", "ALREADY_KNOWN_EXTERNAL", "KNOWN_STRUCTURE_ACTIVITY"}:
            unresolved_novelties.append({"kind": kind, "address": address,
                                         "classification": classification})
    created = list(investigations.values())
    for item in created:
        item["new_edges"] = [edge for edge in item["new_edges"]]
    created.sort(key=lambda item: (
        0 if item["current_classification"] == "NEW_ROM_ACTIVITY" else 1,
        -(item["span"][1] - item["span"][0]) if item["span"][0] is not None else 0,
        -item["occurrences"], item["id"]))
    selected = created[:1]
    rom_new = counts["NEW_ROM_ACTIVITY"]
    writer_new = counts["NEW_WRITER"]
    report = {
        "schema": "oasis.m68k.m12-live-discovery-report.v2",
        "capture": {"raw": str(Path(args.raw).resolve()), "capture_id": header["capture_id"],
                     "scenario": header.get("scenario"), "rom_sha256": header["rom_sha256"],
                     "state_sha256": header["state_sha256"],
                     "watch_plan_sha256": header["watch_plan_sha256"],
                     "emulator_executed": True},
        "known_before_run": {"sqlite_exec": len(known_exec), "sqlite_read": len(known_read),
                              "sqlite_write": len(known_write), "prior_gpgx_exec": len(gpgx_exec)},
        "known_observations": counts["KNOWN"] + counts["ALREADY_KNOWN_EXTERNAL"],
        "new_observations": new_observations,
        "counts": counts,
        "actually_new_information": bool(new_observations),
        "known_edges": [],
        "new_edges": sorted(new_edges.values(), key=lambda edge: (
            edge["source_pc"] if edge["source_pc"] is not None else -1,
            edge["target_address"], edge["kind"])),
        "new_roots": [],
        "new_consumers": [],
        "domain_expansions": [],
        "conflicts": [],
        "unresolved_novelties": unresolved_novelties,
        "investigations_created": created,
        "investigations_selected": selected,
        "follow_up": static_follow_up(selected[0] if selected else None, args.static_report),
        "novelty_summary": {
            "new_rom_activity_events": rom_new,
            "new_ram_writer_events": writer_new,
            "new_edge_count": len(new_edges),
            "new_roots_proven": 0,
            "new_consumers_proven": 0,
            "domain_expansions_proven": 0,
        },
        "source_owned_change": 0,
        "negative_findings": [
            "no ownership promotion is authorized by live observation alone",
            "no new execution PC was emitted by the bounded capture",
            "new root, consumer, and domain-expansion claims remain unproven",
        ],
    }
    Path(args.output).write_text(json.dumps(report, indent=2) + "\n")
    print(json.dumps(report, indent=2))


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--raw", required=True)
    parser.add_argument("--manifest", required=True)
    parser.add_argument("--database", action="append", required=True)
    parser.add_argument("--gpgx", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--static-report")
    args = parser.parse_args()
    run(args)


if __name__ == "__main__":
    main()
