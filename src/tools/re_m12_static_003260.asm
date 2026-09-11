; Exact source ownership for the bounded static routine 0x003260..0x0032E8.
    org $3260
sub_003260:
loc_003260:
    lea.l ($000031FC).L,A6
loc_003266:
    bsr.w loc_002D58
loc_00326A:
    lea.l ($00150000).L,A0
loc_003270:
    lea.l ($00FF2FA8).L,A1
loc_003276:
    movea.l A1,A2
loc_003278:
    bsr.w loc_003820
loc_00327C:
    move.w (A2)+,D6
loc_00327E:
    lea.l 0(A2,D6.W),A2
loc_003282:
    lsr.w #$1,D6
loc_003284:
    ori.w #$700,SR
loc_003288:
    movea.l ($00FF1892).L,A0
loc_00328E:
    move.l #$007F97D5,(A0)+
loc_003294:
    move.w #$20,(A0)+
loc_003298:
    move.w D6,(A0)+
loc_00329A:
    move.l A0,($00FF1892).L
loc_0032A0:
    andi.w #$F9FF,SR
loc_0032A4:
    bset.b #$1,($00FF164E).L
loc_0032AC:
    movea.l A2,A6
loc_0032AE:
    move.w (A6)+,D7
loc_0032B0:
    move.w (A6)+,D6
loc_0032B2:
    move.l #$461C0003,D5
loc_0032B8:
    bsr.w loc_002CBC
loc_0032BC:
    move.w #$28,($00FF10B2).L
loc_0032C4:
    move.w #$F0,($00FF10B0).L
loc_0032CC:
    move.b #$A,($00FF18A4).L
loc_0032D4:
    bset.b #$1,($00FF164D).L
loc_0032DC:
    btst.b #$1,($00FF164D).L
loc_0032E4:
    bne.s loc_0032DC
loc_0032E6:
    rts
