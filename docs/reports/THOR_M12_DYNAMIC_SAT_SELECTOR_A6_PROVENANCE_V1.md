# M12 Dynamic SAT Selector / A6 Provenance V1 — PASS

The selector read is exact:

    0x03B376 LEA.L ($00FFAFCE).L,A6
    0x03B428 MOVE.W 8(A6),D0
    0x03B42C ADD.W D0,D0
    0x03B42E ADDA.W D0,A0
    0x03B436 ADDA.W (A0),A0
    0x03B448 JSR.L ($0000B730).L

For frame 1500, A6 is `0x00FFAFCE`, field address is `0x00FFAFD6`, and
`8(A6)=3`. The most recent observed field writer is
`0x03B3D8 MOVE.W 0(A0,D0.W),8(A6)`, with ROM source `0x003BE82` and value
`3`. No gameplay/entity meaning is assigned.

The selector-to-record mapping is exact:

    ROM pointer 0x0003B90A -> 0x0003B982 -> 0x001742DC
    0x001742DC + (3 * 2) = 0x001742E2
    ROM[0x001742E2] = 0x0076
    0x001742E2 + 0x0076 = 0x00174358

Thus the tail entry A0 is `0x00174358`; `0x00B73C` post-increments it to
`0x0017435A`, and the accepted X source is `0x0017435C`.

The 23 validated transitions contain A6 values `0x00FFAFCE` and
`0x00FFAFE6`, selector values 1 through 19, and distinct observed ROM record
addresses. The observed A6 spacing is `0x18`; full enclosing record semantics
remain intentionally unassigned. The selector update block has a bounded
backedge at `0x03B404` to `0x03B3B2`, with `10(A6)` advanced by four bytes,
but total iteration count is not promoted.

`RAM_TO_ROM_SELECTOR_CHAIN=EXACT`; the first gameplay-render-record
classification remains unassigned. PLAYER is not labelled and
`SOURCE_OWNED` remains `1,487,672` with zero delta. No commit or push was
performed.
