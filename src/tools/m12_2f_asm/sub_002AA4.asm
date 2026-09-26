; M12 2F MAP-driven executed ASM; canonical ROM eb19bda4982366a2fd43d65ab8a7f9709d83a8cc902c14a682c088c16359c263
    org $2AA4
sub_002AA4:
loc_002AA4:
    move.w #$0,($00A11100).L
loc_002AAC:
    move.b #$0,(A1)+
loc_002AB0:
    asl.b #$2,D2
loc_002AB2:
    andi.w #$C0,D2
loc_002AB6:
    andi.w #$3F,D1
loc_002ABA:
    or.b D1,D2
loc_002ABC:
    not.b D2
loc_002ABE:
    move.b (A1),D1
loc_002AC0:
    eor.b D2,D1
loc_002AC2:
    move.b D2,(A1)+
loc_002AC4:
    and.b D2,D1
loc_002AC6:
    move.b D1,(A1)+
loc_002AC8:
    move.w #$0,(A1)+
loc_002ACC:
    rts
