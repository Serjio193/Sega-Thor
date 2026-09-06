import importlib.util
from pathlib import Path


MODULE = Path(__file__).parents[1] / "src" / "tools" / "re_evidence_audit.py"
SPEC = importlib.util.spec_from_file_location("re_evidence_audit", MODULE)
AUDIT = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUDIT)


def main():
    entry = {"start": 0x100, "end": 0x110, "kind": "CODE_VERIFIED",
             "artifact": "code/sub_000100.asm"}
    candidate = {"entry": "0x100"}
    other_function = {"entry": "0x100", "range": "0x100..0x120"}
    assert not AUDIT.provenance_status(entry, candidate, other_function)["matched"]
    assert "ghidra_range_mismatch" in AUDIT.provenance_status(
        entry, candidate, other_function)["reasons"]
    assert not AUDIT.provenance_status(
        entry, {"entry": "0x120"}, {"entry": "0x120", "range": "0x120..0x130"})["matched"]

    assert AUDIT.trust_classification(True) == "ASM_ROUNDTRIP_EXACT"
    assert AUDIT.trust_classification(True, static_supported=True) == "CODE_STATIC_SUPPORTED"
    assert AUDIT.trust_classification(True, executed=True) == "CODE_EXECUTED"

    target = {"entry": "0x100", "range": "0x100..0x110", "called_by": ["0x200"]}
    caller = {"entry": "0x200", "range": "0x200..0x210", "calls": ["0x100"]}
    xrefs = AUDIT.incoming_xrefs(0x100, target, [caller],
                                 {0x200: "CODE_STATIC_SUPPORTED"})
    assert xrefs[0]["edge_to_audited_entry"] and xrefs[0]["trusted"]

    # Data-like bytes may round-trip as legal words but have no static trust path.
    assert len(AUDIT.NEGATIVE_CORPUS) == 3
    assert bytes.fromhex(AUDIT.NEGATIVE_CORPUS[0]) == b"\x4e\x71\x4e\x75"
    assert AUDIT.trust_classification(True) == "ASM_ROUNDTRIP_EXACT"

    manifest = {"entries": [dict(entry)]}
    downgraded = AUDIT.apply_classifications(manifest, [{
        "start": 0x100, "new_classification": "ASM_ROUNDTRIP_EXACT"}])
    assert downgraded["entries"][0]["start"] == entry["start"]
    assert downgraded["entries"][0]["end"] == entry["end"]
    assert downgraded["entries"][0]["artifact"] == entry["artifact"]
    assert downgraded["entries"][0]["classification"] == "ASM_ROUNDTRIP_EXACT"

    assert AUDIT.canonical_instruction_form("    rts") == "rts"
    assert AUDIT.canonical_instruction_form("    moveq #0,D0") == "moveq"
    assert AUDIT.canonical_instruction_form("    beq.s loc_000100") == "beq.branch"
    assert AUDIT.canonical_instruction_form({"operation": "beq",
        "branch_width_bytes": 1, "width_bytes": 1}) == "beq.branch"
    print("evidence audit helper tests passed")


if __name__ == "__main__":
    main()
