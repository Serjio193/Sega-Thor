# M12 Dynamic SAT Shadow Mutation Discovery V1

Date: 2026-09-22
Result: `STOP_NO_DYNAMIC_SAT_GROUP_IDENTIFIED`

## Scope and accepted boundary

This report audits the accepted `0x00FF13CC` SAT-shadow candidate only. It
does not modify Worker capture, assign gameplay/player semantics, broaden the
search to unrelated systems, or change `SOURCE_OWNED`.

The canonical ROM is SHA-256
`eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263`.
The accepted static relation remains:

`0x00A36C -> D2 -> 0x00A370 -> 0x00A372 -> 0x00FF13CC -> DMA -> VRAM 0xD000`.

## Writer census

The bounded direct-absolute scan and the statically closed indirect contracts
produce the following writer set:

```text
SAT_SHADOW_WRITER_PCS =
0x0003FE, 0x001FD0, 0x002B6E, 0x003330, 0x006026,
0x00A372, 0x00A37A, 0x00A4FE, 0x00CD1A,
0x03BFA4, 0x03C904, 0x03CB2E, 0x03E954
```

`0x00A372`, `0x00A37A`, and `0x00A4FE` are classified as template/fixed-root
copies. The remaining entries are startup, reset, or subsystem-clear writes.
No `POST_TEMPLATE_PATCH` writer and no unknown writer inside the bounded
direct-write census was found. The only runtime callback identity at `0xA374`
is normalized to the preceding `0xA372` post-store/next-fetch contract; it is
not promoted to a second writer.

The coverage boundary is explicit: direct absolute 68K write forms are
scanned across the canonical ROM; A5/A6 register-based writers are included
only where their address contract is statically closed. Unexecuted indirect
aliases outside those contracts remain unresolved.

## Runtime and DMA result

Targeted run `1790106191` showed stable template-field values over deliberate
RIGHT, LEFT, UP, DOWN, and ACTION phases. The preserved runtime writer set
contains no input-dependent post-template patch. The accepted refresh order is
template copy at `0xA372/0xA37A`, followed by the observed DMA setup at
`0x27DE/0x27EA`, source `0xFF13CC`, destination `0xD000`; no patch event is
present between copy and DMA.

All preserved D000 SAT DMA records use source `0xFF13CC`. The observed
`0xFF134C` transfer is to CRAM `0x0000`, not SAT VRAM, so it is not an
alternative dynamic SAT group. No different dynamic SAT source group is
identified in the preserved corpus.

The caller/register boundary is also negative for gameplay provenance:
 A0/A6 select ROM roots, A5 targets the shadow plus `FF188C`, and D0/D3/D5
serve loop/index/control roles. No controller register, gameplay-derived
position/state parameter, or exact upstream RAM source is proven at the
producer call.

Therefore:

```text
FF13CC_GROUP_CLASS = TEMPLATE_ONLY
POST_TEMPLATE_PATCH_WRITERS = []
DYNAMIC_SAT_GROUP_CANDIDATES = []
PLAYER_LABEL_PROVEN = NO
ENTITY_ROLE = ENTITY_CANDIDATE
SOURCE_OWNED_BEFORE = 1487672
SOURCE_OWNED_AFTER  = 1487672
SOURCE_OWNED_DELTA  = 0
COMMIT_CREATED = NO
PUSH_PERFORMED = NO
PASS_DYNAMIC_SAT_SHADOW_MUTATION_DISCOVERY_V1 = STOP_NO_DYNAMIC_SAT_GROUP_IDENTIFIED
```

## Deterministic artifacts

The payload-free analyzer is
`src/tools/m12_dynamic_sat_shadow_discovery.py`; its focused tests are
`tests/m12_dynamic_sat_shadow_discovery_test.py`. The generated five-artifact
receipt is under the ignored `build/thor-evidence/` output directory and keeps
the negative result reproducible without adding ROM or extracted assets.
