# M11.53 — Second portable native routine

## Result

STATUS: SECOND_PORTABLE_NATIVE_ROUTINE_PROVEN

The selected second routine is the bounded ROM leaf 0x604BC..0x604E6. Its
portable semantics are extracted into oasis_core; its ROM identity, canonical
opcode/extension validation, GPGX instruction bridge and return/prefetch
handoff remain in the developer-only hybrid adapter. The generated/interpreter
path remains the shadow oracle and fail-closed fallback.

## Baseline gate

The baseline was run before production-code changes, twice for each mode,
against the canonical USA ROM and pinned instrumented GPGX from M11.52.

| identity / count | EMULATED A/B | M11.52 native A/B |
| --- | ---: | ---: |
| checkpoint aggregate | 251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d | identical |
| video sequence | 5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58 | identical |
| total guest instructions | 6,488,773 | 6,488,773 |
| TableCopy native instructions | 0 | 34 |
| fallback | 6,488,773 | 0 |
| deterministic A/B | exact | exact |

The gate passed, so candidate work continued.

## Candidate inventory and selection

The inventory combines the exact USA disassembly/CFG, M11.44–M11.49 ledger and
provenance, existing semantic adapters, safe-memory classification and the
M11.52 continuation contract. No new gameplay or whole-ROM search was used.

| candidate | range / exit | calls / represented instructions | memory and control | unresolved / continuation | classification |
| --- | --- | ---: | --- | --- | --- |
| 0x2D66 TableCopy | 0x2D66..0x2D84, RTS to caller | 1 / 34 | ROM source, bounded RAM output, stack; local DBF loop | closed by M11.52 | ROUTINE_CONTRACT_COMPLETE |
| 0x604BC selected | 0x604BC..0x604E6, RTS at 0x604E4 | 4 / 40 | main RAM flags/output and caller stack; no I/O | closed in this milestone; no calls, indirect edges or loops | ROUTINE_CONTRACT_COMPLETE |
| 0x61032 | 0x61032..0x610C8, direct RTS | 9 / existing adapter coverage | bounded RAM/table transform and one indirect data read | old adapter had lump-sum bus/refresh handoff; full routine extraction not selected | CONTINUATION_BLOCKED |
| 0x6121A | bounded leaf | 5 | reaches VDP address 0xC00011 | hardware-visible ordering | HARDWARE_BLOCKED |
| 0x3820 | 0x3820..0x3B3E, multiple returns/interrupt points | natural calls documented separately | ROM/RAM decompressor and stack | CCR.X, prefetch and interrupt contract remains separate | CONTINUATION_BLOCKED |
| broad 0x38DA/0x3A00 slices | no single closed routine range | not promoted | mixed graphics/data paths | semantic closure incomplete | SEMANTICS_BLOCKED |
| M11.47 isolated 0x00026A, 0x06193C, 0x061954 | single forms | ledger evidence only | safe-memory observations | no complete caller/exit contract | ROUTINE_CONTRACT_PARTIAL |

0x604BC wins the required priority order: complete CFG, safe RAM/stack
contract, no indirect control flow, no hardware access, independently closed
forms, bounded entry/exit, natural execution, and manageable continuation.
The existing M11.30 shadow evidence is 4/4 with zero divergence.

## Exact routine contract

The exact instruction stream is:

    604BC  LEA  $00FF0628.L,A6
    604C2  BSET #4,0(A6)
    604C8  LEA  $00FF06F2.L,A6
    604CE  BSET #4,0(A6)
    604D4  LEA  5(A5),A0
    604D8  SF   (A0)+
    604DA  SF   (A0)+
    604DC  SF   (A0)+
    604DE  SF   $00FF0016.L
    604E4  RTS

Entry permits an even A7 with a readable longword return address and an A5
inside main RAM. The routine changes A6 to 0x00FF06F2, changes A0 to
A5+5+3, consumes the return longword and advances A7 by four. D0–D7 and
A1–A5 are preserved. BSET reads and writes one byte at each fixed flag
address; it sets bit 4 and updates only Z according to the pre-operation bit.
The three post-increment Scc writes and the absolute Scc write are byte zero
writes. LEA, SF and RTS preserve CCR; the final Z is therefore the result of
the second BSET while N/V/C/X and non-CCR SR bits are preserved. The only
exit is the caller return PC.

The portable contract has opaque entry/step/return tokens and adapter-supplied
addresses. It contains no ROM PC, opcode decoder, GPGX/libretro type,
serialized-state dependency or generated-block dependency.

## Semantic form closure

The independent core vectors cover both BSET input cases (bit clear and
already set), preserved N/V/C/X and SR bits, ordered byte writes, A0 32-bit
wrap, stack/RTS return, invalid token/contract rejection, and event/interrupt
yield plus resume. The adapter regression checks the exact 19-word fetch
sequence, ten begin/finish pairs, all extension words, final registers and
all six writes.

