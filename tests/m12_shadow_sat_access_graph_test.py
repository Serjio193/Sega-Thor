import importlib.util
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "m12_shadow_sat_access_graph", ROOT / "src/tools/m12_shadow_sat_access_graph.py"
)
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def test_synthetic_access_classification():
    census = {"instruction_count": 8, "instructions": [
        {"address": "0x10", "raw_bytes": "4A39", "status": "DECODED",
         "decoded_instruction": "tst.b ($00FF1858).L", "classification": "UNKNOWN"},
        {"address": "0x12", "raw_bytes": "50F9", "status": "DECODED",
         "decoded_instruction": "st.b ($00FF1858).L", "classification": "UNKNOWN"},
        {"address": "0x14", "raw_bytes": "3A39", "status": "DECODED",
         "decoded_instruction": "move.w ($00FF188A).L,D5", "classification": "UNKNOWN"},
        {"address": "0x18", "raw_bytes": "33C5", "status": "DECODED",
         "decoded_instruction": "move.w D5,($00FF188A).L", "classification": "UNKNOWN"},
        {"address": "0x1E", "raw_bytes": "23C0", "status": "DECODED",
         "decoded_instruction": "move.l D0,($00FF188A).L", "classification": "UNKNOWN"},
        {"address": "0x24", "raw_bytes": "DAF9", "status": "DECODED",
         "decoded_instruction": "adda.w ($00FF188C).L,A5", "classification": "UNKNOWN"},
        {"address": "0x2A", "raw_bytes": "33CD", "status": "DECODED",
         "decoded_instruction": "move.w A5,($00FF188C).L", "classification": "UNKNOWN"},
        {"address": "0x30", "raw_bytes": "0679", "status": "DECODED",
         "decoded_instruction": "addi.w #$30,($00FF188C).L", "classification": "UNKNOWN"},
    ]}
    graph = MODULE.build_graph(census)
    assert graph["targets"]["FF1858"]["readers"] == 1
    assert graph["targets"]["FF1858"]["writers"] == 1
    assert graph["targets"]["FF188A"]["direct_access_count"] == 3
    assert graph["targets"]["FF188A"]["writers"] == 2
    assert graph["targets"]["FF188A"]["accesses"][2]["alias"] == "overlaps FF188C"
    assert graph["targets"]["FF188C"]["direct_access_count"] == 3
    assert graph["targets"]["FF188C"]["writers"] == 2


def test_local_census_counts_when_available():
    path = ROOT / "build/gpgx-classified.json"
    if not path.exists():
        return
    graph = MODULE.build_graph(json.loads(path.read_text(encoding="utf-8")))
    assert graph["targets"]["FF1858"]["direct_access_count"] == 6
    assert graph["targets"]["FF188A"]["direct_access_count"] == 34
    assert graph["targets"]["FF188C"]["direct_access_count"] == 29


if __name__ == "__main__":
    test_synthetic_access_classification()
    test_local_census_counts_when_available()
    print("M12 shadow-SAT access graph tests passed")
