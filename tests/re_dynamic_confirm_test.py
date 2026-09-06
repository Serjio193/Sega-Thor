import importlib.util
from pathlib import Path


MODULE = Path(__file__).parents[1] / "src" / "tools" / "re_dynamic_confirm.py"
SPEC = importlib.util.spec_from_file_location("re_dynamic_confirm", MODULE)
CONFIRM = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(CONFIRM)


def main():
    linked = CONFIRM.exact_linkage(["0x100", "0x102"], 0x100, 0x104)
    assert linked["matched"] and linked["observed_outside_range"] == []
    wrong = CONFIRM.exact_linkage(["0x100", "0x104"], 0x100, 0x104)
    assert not wrong["matched"] and wrong["observed_outside_range"] == [0x104]

    partial = CONFIRM.coverage([0x100], [0x100, 0x102])
    assert partial["partial"] and partial["fraction"] == 0.5
    assert CONFIRM.promote_from_natural("CODE_STATIC_SUPPORTED", "DYNAMIC_FORCED", True) == "CODE_STATIC_SUPPORTED"
    assert CONFIRM.promote_from_natural("CODE_STATIC_SUPPORTED", "DYNAMIC_NATURAL", True) == "CODE_EXECUTED"
    propagation = CONFIRM.no_propagation("CODE_STATIC_SUPPORTED", "CODE_STATIC_SUPPORTED", "CODE_EXECUTED")
    assert propagation["caller_after"] == "CODE_STATIC_SUPPORTED"
    print("targeted dynamic confirmation helper tests passed")


if __name__ == "__main__":
    main()
