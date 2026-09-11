"""Regression checks for graph-guided M12 Carver expansion helpers."""

import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "src/tools"))
from m12_carver_expansion import pointer_records, raw_size_hypotheses


def test_graph_expansion_keeps_candidates_non_owned_and_typed():
    table = {"records": [
        {"index": 4, "address": 0x3F386,
         "fields": [0, 0x13AD1C, 0, 0, 0, 0, 0xC00, 0]},
        {"index": 5, "address": 0x3F3A6,
         "fields": [0, 0x11CE60, 0, 0, 0, 0, 0xC00, 0]},
    ]}
    records, edges, nodes = pointer_records(table, "range:table")
    hypotheses = raw_size_hypotheses(table, "range:table")
    assert len(records) == 2 and len(edges) == 4 and len(nodes) == 2
    assert all(edge["type"] in {"REFERENCES", "POINTS_TO"} for edge in edges)
    assert [item["end"] - item["start"] for item in hypotheses] == [0xC00, 0xC00]
    assert all(item["confidence"] == "CANDIDATE" for item in hypotheses)
    assert all(item["provenance_parents"] == ["range:table"] for item in hypotheses)


if __name__ == "__main__":
    test_graph_expansion_keeps_candidates_non_owned_and_typed()
    print("M12 Carver expansion helper tests passed")
