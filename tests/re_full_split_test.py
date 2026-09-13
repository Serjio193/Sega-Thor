import importlib.util
from pathlib import Path
import tempfile


MODULE = Path(__file__).parents[1] / "src" / "tools" / "re_full_split_run.py"
SPEC = importlib.util.spec_from_file_location("re_full_split_run", MODULE)
FULL = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(FULL)


def main():
    routines = [{"start": 8, "end": 12, "confidence": "A", "asm": "code/a.asm"},
                {"start": 16, "end": 20, "confidence": "B", "asm": "code/b.asm"}]
    entries = FULL.build_entries(24, routines)
    assert [(entry["start"], entry["end"], entry["kind"]) for entry in entries] == [
        (0, 8, "UNKNOWN"), (8, 12, "CODE_VERIFIED"), (12, 16, "UNKNOWN"),
        (16, 20, "CODE_VERIFIED"), (20, 24, "UNKNOWN")]
    assert sum(entry["size"] for entry in entries) == 24
    assert FULL.build_entries(24, routines) == entries
    assert FULL.first_difference(bytes(range(24)), bytes(range(24)), entries) is None
    diff = FULL.first_difference(bytes(range(24)), bytes([0, 1, 9] + list(range(3, 24))), entries)
    assert diff["rom_offset"] == 2 and diff["manifest_entry"] == 0
    assert diff["artifact_type"] == "blob"
    summary = FULL.metrics(entries, 24)
    assert summary["TOTAL_ROM_BYTES"] == 24
    assert summary["ASM_BYTES"] == 8 and summary["BLOB_BYTES"] == 16
    with tempfile.TemporaryDirectory() as temp:
        first = Path(temp) / "first"
        second = Path(temp) / "second"
        for output in (first, second):
            (output / "code").mkdir(parents=True)
            (output / "blobs").mkdir()
            (output / "code" / "a.asm").write_text(
                "loc_000004 equ $000004\n    dc.w $4E71\n")
            (output / "code" / "b.asm").write_text(
                "loc_000004:\n    dc.w $4E75\n")
            (output / "blobs" / "000008_00000C.bin").write_bytes(b"\0" * 4)
            layout_entries = [
                {"start": 0, "end": 4, "kind": "CODE_VERIFIED",
                 "emitted_artifact_type": "asm", "artifact": "code/a.asm"},
                {"start": 4, "end": 8, "kind": "CODE_VERIFIED",
                 "emitted_artifact_type": "asm", "artifact": "code/b.asm"},
                {"start": 8, "end": 12, "kind": "UNKNOWN",
                 "emitted_artifact_type": "blob",
                 "artifact": "blobs/000008_00000C.bin"},
            ]
            FULL.write_layout(output, layout_entries)
        assert "loc_000004 equ" not in (first / "full_layout.asm").read_text()
        assert (first / "full_layout.asm").read_bytes() == \
            (second / "full_layout.asm").read_bytes()
        conflict = first / "code" / "a.asm"
        conflict.write_text("loc_000004 equ $000005\n")
        try:
            FULL.write_layout(first, layout_entries)
        except ValueError as error:
            assert "conflicting duplicate alias" in str(error)
        else:
            raise AssertionError("layout conflict must fail closed")
    print("full split helper tests passed")


if __name__ == "__main__":
    main()
