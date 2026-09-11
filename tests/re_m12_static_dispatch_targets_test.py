import importlib.util
from pathlib import Path
import tempfile
from m12_local_inputs import optional_bytes


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_static_dispatch_targets_promote",
    ROOT / "src/tools/re_m12_static_dispatch_targets_promote.py",
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_contract_and_table_source():
    rom = optional_bytes(
        ROOT / "build/reference/Beyond Oasis (USA).bin", "canonical ROM")
    if rom is None:
        return
    contract = MODULE.parse_contract(rom)
    assert contract["table_values"] == MODULE.TABLE_VALUES
    assert contract["targets"][-1]["end"] == 0x03B188
    with tempfile.TemporaryDirectory() as directory:
        path = Path(directory) / "table.asm"
        MODULE.table_source(path)
        text = path.read_text()
        assert "dc.l $0003B0BC,$0003B0E8,$0003B132,$0003B0BA" in text
        dispatch = Path(directory) / "dispatch.asm"
        MODULE.dispatcher_source(dispatch)
        assert "movea.l 0(A0,D0.W),A0" in dispatch.read_text()


def test_split_requires_unknown_boundary():
    entries = [{"start": 0, "end": 0x100, "kind": "UNKNOWN", "size": 0x100}]
    result = MODULE.split_unknown(
        entries, 0x20, 0x30, "CODE_VERIFIED", "test", "bounded")
    assert [(entry["start"], entry["end"], entry["kind"]) for entry in result] == [
        (0, 0x20, "UNKNOWN"), (0x20, 0x30, "CODE_VERIFIED"),
        (0x30, 0x100, "UNKNOWN")]


if __name__ == "__main__":
    test_contract_and_table_source()
    test_split_requires_unknown_boundary()
    print("M12 static dispatch target helper tests passed")
