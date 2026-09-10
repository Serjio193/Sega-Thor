import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "chain", ROOT / "src/tools/re_m12_direct_graphics_chain_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)

assert len(MODULE.ANCHORS) == 3
assert len(MODULE.STREAMS) == 5
assert sum(item["end"] - item["start"] for item in MODULE.ANCHORS) == 8807
assert sum(item["end"] - item["start"] for item in MODULE.STREAMS) == 21016
all_streams = sorted(MODULE.ANCHORS + MODULE.STREAMS,
                     key=lambda item: item["start"])
for earlier, later in zip(all_streams, all_streams[1:]):
    if earlier["end"] == later["start"]:
        continue
    assert later["start"] > earlier["end"]

entries = [{"start": 0, "end": 0x300000, "kind": "UNKNOWN", "size": 0}]
for item in MODULE.STREAMS:
    entries = MODULE.split_unknown(entries, item["start"], item["end"])
owned = sum(item["size"] for item in entries if item["kind"] != "UNKNOWN")
assert owned == 21016
assert all(item["classification"] == "DIRECT_GRAPHICS_STREAM"
           for item in entries if item["kind"] == "LOCAL_ROM_DERIVED_ASSET")
print("M12 direct graphics chain promotion helper tests passed")
