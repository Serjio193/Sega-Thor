import importlib.util
from pathlib import Path


MODULE = Path(__file__).parents[1] / "src" / "tools" / "re_m12_2_promote.py"
SPEC = importlib.util.spec_from_file_location("re_m12_2_promote", MODULE)
M12 = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(M12)


def main():
    assert M12.TARGET == (0x00DE00, 0x00E338)
    assert sum(end - start for start, end, *_ in M12.PROMOTIONS) == 366
    assert M12.next_p0() == {
        "start": 0x006516, "end": 0x0083D4, "size": 7870,
        "observed_pc_count": 25, "static_xref_count": 0,
        "reason": "highest remaining observed-PC concentration after M12.2 removal; not started"}
    print("M12.2 helper tests passed")


if __name__ == "__main__":
    main()
