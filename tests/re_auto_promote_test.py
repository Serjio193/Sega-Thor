import importlib.util
from pathlib import Path


MODULE = Path(__file__).parents[1] / "src" / "tools" / "re_auto_promote.py"
SPEC = importlib.util.spec_from_file_location("re_auto_promote", MODULE)
AUTO = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(AUTO)


def evidence(entry, complexity, score_class):
    return {"entry": f"0x{entry:X}", "decode_ok": True,
            "first_instruction_supported": True, "boundary_status": "BOUNDARY_AGREES",
            "unsupported_opcode_count": 0, "unsupported_addressing_count": 0,
            "unresolved_indirect_flow": False, "decode_conflict": False,
            "known_data_overlap": False, "confirmed_code_overlap": False,
            "other_ghidra_overlap": False, "complexity": complexity,
            "structural_classification": score_class, "direct_caller_count": 1,
            "known_static_target": True, "existing_beta_support": False}


def main():
    manifest = {"entries": [{"start": 0, "end": 100, "kind": "UNKNOWN"}]}
    mass = {"candidates": [evidence(40, "LEAF", "MODERATE_STATIC"),
                            evidence(10, "SHALLOW", "STRONG_STATIC")]}
    ghidra = {"functions": [{"entry": "0x28", "range": "0x28..0x30"},
                             {"entry": "0xA", "range": "0xA..0x12"}]}
    first, count = AUTO.discover_candidates(mass, ghidra, manifest, 2)
    assert count == 2 and first[0]["address"] == 0xA
    gated, gated_count = AUTO.discover_candidates(mass, ghidra, manifest, 2,
                                                   data_ranges=[(0x28, 0x30)])
    assert gated_count == 1 and gated[0]["address"] == 0xA
    assert first == AUTO.discover_candidates(mass, ghidra, manifest, 2)[0]
    promoted = AUTO.promote(manifest["entries"], first[0])
    assert [(e["start"], e["end"], e["kind"]) for e in promoted] == [
        (0, 10, "UNKNOWN"), (10, 18, "CODE_VERIFIED"), (18, 100, "UNKNOWN")]
    assert sum(e["end"] - e["start"] for e in promoted) == 100
    assert manifest["entries"] == [{"start": 0, "end": 100, "kind": "UNKNOWN"}]
    assert AUTO.FULL.first_difference(b"abcd", b"abxd", [{"manifest_index": 0,
        "start": 0, "end": 4, "emitted_artifact_type": "blob"}])["rom_offset"] == 2
    assert AUTO.classify_error("fatal error: illegal opcode extension") == "ASSEMBLER_SYNTAX"
    assert AUTO.promotion_trust_level({}) == "ASM_ROUNDTRIP_EXACT"
    assert AUTO.promotion_trust_level({"known_static_target": True}) == "CODE_STATIC_SUPPORTED"
    assert AUTO.promotion_trust_level({"existing_dynamic_support": True}) == "CODE_EXECUTED"
    assert AUTO.trusted_data_overlap({"data_classifications": ["DATA_STRUCTURE_SUPPORTED"]})
    assert not AUTO.trusted_data_overlap({"data_classifications": ["DATA_HYPOTHESIS"]})
    assert AUTO.trusted_data_ranges({"ranges": [{"start": "0x20", "end": "0x30",
        "classification": "DATA_REGION_SUPPORTED"}]}) == [(0x20, 0x30)]
    assert AUTO.reject_form({"reason": "UNSUPPORTED_FORM", "detail": "no exact IR"}) == \
        "unsupported exact IR"
    windows = AUTO.acceptance_windows([{"accepted": value} for value in
                                       [True, False, True, True]])
    assert windows[0]["accepted"] == 3 and windows[0]["attempted"] == 4
    clusters = AUTO.reject_clusters([
        {"accepted": False, "reason": "UNSUPPORTED_FORM", "mnemonic": "foo",
         "operand_forms": ["unknown"], "address": "0x10"},
        {"accepted": False, "reason": "UNSUPPORTED_FORM", "mnemonic": "foo",
         "operand_forms": ["unknown"], "address": "0x20"}])
    assert clusters[0]["count"] == 2 and clusters[0]["mnemonic"] == "foo"
    print("auto promotion helper tests passed")


if __name__ == "__main__":
    main()
