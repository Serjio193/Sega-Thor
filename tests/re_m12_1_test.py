import importlib.util
from pathlib import Path


MODULE = Path(__file__).parents[1] / "src" / "tools" / "re_m12_1_promote.py"
SPEC = importlib.util.spec_from_file_location("re_m12_1_promote", MODULE)
M12 = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M12)


def main():
    entries = [
        {"start": 0, "end": 10, "kind": "CODE_VERIFIED"},
        {"start": 10, "end": 100, "kind": "UNKNOWN"},
        {"start": 100, "end": 200, "kind": "CODE_VERIFIED"},
    ]
    assert M12.target_blob_intervals(entries) == []
    assert M12.TARGET == (0x06042A, 0x0611F4)
    assert sum(end - start for start, end, _ in M12.PROMOTIONS) == 546
    metrics = M12.target_metrics([
        {"start": 0x06042A, "end": 0x060484, "kind": "CODE_VERIFIED"},
        {"start": 0x060484, "end": 0x0611F4, "kind": "UNKNOWN"},
    ])
    assert metrics["asm"] == 90 and metrics["blob"] == 3_440
    assert M12.overlap((0x06042A, 0x0611F4), (0x060B50, 0x060CDA))
    assert not M12.overlap((0x06042A, 0x0611F4), (0x0611F4, 0x061200))
    print("M12.1 helper tests passed")


if __name__ == "__main__":
    main()
