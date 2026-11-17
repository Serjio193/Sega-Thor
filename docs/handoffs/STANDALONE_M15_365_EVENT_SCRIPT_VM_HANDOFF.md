# Standalone Handoff — M15.365: ROM Room/Event Script VM Dispatcher & Cutscene Lock (`0x00F788..0x00F8C6`)

## ROM Source & Evidence
- ROM routine: `0x00F788..0x00F8C6`
- Evidence JSON: `data/thor-evidence/rom_event_script_vm_m15_365.json`
- Extractor: `tools/extract_rom_event_script_vm.py`
- Python tests: `tests/rom_event_script_vm_test.py`

## Exact Behavioral Contract
1. **Top-Level Guards (`0x00F788..0x00F7B2`)**:
   - Skips completely if `$FF17B8` bit 1 (time-stop) is set.
   - If `$FF17B6` (`wait_timer > 0`), decrements it by 1 and returns.
   - If `$FF17BD` (`cutscene_lock > 0`), jumps to `0x00F894`.
2. **Cutscene Lock Tick (`0x00F894..0x00F8C6`)**:
   - Dispatches `$FF183A` via `0x00F8C8`, decrements `$FF17BD`; when `$FF17BD` reaches `0`, clears `$FF1982 = 0`, `$FF19EC = 0`, and `$FF1A12 = 0`.
3. **VM Bytecode Loop (`0x00F7B4..0x00F7CF`)**:
   - Loads `pc = *(u32*)$FF17B2`, reads byte `op = *pc`, dispatches `0x00F7D0[op]`, stores updated `pc` back to `$FF17B2`, and loops until an opcode handler pops/yields or sets `$FF17B6`/`$FF17BD`.
   - `31` even-aligned opcodes (`0x00..0x1E`).
