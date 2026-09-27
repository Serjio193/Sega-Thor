"""Guard the exact Format-A Z80 read-site property adapter."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    runtime = (ROOT / "tools/bizhawk-native-ring/rom_properties_runtime.c").read_text(
        encoding="utf-8")
    assert "thor_rom_properties_mark_audio_payload" in runtime
    assert "(uint16_t)Z80.pc.w.l" in runtime
    source = (ROOT / "tools/bizhawk-native-ring/rom_properties_core.c").read_text(
        encoding="utf-8")
    assert "z80_pc != 0x080eu && z80_pc != 0x0855u" in source
    assert "source_offset < 0x0bc95cu" in source
    assert "source_offset >= 0x0bf768u" in source
    assert "thor_rom_properties_or_byte(map, source_offset" in source


if __name__ == "__main__":
    main()
