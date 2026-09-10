import importlib.util
from pathlib import Path


MODULE = Path(__file__).parents[1] / "src" / "tools" / "re_m12_4_promote.py"
SPEC = importlib.util.spec_from_file_location("re_m12_4_promote", MODULE)
M12 = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M12)


def main():
    assert M12.TARGET == (0, 0x7C4)
    assert M12.VECTOR == (0, 0x100)
    assert M12.HEADER == (0x100, 0x200)
    assert M12.STRUCTURED == (0x29C, 0x308)
    assert [(start, end) for start, end, *_ in M12.CODE_RANGES] == [
        (0x200, 0x204), (0x204, 0x206), (0x206, 0x20A), (0x20A, 0x20C),
        (0x20C, 0x20E), (0x20E, 0x29C), (0x308, 0x45A), (0x6F6, 0x7C4)]
    assert sum(end - start for start, end, *_ in M12.CODE_RANGES) == 700
    target_entries = [
        {"start": 0, "end": 0x100, "kind": "HEADER_VECTOR_ASM"},
        {"start": 0x100, "end": 0x200, "kind": "HEADER_VECTOR_ASM"},
        {"start": 0x200, "end": 0x20E, "kind": "CODE_VERIFIED"},
        {"start": 0x20E, "end": 0x29C, "kind": "CODE_VERIFIED"},
        {"start": 0x29C, "end": 0x308, "kind": "STRUCTURED_DATA_CONFIRMED"},
        {"start": 0x308, "end": 0x45A, "kind": "CODE_VERIFIED"},
        {"start": 0x45A, "end": 0x6F6, "kind": "UNKNOWN"},
        {"start": 0x6F6, "end": 0x7C4, "kind": "CODE_VERIFIED"},
    ]
    assert M12.target_metrics(target_entries) == {
        "header_vector_asm": 512, "asm": 700, "structured_data_asm": 108,
        "padding": 0, "blob": 668}
    vectors = bytearray(0x100)
    for offset in range(0, 0x100, 4):
        vectors[offset:offset + 4] = (0x00000200).to_bytes(4, "big")
    vectors[4:8] = (0x0000020E).to_bytes(4, "big")
    assert M12.vector_evidence(bytes(vectors))["reset_target"] == 0x20E
    assert M12.next_m12_5() == {
        "start": 0x3B3E, "end": 0x4A92, "size": 3924,
        "observed_pc_count": 12, "static_xref_count": 3,
        "reason": "largest remaining source-owned byte-gain opportunity; not started"}
    rom = bytes(range(256)) * 4
    assert M12.directive_text(rom, 0, 16, 4).count("dc.l") == 1
    print("M12.4 helper tests passed")


if __name__ == "__main__":
    main()