The adapter validates these canonical forms:

    4DF9 00FF 0628       LEA absolute long
    08EE 0004             BSET.B #4,0(A6)
    4DF9 00FF 06F2       LEA absolute long
    41ED 0005             LEA 5(A5),A0
    51D8                  SF.B (A0)+
    51F9 00FF 0016       SF.B absolute long
    4E75                  RTS

No form is promoted by renaming generated code. The portable implementation
uses structured operations and the adapter supplies timing/fetch behavior.

## Hybrid continuation

Every represented instruction executes:

1. exact adapter PC/prefetch seed and opcode/extension fetch;
2. begin_instruction;
3. portable register/memory semantics;
4. finish_instruction;
5. GPGX boundary-reason sampling.

The native adapter is installed through the developer-only block-hook
continuation bridge. A scheduler event returns the exact next opaque token to
GPGX (`2` for event, `3` for interrupt, `4` for trace), and the next host
dispatch resumes the same core continuation; the native proof exercised one
event yield and one resumption.

RTS reads the return address before calling the adapter's return-state bridge,
which restores PC, IR/prefetch metadata and the caller continuation. No
add_cycles or skip_bus_refresh call is used by the second routine. The bridge's
normal instruction timing and refresh processing therefore remain
per-instruction. Natural calls observed no external interrupt inside the
routine; synthetic core vectors prove non-normal boundary yield/resume.

The old 604BC native adapter was intentionally not treated as proof: it used
one lump-sum 1022 cycle update and produced aggregate
0d83d11d6c394842516a0f2517bb153290b664126a51bbeefea7b90c5ff3153a.
The extracted adapter removes that path.

## Authoritative dual-routine proof

The reference run, shadow run and two independent dual native runs used
NATIVE_OVERRIDE 0x2D66,0x604BC for 600 cold-reset neutral frames. The shadow
run had 5/5 comparisons and zero divergence. Both native runs were identical
to the reference on all ten canonical checkpoints and all 600 video frames.

| measure | reference / shadow | native A / native B |
| --- | ---: | ---: |
| checkpoint aggregate | 251fab870a22fe5ac053f626e73413f1ecf83b4c548bfbe572e5ab417f32d38d | identical |
| video sequence | 5e74ec4ef4a0c6891d5c6d60f4f260703c0bc2ebde9b15edea7e4f2ae3437a58 | identical |
| total guest instructions | 6,488,773 | 6,488,773 |
| TableCopy native | 1 call / 34 instructions | 1 / 34 |
| second native | 4 calls / 40 instructions | 4 / 40 |
| interpreter remainder | 6,488,773 | 6,488,699 |
| second-routine boundary yields / resumptions | 0 / 0 | 1 / 1 |
| fallback / divergence | 0 / 0 | 0 / 0 |
| external interrupts in candidates | 0 | 0 |

The dual native call records are:

    0x604BC call 1: entry cycles 189599, exit 190635, 10 instructions
    0x604BC call 2: entry cycles 343846, exit 344868, 10 instructions
    0x604BC call 3: entry cycles 669801, exit 670823, 10 instructions
    0x604BC call 4: entry cycles 100661, exit 101683, 10 instructions
    0x2D66  call 1: entry cycles 193626, exit 196454, 34 instructions

The frozen paired manifest proves CPU/RAM/VDP/sound/interrupt-visible
checkpoint identity and video identity. The native run itself retains the
runner's conservative standalone full_cpu_equivalence=false field because
that field does not load a paired reference manifest; the authoritative claim
is the explicit paired comparison above.

## Separate accounting and coexistence

The dual native total is:

    GENERATED_TRANSLATED       0
    MECHANICAL_PRIMITIVE       0
    NATIVE_ROUTINE_TABLE_COPY 34
    NATIVE_ROUTINE_SECOND      40
    INTERPRETER          6,488,699
                               -----------
    TOTAL                6,488,773

The shared registry routes natural entry PCs to separate metadata and
separate core contracts. Both adapters use the shared generic
fetch/begin/finish/boundary bridge but neither core routine knows the other's
tokens, addresses or state. No singleton, global candidate state or
candidate-specific timing correction was introduced. The deterministic
interleaving is proven by the two identical dual runs.

## Subsystem readiness inventory

This milestone does not implement a subsystem. The bounded readiness view is:

| evidence | readiness |
| --- | --- |
| 0x604BC | ISOLATED_ROUTINE |
| 0x2D66 plus 0x604BC shared registry | RELATED_STRUCTURAL_ROUTINE |
| flag/output main-RAM layout | SHARED_MEMORY_STRUCTURE |
| callers around 0x60BCC | SHARED_CALLER_CLUSTER |
| broader 0x604BC caller region | POSSIBLE_SUBSYSTEM_BOUNDARY |
| gameplay meaning / feature ownership | INSUFFICIENT_EVIDENCE |

No gameplay name or subsystem boundary is inferred from this classification.

## Reproducibility and scope

ROM SHA-256:
eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263.

Pinned GPGX SHA-256:
140c00fc7475cf22ca22415d65dfc7ffc66523de128454f9994e7ab77d9826fd.

All ROM-backed output directories are local evidence only and remain
untracked. No ROM, commercial asset, BIOS or emulator binary was added to the
repository.
