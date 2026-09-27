# M12 Dynamic SAT Caller-Side A0 Provenance V1 — PASS

The accepted dynamic tail is entered by one runtime-observed caller:

    0x03B448 JSR.L ($0000B730).L
    0x00B730 entry A0
    0x00B73C MOVE.W (A0)+,D5   ; A0 += 2
    0x00B768 MOVE.W 2(A0),D6

The caller-side A0 chain is:

    0x03B416 LEA.L ($04C6,PC),A0       -> 0x0003B8DE
    0x03B41A MOVE.W ($00FFAFAE).L,D0
    0x03B420 ASL.W #4,D0
    0x03B422 MOVEA.L 12(A0,D0.W),A0
    0x03B426 MOVEA.L (A0),A0
    0x03B428 MOVE.W 8(A6),D0
    0x03B42C ADD.W D0,D0
    0x03B42E ADDA.W D0,A0
    0x03B436 ADDA.W (A0),A0
    0x03B448 JSR.L ($0000B730).L

At frame 1500, A0 is 0x00174358 at tail entry and 0x0017435A at
0x00B768; the only in-tail change is the postincrement at 0x00B73C. The
source word for the A0 definition is the ROM word at the dynamic A0 address
before 0x03B436 (frame 1500: 0x001742E2 = 0x0076), which yields 0x00174358.

The caller was observed 763 times; all 763 entries used 0x03B448. The
provenance rows validate across all 23 accepted transitions. The separate
0x00B6AA setup ends at 0x00B72E RTS, so it does not fall through to
0x00B730 and is classified DOES_NOT_REACH_DYNAMIC_TAIL.

The selector remains unassigned semantically. Its one-level source is the
8(A6) parameter consumed at 0x03B428, with the static ROM pointer chain
through 0x0003B90A and 0x0003B982; no PLAYER/entity label is assigned.
SOURCE_OWNED remains 1,487,672 with delta zero. No commit or push was
performed.
