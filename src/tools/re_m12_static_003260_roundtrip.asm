; Isolated validation wrapper; external targets are anchored with harmless NOPs.
    org $2CBC
loc_002CBC:
    nop
    org $2D58
loc_002D58:
    nop
    org $3820
loc_003820:
    nop
    include "re_m12_static_003260.asm"
