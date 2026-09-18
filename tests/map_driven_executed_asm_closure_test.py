"""Deterministic synthetic gates for M12 2F; no ROM or runtime required."""
import hashlib
import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "map_driven_asm_closure", ROOT / "src/tools/thor_evidence/map_driven_asm_closure.py")
M = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M)
ROM_SHA = "a" * 64


def row(start, raw):
    return {"object_id": f"o{start}", "start": start, "end": start + len(raw),
            "attributes": {"opcode": int.from_bytes(raw[:2], "big"),
                           "length": len(raw), "bytes_sha256": hashlib.sha256(raw).hexdigest(),
                           "source_owned_bytes": 0}}


def ins(address, raw, flow="none", operation="nop", target=None, supported=True):
    words = [int.from_bytes(raw[i:i + 2], "big") for i in range(0, len(raw), 2)]
    return {"address": address, "opcode": words[0], "raw_words": words,
            "flow": flow, "operation": operation, "branch_target": target,
            "supported": supported}


def candidate(rows):
    start, end = rows[0]["start"], rows[-1]["end"]
    return {"candidate_id": M.stable_id(ROM_SHA, [(start, end)]), "rom_sha256": ROM_SHA,
            "intervals": [(start, end)], "instruction_ids": [r["object_id"] for r in rows],
            "instructions": rows, "observed_edge_count": max(0, len(rows) - 1)}


def prove(rows, decoded_items, raw, owned=None, data=None, observed=None):
    start, end = rows[0]["start"], rows[-1]["end"]
    result = M.prove_decoded_island(candidate(rows),
        {"source_decoder": "re_slice_decoder", "start": start, "end": end,
         "instructions": decoded_items}, raw, owned or [], data or [],
        observed_edges=observed or [])
    return result


