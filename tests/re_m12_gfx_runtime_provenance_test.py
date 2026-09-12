from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
SCRIPT = ROOT / "src/tools/re_bizhawk_m12_gfx_provenance.lua"
CALLERS = ("00D54A", "00D650", "02DB52", "02F6A0", "03B236", "03B28A",
           "03B2FE", "03C07C", "03D5AE", "03E61A")


def main():
    text = SCRIPT.read_text(encoding="utf-8")
    assert "oasis.m68k.m12-gfx-runtime-provenance.v1" in text
    assert "writes_emitted\\\":false" in text or "writes_emitted\":false" in text
    assert "OASIS_SCENARIO_FILE is required" in text
    for address in CALLERS:
        assert f"0x{address}" in text
    assert text.count("event.on_bus_exec") == 2
    print("re_m12_gfx_runtime_provenance_test: pass")


if __name__ == "__main__":
    main()
