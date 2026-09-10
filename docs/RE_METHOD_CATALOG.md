# Reverse-engineering method catalog

## BUILD_RECONSTRUCTION

- BUILD-03 — Full-ROM ASM completion census. Domain: BUILD_RECONSTRUCTION.
  Evidence: canonical ROM identity, contiguous manifest, source/blob byte
  totals, exact rebuild hashes and an explicit dependency graph. Prerequisites:
  M11.15 exact split, M11.17 bounded data report and local vasm proof.
  Failure/falsification: any gap, overlap, identity mismatch or untracked
  executable blob. Used: M12.0. Future blocker: complete source ownership and
  Z80 map.

This catalog names reusable evidence methods and their limits. It is a method index, not a claim that every domain is solved. A method may produce a storage, control-flow, or provenance fact without supplying a semantic type. External projects are references for method shape only; their game behavior is not imported.

Every new entry must state: **ID/name**, **domain**, **evidence**, **prerequisites**, **failure/falsification**, **already used in Sega-Thor**, and **likely future blocker**. Preserve these rules: storage/compression is not a semantic type; decoder correctness is not encoder equivalence or historical byte equivalence; natural-path proof is not whole-static proof; hardware must remain visible rather than hidden behind mocks.

## CODE

- **CODE-01 — Bounded decode and CFG census.** Domain: CODE. Evidence: decoded instruction/block/branch/call/return inventory with unresolved indirect edges retained. Prerequisites: exact ROM identity and decoder validation. Failure/falsification: illegal decode, missed branch, or new indirect edge. Used: M11.61–M11.63. Future blocker: unresolved static targets such as `0x062CEC`, `0x061F60`, `0x062878`.
- **CODE-02 — Operand and effect classification.** Domain: CODE. Evidence: PC, EA, direction, width, order, path signature, and conservative class. Prerequisites: validated 68K semantics and bounded trace. Failure/falsification: alias, width, or ordering mismatch. Used: M11.63. Future blocker: whole-static effects and indirect CFG.
- **CODE-03 — Callee preservation observer.** Domain: CODE. Evidence: natural entry/return, parent continuation, register equality, nested direct/indirect paths. Prerequisites: controlled scenario and developer-only runner. Failure/falsification: entry/return mismatch, interrupt, divergence. Used: M11.61–M11.63. Future blocker: parent ownership and hardware effects.

## RAM

- **RAM-01 — Raw address provenance and lifetime.** Domain: RAM. Evidence: register materialization, raw reads/writes, parent restore, and lifetime interval. Prerequisites: identity-bound trace and explicit address classes. Failure/falsification: external writer or alias discovered. Used: M11.59–M11.60. Future blocker: ownership outside the interval.
- **RAM-02 — Alias and external-writer census.** Domain: RAM. Evidence: independent writer/reader sites and overlapping ranges. Prerequisites: bounded static plus dynamic coverage. Failure/falsification: incomplete writer set. Used: M11.59 as a blocker. Future blocker: missing scenarios and indirect addressing.
- **RAM-03 — Controlled perturbation and RAM watch.** Domain: RAM. Evidence: state delta attributable to one bounded input or watchpoint. Prerequisites: reversible developer-only harness and stable baseline. Failure/falsification: unrelated deltas or timing shift. Used: candidate M12 method. Future blocker: shared ownership and side effects.
- **RAM-04 — Snapshot differential slicing.** Domain: RAM. Evidence: before/after snapshots intersected with observed access paths. Prerequisites: repeatable checkpoints. Failure/falsification: nondeterministic or hidden DMA effects. Used: candidate M12 method. Future blocker: hardware/cache visibility.

## ROM_DATA

- **ROM-01 — ROM identity and reader correlation.** Domain: ROM_DATA. Evidence: ROM hash, address/width readers, repetition and provenance. Prerequisites: user-supplied ROM only. Failure/falsification: hash mismatch or inferred table boundary. Used: M11 baselines. Future blocker: unknown pointer bases.
- **ROM-02 — Resource/table graph reconstruction.** Domain: ROM_DATA. Evidence: pointer tables, consumers, spans, and cross references. Prerequisites: stable readers and structural validation. Failure/falsification: a graph edge unsupported by runtime or decode. Used: candidate M12 method. Future blocker: compressed/overlay data.

