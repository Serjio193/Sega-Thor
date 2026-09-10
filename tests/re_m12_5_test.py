import importlib.util
from pathlib import Path


MODULE = Path(__file__).parents[1] / "src" / "tools" / "re_m12_5_promote.py"
SPEC = importlib.util.spec_from_file_location("re_m12_5_promote", MODULE)
M12 = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M12)


def main():
    assert M12.TARGET == (0x3B3E, 0x4A92)
    assert M12.BASELINE_COMMIT == "33cb6d9985b3a87cd9eb92d4ad0883a736bde9ff"
    assert len(M12.PROMOTIONS) == 23
    assert sum(end - start for start, end, *_ in M12.PROMOTIONS) == 1426
    M12.validate_ranges()
    entries = [{"start": 0, "end": 0x100000, "kind": "UNKNOWN"}]
    split = M12.split_owned(entries, 0x3B3E, 0x3D06, Path("island.asm"), "test")
    assert M12.target_metrics(split) == {"asm": 456, "blob": 3468}
    print("M12.5 helper tests passed")


if __name__ == "__main__":
    main()
