# M12.5 — exact nested ASM islands

Result: `M12_5_EXACT_NESTED_ISLANDS_PARTIAL`.

Against the `33cb6d9985b3a87cd9eb92d4ad0883a736bde9ff` baseline, this
transaction promotes 23 non-overlapping islands inside
`0x003B3E..0x004A92`, totaling 1,426 bytes. Each interval has a bounded
68000 decode and vasm byte round-trip. The enclosing routines remain
blob-backed where their full CFG has gaps or overlap. The full ROM remains
byte-exact with zero gaps and overlaps. The combined M12-AUTO result is
recorded in `ASM_AUTONOMOUS_TO_90_PERCENT_M12_AUTO.md`.