def main():
    # A. A complete straight-line path closes through an exact terminal.
    raw = bytes.fromhex("4e714e75")
    rows = [row(0, raw[:2]), row(2, raw[2:])]
    result = prove(rows, [ins(0, raw[:2]), ins(2, raw[2:], "return", "rts")], raw)
    assert result["status"] == "PASS_CLOSED_ASM_RANGE"

    # B/C. A conditional branch target is exact, but remains DERIVED_EXACT.
    raw = bytes.fromhex("66024e714e75")
    rows = [row(0, raw[:2]), row(2, raw[2:4]), row(4, raw[4:])]
    result = prove(rows, [ins(0, raw[:2], "direct_branch", "bne", 4),
                          ins(2, raw[2:4]), ins(4, raw[4:], "return", "rts")], raw,
                   observed=[(0, 2), (2, 4)])
    branch = next(edge for edge in result["edges"] if edge["kind"] == "DIRECT_ENCODED")
    assert branch["target"] == 4 and branch["status"] == "DERIVED_EXACT"
    assert not any(edge["source"] == 0 and edge["target"] == 4 and
                   edge["status"] == "OBSERVED_RUNTIME" for edge in result["edges"])

    # D. BSR to already source-owned code is allowed; local return path falls through.
    raw = bytes.fromhex("61064e75")
    rows = [row(0, raw[:2]), row(2, raw[2:])]
    result = prove(rows, [ins(0, raw[:2], "direct_call", "bsr", 8),
                          ins(2, raw[2:], "return", "rts")], raw, owned=[(8, 10)])
    assert result["status"] == "PASS_CLOSED_ASM_RANGE"
    assert next(e for e in result["edges"] if e["kind"] == "DIRECT_ENCODED")["classification"] == "SOURCE_OWNED"

    # E. RTS is a terminal and must not invent a fallthrough.
    raw = bytes.fromhex("4e75")
    result = prove([row(0, raw)], [ins(0, raw, "return", "rts")], raw)
    assert result["status"] == "PASS_CLOSED_ASM_RANGE"
    assert result["edges"] == []

    # F. DATA immediately after a closed interval is not consumed.
    result = prove([row(0, raw)], [ins(0, raw, "return", "rts")], raw,
                   data=[(2, 4)])
    assert result["status"] == "PASS_CLOSED_ASM_RANGE"

    # G. An instruction crossing the proposed end rejects the boundary.
    crossing = bytes.fromhex("4e714e75")
    result = prove([row(0, crossing[:2])], [ins(0, crossing, "none", "move")], crossing[:2])
    assert "STOP_BOUNDARY_UNPROVEN" in result["blockers"]

    # H. Unsupported instruction encodings fail closed.
    raw = bytes.fromhex("ffff")
    result = prove([row(0, raw)], [ins(0, raw, "unsupported", "unsupported", supported=False)], raw)
    assert "STOP_UNSUPPORTED_M68K_DECODE" in result["blockers"]

    # I. An indirect dispatch with no accepted 2E witness is unresolved.
    raw = bytes.fromhex("4ef1")
    result = prove([row(0, raw)], [ins(0, raw, "indirect_jump", "jmp")], raw)
    assert "STOP_CONTROL_FLOW_ESCAPE_UNRESOLVED" in result["blockers"]

    # J/K. Byte-identical ASM passes; a single changed byte fails.
    assert M.verify_roundtrip(b"abc", b"abc")["status"] == "PASS_ASM_ROUNDTRIP_EXACT"
    bad = M.verify_roundtrip(b"abc", b"axc")
    assert bad["status"] == "STOP_ASM_ROUNDTRIP_MISMATCH" and bad["first_difference"] == 1

    # L. Repeated FLOW evidence does not duplicate candidate instructions.
    rows = [row(0, bytes.fromhex("4e71")), row(2, bytes.fromhex("4e75"))]
    edges = [{"source_object_id": "o0", "target_object_id": "o2"}] * 5
    paths = M.observed_chains(rows, edges, ROM_SHA)
    assert len(paths) == 1 and paths[0]["instruction_ids"] == ["o0", "o2"]

    # M. Overlapping candidates merge deterministically at exact instruction boundaries.
    rows = [row(0, bytes.fromhex("4e71")), row(2, bytes.fromhex("4e71")),
            row(4, bytes.fromhex("4e75"))]
    first = candidate(rows[:2]); second = candidate(rows[1:])
    merged_a = M.merge_overlapping_candidates([second, first])
    merged_b = M.merge_overlapping_candidates([first, second])
    assert merged_a == merged_b and merged_a[0]["intervals"] == [(0, 6)]

    # N/O. Promotion changes only selected UNKNOWN bytes and accounts unique ownership.
    baseline = [{"start": 0, "end": 8, "kind": "UNKNOWN", "confidence": "ROM_HASH_VERIFIED",
                 "emitted_artifact_type": "blob", "artifact": "blob.bin"}]
    promotions = [{"start": 2, "end": 4, "artifact": "code/sub_000002.asm"}]
    after = M.promote_entries(baseline, promotions)
    assert [(e["start"], e["end"], e["kind"]) for e in after] == [
        (0, 2, "UNKNOWN"), (2, 4, "CODE_VERIFIED"), (4, 8, "UNKNOWN")]
    assert M.ownership_delta(baseline, after, promotions, 8) == 2
    assert baseline[0]["start"] == 0 and baseline[0]["end"] == 8
    adjacent = M.promote_entries(baseline, [
        {"start": 2, "end": 4, "artifact": "code/a.asm"},
        {"start": 4, "end": 6, "artifact": "code/b.asm"}])
    assert [(e["start"], e["end"]) for e in adjacent] == [(0, 2), (2, 4), (4, 6), (6, 8)]

    # A locally exact island cannot escape into a candidate that failed another gate.
    selected, dependencies = M.dependency_closed_starts([
        {"start": 2, "edges": [{"target": 8, "status": "DERIVED_EXACT",
                                 "classification": "CANDIDATE_ISLAND"}]},
        {"start": 24, "edges": []}])
    assert selected == {24} and dependencies[2] == [8]

    # P/Q. Partition and serialized manifest are complete and deterministic.
    M.validate_partition(after, 8)
    assert M.canonical_hash(after) == M.canonical_hash(M.promote_entries(baseline, promotions))
    try:
        M.validate_partition(after + [{"start": 8, "end": 9}], 8)
    except ValueError as error:
        assert "PARTITION" in str(error)
    else:
        raise AssertionError("out-of-bounds partition must fail")

    # R. A rejected transaction cannot mutate its accepted input.
    frozen = [dict(entry) for entry in after]
    try:
        M.promote_entries(after, [{"start": 1, "end": 3, "artifact": "x.asm"}])
    except ValueError:
        pass
    else:
        raise AssertionError("overlapping promotion must fail")
    assert after == frozen

    # S. Reapplying an exact accepted promotion is a deterministic no-op.
    assert M.promote_entries(after, promotions) == after

    # T. Existing runtime edges retain observed status; decode-derived edges stay static.
    runtime = M.prove_decoded_island(candidate(rows[:2]),
        {"source_decoder": "re_slice_decoder", "start": 0, "end": 4,
         "instructions": [ins(0, bytes.fromhex("4e71")),
                          ins(2, bytes.fromhex("4e71"))]},
        bytes.fromhex("4e714e71"), [], [], observed_edges=[(0, 2)])
    assert [edge["status"] for edge in runtime["edges"]].count("OBSERVED_RUNTIME") == 1
    assert all(edge["status"] != "OBSERVED_RUNTIME" for edge in runtime["edges"]
               if edge["kind"] in {"DIRECT_ENCODED", "FALLTHROUGH"})
    print("M12 2F synthetic gates A-T passed")


if __name__ == "__main__":
    main()
