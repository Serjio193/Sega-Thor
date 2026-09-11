import importlib.util
from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SPEC = importlib.util.spec_from_file_location(
    "re_m12_sound_data_promote", ROOT / "src/tools/re_m12_sound_data_promote.py")
MODULE = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(MODULE)


def main():
    assert MODULE.DATA_START == MODULE.UPLOAD_END
    assert MODULE.DATA_END < MODULE.FF_END
    entries = [{"start": 0, "end": 0x300000, "kind": "UNKNOWN"}]
    promoted = MODULE.split_unknown(entries)
    owned = next(item for item in promoted if item["start"] == MODULE.DATA_START)
    assert owned["end"] == MODULE.DATA_END
    assert owned["kind"] == "DATA_KNOWN"
    assert owned["classification"] == "SOUND_DATA_CONTAINER_CONFIRMED"
    print("M12 sound-data promotion helper tests passed")


if __name__ == "__main__":
    main()
