import json
import tempfile
from pathlib import Path

import sys

sys.path.insert(0, str(Path(__file__).parents[1] / "src" / "tools" / "thor_evidence"))
import live_discovery_analyze as analyze


def main():
    entries = [{"start": 0x100, "end": 0x120, "kind": "CODE_VERIFIED"}]
    assert analyze.classify(entries, 0x110)[0] == "CODE_VERIFIED"
    assert analyze.classify(entries, 0x200)[0] == "OUTSIDE_ROM_MAP"
    event = {
        "kind": "ROM_READ_NOVELTY", "epoch": 1, "frame": 5,
        "data": {"address": 0x200, "registers": {"PC": 0x300}},
    }
    item = analyze.investigation(event, "NEW_ROM_ACTIVITY", [None, None], "capture")
    assert item["new_edges"] == [{"source_pc": 0x300, "target_address": 0x200,
                                  "kind": "ROM_READ"}]
    with tempfile.TemporaryDirectory() as directory:
        report = Path(directory) / "static.json"
        report.write_text(json.dumps({
            "entry_point": "0x300",
            "instructions": [{"memory_references": []}],
        }))
        follow_up = analyze.static_follow_up(item, report)
        assert follow_up["status"] == "STATIC_SLICE_COMPLETE_INCONCLUSIVE"
        assert follow_up["absolute_rom_reference_count"] == 0
    print("live discovery analysis tests passed")


if __name__ == "__main__":
    main()