## GRAPHICS

- **GRAPHICS-01 — Asset inspection by bounded readers.** Domain: GRAPHICS. Evidence: tile/palette/layout candidates tied to ROM readers and output. Prerequisites: decoder and format vectors. Failure/falsification: visual match without provenance. Used: earlier asset tooling. Future blocker: unknown table semantics.

## COMPRESSION

- **COMPRESSION-01 — `0x00003820` decoder vectors.** Domain: COMPRESSION. Evidence: deterministic input/output vectors and runtime identity. Prerequisites: exact ROM and independent decoder checks. Failure/falsification: output mismatch; does not prove encoder or historical bytes. Used: active project direction. Future blocker: format variants.

## SOUND

- **SOUND-01 — Conservative driver/resource provenance.** Domain: SOUND. Evidence: bounded command readers, ROM samples/tables, and hardware writes when observed. Prerequisites: source-PC and timing evidence. Failure/falsification: semantic naming without a contract. Used: catalog only. Future blocker: undocumented hardware protocol.

## SCRIPT

- **SCRIPT-01 — Event/resource graph with parser-as-detector.** Domain: SCRIPT. Evidence: structurally justified records and consumers. Prerequisites: repeated shape plus runtime correlation. Failure/falsification: parser that merely makes bytes look typed. Used: catalog only. Future blocker: sparse or indirect dispatch.

## SAVE

- **SAVE-01 — Save/load serialization as RAM oracle.** Domain: SAVE. Evidence: reversible serialized fields mapped to RAM deltas and reload behavior. Prerequisites: controlled save state and independent snapshots. Failure/falsification: field changes not reproduced after reload. Used: candidate M12 method. Future blocker: checksum, device, or version gates.

## COMPARATIVE

- **COMPARATIVE-01 — Method comparison only.** Domain: COMPARATIVE. Evidence: externally published analysis used to select a method or falsification pattern. Prerequisites: source citation and project-specific revalidation. Failure/falsification: importing external semantics. Used: catalog governance. Future blocker: absent matching build.

## DYNAMIC

- **DYNAMIC-01 — Runtime provenance/taint.** Domain: DYNAMIC. Evidence: value origin across ROM, RAM, registers, and hardware-visible writes. Prerequisites: developer-only instrumentation and identity-bound traces. Failure/falsification: taint loss at indirect flow or DMA. Used: M11 natural observers. Future blocker: hardware/cache and dual-CPU effects.
- **DYNAMIC-02 — Repeatable path and timing census.** Domain: DYNAMIC. Evidence: path hashes, ordered events, frame/checkpoint/video hashes, and repeat equality. Prerequisites: cold-reset scenario and fixed input. Failure/falsification: repeat divergence. Used: M11.60–M11.64. Future blocker: scenario coverage.

## VERIFICATION

- **VERIFICATION-01 — Identity/checkpoint/video gate.** Domain: VERIFICATION. Evidence: exact ROM/GPGX hashes, state checkpoint, video sequence, accounting, and shadow comparisons. Prerequisites: reproducible runner. Failure/falsification: any mismatch is STOP. Used: every M11 baseline.
- **VERIFICATION-02 — Native/shadow oracle pair.** Domain: VERIFICATION. Evidence: native replacement plus interpreter shadow, repeated with zero divergence. Prerequisites: bounded contract already proven. Failure/falsification: fallback, divergence, or missing continuation. Used: M11 mechanical family only. Future blocker: portable contract.

## BUILD_RECONSTRUCTION

- **BUILD-01 — Toolchain and artifact identity.** Domain: BUILD_RECONSTRUCTION. Evidence: Debug/Release/UCRT/GNU-equivalent results, binary hashes, and exact commit. Prerequisites: clean reproducible checkout. Failure/falsification: environment failure must be classified, not hidden. Used: M11 validation. Future blocker: local UCRT pre-diagnostic failure.
- **BUILD-02 — Historical reconstruction as evidence.** Domain: BUILD_RECONSTRUCTION. Evidence: documented fingerprints and project-specific matches only. Prerequisites: `docs/RE_TOOLCHAIN_GUIDE.md`; no proprietary SDK retrieval. Failure/falsification: candidate toolchain unsupported by project evidence. Used: governance method. Future blocker: missing preserved inputs.
