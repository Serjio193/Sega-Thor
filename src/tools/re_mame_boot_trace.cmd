focus maincpu
temp0=0
trace build/re_emulator_local/mame-instruction.log,maincpu,noloop,{tracelog "OASIS_EVENT seq=%d pc=0x%08X kind=instruction d0=0x%08X d1=0x%08X d2=0x%08X d3=0x%08X d4=0x%08X d5=0x%08X d6=0x%08X d7=0x%08X a0=0x%08X a1=0x%08X a2=0x%08X a3=0x%08X a4=0x%08X a5=0x%08X a6=0x%08X a7=0x%08X sr=0x%04X\n",temp0,pc,d0,d1,d2,d3,d4,d5,d6,d7,a0,a1,a2,a3,a4,a5,a6,sp,sr}
rp {++temp0 >= #512},{traceflush; quit}
g
