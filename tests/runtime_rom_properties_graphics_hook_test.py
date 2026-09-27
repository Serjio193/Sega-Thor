"""Guard the narrow live 0x3820 compressed-graphics consumer contract."""

from pathlib import Path


ROOT = Path(__file__).resolve().parents[1]
runtime = (ROOT / "tools/bizhawk-native-ring/rom_properties_runtime.c").read_text()
core = (ROOT / "tools/bizhawk-native-ring/rom_properties_core.c").read_text()
contract = (ROOT / "src/tools/thor_evidence/runtime_rom_properties_contract.json").read_text()

required_runtime_facts = (
    "m68k_instruction_pc == 0x3820u",
    "gfx_source_address = (uint32_t)REG_A[0]",
    "(uint16_t)m68k.ir == 0x4e75u",
    "uint32_t end = (uint32_t)REG_A[0]",
    "offset != first_offset + i",
    "THOR_ROM_COMPRESSED_GRAPHICS_SOURCE",
)
for fact in required_runtime_facts:
    assert fact in runtime or fact in core, fact
assert '"decoder_entry_pc": "0x003820"' in contract
assert '"name": "COMPRESSED_GRAPHICS_SOURCE"' in contract
assert "direct VRAM semantics inferred from compressed bytes" in contract
