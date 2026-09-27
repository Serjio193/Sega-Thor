"""Guard the consumer-side direct M68K ROM-to-VDP hook wiring."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]


def main() -> None:
    patcher = (ROOT / "tools/bizhawk-native-ring/apply_runtime_rom_properties_v1.py")
    source = patcher.read_text(encoding="utf-8")
    replacement_start = source.index("oasis_romprops_vdp_cpu_source_write(")
    replacement_start = source.rfind('replace_once(gx / "core/vdp_ctrl.c"',
                                     0, replacement_start)
    replacement_end = source.index("\n    replace_once", replacement_start + 1)
    replacement = source[replacement_start:replacement_end]
    assert "vdp_bus_w(data)" in replacement
    assert "/* Check if DMA Fill is pending */" in replacement
    assert "(uint16_t)data" in replacement
    assert "code & 0x0fu" in replacement
    assert "dmafill != 0" in replacement
    assert replacement.index("vdp_bus_w(data);") < replacement.index(
        "oasis_romprops_vdp_cpu_source_write(")

    runtime = (ROOT / "tools/bizhawk-native-ring/rom_properties_runtime.c").read_text(
        encoding="utf-8")
    assert "vdp_move_candidate.read_count == 1u && width == 16u" in runtime
    assert "resolve_m68k_word(address, (uint16_t)value" in runtime
    assert "thor_rom_properties_mark_vdp_direct_move" in runtime


if __name__ == "__main__":
    main()
