import importlib.util
from pathlib import Path


MODULE = Path(__file__).parents[1] / "src" / "tools" / "re_m12_3_promote.py"
SPEC = importlib.util.spec_from_file_location("re_m12_3_promote", MODULE)
M12 = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M12)


def main():
    assert M12.TARGET == (0x6516, 0x83D4)
    assert M12.BASELINE_COMMIT == "b9b55fc46fad88eb2ff1285e85bd9d8169d0289d"
    assert sum(end - start for start, end, *_ in M12.PROMOTIONS) == 696
    assert len(M12.PROMOTIONS) == 14
    assert [(start, end) for start, end, *_ in M12.PROMOTIONS] == [
        (0x007A28, 0x007B2A), (0x007B64, 0x007B76),
        (0x007B76, 0x007B7A), (0x007B7A, 0x007B84),
        (0x007B84, 0x007B9A), (0x007B9A, 0x007BA4),
        (0x007BA4, 0x007BD4), (0x007BD4, 0x007BE4),
        (0x007BE4, 0x007BE8), (0x007BE8, 0x007BF6),
        (0x007BF6, 0x007C20), (0x007C20, 0x007C3C),
        (0x0082AE, 0x0082F8), (0x0082F8, 0x00838C)]
    assert M12.next_p0() == {
        "start": 0, "end": 0x7C4, "size": 0x7C4,
        "observed_pc_count": 23, "static_xref_count": 3,
        "reason": "highest remaining P0 observed-PC concentration after M12.3 removal; not started"}
    print("M12.3 helper tests passed")


if __name__ == "__main__":
    main()
