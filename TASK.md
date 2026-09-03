# Current Task

TASK: M11.5 — Third bounded 68000 dynamic tracing PoC
WHY: Test whether a minimal isolated execution backend can resolve selected
unresolved static evidence without becoming an emulator or runtime dependency.
CURRENT MILESTONE: M11.5
MILESTONE UNDERSTANDING CONFIDENCE: 95%
CURRENT SLICE UNDERSTANDING CONFIDENCE: 95%
SLICE MODE: RE_TOOLING_ONLY
STATUS: COMPLETE

## Scope

This is a bounded tooling checkpoint, not a gameplay/runtime milestone. The
tool is research infrastructure only and must not become part of the native
game runtime.

Selected target/scenario: existing `0xA6A4` slice, controlled start at
`0xA7D4`, raw `A6=FF2954`, record word `1`, and raw `+0x22` pointer to
`0xA7E4`. Addresses remain raw evidence, not semantic names.

## Acceptance criteria

- [x] use the user-supplied canonical USA ROM only; do not commit ROM bytes or extracted commercial assets;
- [x] add an isolated bounded execution backend separate from `oasis_core` gameplay runtime;
- [x] record executed PCs/blocks, branch outcome, return, RAM reads and indirect target;
- [x] capture only relevant `A6`/`A0` register state for unresolved addressing/control flow;
- [x] emit deterministic `oasis.m68k.re-trace.v1` JSON and a human report;
- [x] compare static confirmed/dynamic/newly-resolved/still-unresolved evidence by address;
- [x] resolve at least one prior unresolved item without semantic invention;
- [x] add synthetic tests, USA oracle, documentation, Debug/Release CTest and hygiene checks;
- [x] keep every source/document file <= 500 lines;
- [x] Debug/Release builds and CTest are green.

## Result

The bounded scenario executes `A7D4, A7DA, A7DE, A7E2, A7E4`: 5 instructions,
3 basic blocks, one not-taken branch, two RAM reads, one return and indirect
target `A7E2 -> A7E4`. It resolves three static items: the two register-based
memory references at `A7D4/A7DE` and the indirect control-flow target at `A7E2`;
9 static items remain unresolved. No production gameplay behavior was added.

## Hard boundaries

- Do not build a full Motorola 68000 emulator.
- Do not build a general-purpose Mega Drive emulator.
- Do not attempt whole-ROM recompilation in this checkpoint.
- Do not add generated 68000 instruction-by-instruction code to the native gameplay runtime.
- Do not invent semantic names for unknown routines, RAM fields, driver commands or event meanings.
- Do not start M12 implementation.
- Do not add a broad dependency/framework unless a narrow experiment proves it is required.

## Known unknowns

Driver command meanings, audio protocol, event/progression semantics and the
producer caller remain UNKNOWN. The backend supports only the exact scenario
opcodes and stops explicitly on other opcodes, does not model calls or writes
in this scenario, and is not a generic 68000 decoder/emulator or full-game
tracing system. The nine unobserved static items remain unresolved.

## Exact next action

Stop at this verified checkpoint and await explicit instruction before full-game
tracing, TAS integration, similarity search, whole-ROM discovery, recompilation
or M12 work.
