# M12 Dynamic SAT ROM Index / A0 Provenance V1 — STOP

Scope: trace the accepted dynamic SAT producer's `A0` and ROM-record selection
without assigning PLAYER or gameplay-object semantics.

## Exact static evidence

The bounded decode of `0x00B6AA..0x00B72E` proves a separate setup entry:

```text
0x00B6AA MOVEA.L ($0003F326).L,A0
0x00B6BC ADD.W D0,D0
0x00B6BE ADDA.W D0,A0
0x00B6C2 MOVE.W (A0),D5
0x00B6C8 ADDA.W D5,A0
0x00B6CA MOVE.W (A0)+,D5
0x00B724 ADDQ.L #6,A0
```

The ROM image contains `0x00154B98` at absolute address `0x0003F326`.
The accepted dynamic tail begins at `0x00B730`, where `A0` is an input to
the routine; no local `A0` definition occurs in `0x00B730..0x00B78C`.
Runtime capture confirms the tail's `A0` values, including
`0x0017435A` at frame 1500 for SAT entry 0, but does not execute the
`0x00B6AA` setup entry on that path.

The proven field remains:

```text
0x00B768 MOVE.W 2(A0),D6
0x00B76C ADD.W D3,D6
0x00B76E MOVE.W D6,4(A1)
```

Therefore `0x0017435C` is exactly `A0 + 2`, and the X result is still
validated over 23 transitions. The ROM table base, record stride, record
index, and caller-side reaching definition of `A0` remain unresolved.

## Caller boundary

Static absolute transfers to `0x00B730` were found at `0x000DEA`,
`0x000E3A`, `0x000EFE`, `0x03B448`, and `0x03CE86`. Their caller-side
parameter construction was not closed in this bounded pass, so they are
retained as caller candidates only.

`SOURCE_OWNED` is unchanged at `1,487,672`; PLAYER remains unassigned.
No commit or push was performed.
