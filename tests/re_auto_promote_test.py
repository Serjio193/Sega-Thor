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
    assert first == AUTO.discover_candidates(mass, ghidra, manifest, 2)[0]
    promoted = AUTO.promote(manifest["entries"], first[0])
    assert [(e["start"], e["end"], e["kind"]) for e in promoted] == [
        (0, 10, "UNKNOWN"), (10, 18, "CODE_VERIFIED"), (18, 100, "UNKNOWN")]
    assert sum(e["end"] - e["start"] for e in promoted) == 100
    assert manifest["entries"] == [{"start": 0, "end": 100, "kind": "UNKNOWN"}]
    assert AUTO.FULL.first_difference(b"abcd", b"abxd", [{"manifest_index": 0,
        "start": 0, "end": 4, "emitted_artifact_type": "blob"}])["rom_offset"] == 2
    print("auto promotion helper tests passed")


if __name__ == "__main__":
    main()
