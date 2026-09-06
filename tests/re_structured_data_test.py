import importlib.util
from pathlib import Path


MODULE = Path(__file__).parents[1] / "src" / "tools" / "re_structured_data.py"
SPEC = importlib.util.spec_from_file_location("re_structured_data", MODULE)
DATA = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(DATA)


def main():
    rom = bytes(range(64)) * 4
    good = {"id": "good", "start": 0, "end": 8, "width": 2, "count": 4,
            "consumer": "parser", "references": ["0x0"]}
    assert DATA.parse_words(rom, 0, 8, 2) == [1, 515, 1029, 1543]
    assert DATA.classify_table(good, rom, [])["status"] == "ACCEPTED"

    invalid_pointer = dict(good, id="invalid", width=4, end=8, count=2,
                           consumer="pointer parser")
    assert DATA.classify_table(invalid_pointer, rom, [])["status"] == "REJECTED"
    code_like = dict(good, id="code_like", consumer="", references=[])
    assert DATA.classify_table(code_like, rom, [])["classification"] == "UNKNOWN"

    conflict = DATA.classify_table(good, rom, [(2, 6, "CODE_EXECUTED")])
    assert conflict["status"] == "CONFLICT" and conflict["conflicts"]
    assert DATA.overlap((0, 8), (8, 10)) is False
    print("structured data classification helper tests passed")


if __name__ == "__main__":
    main()
