focus maincpu
wp ff0000,10000,w,1,{printf "OASIS_WRITE PC=%08X ADDR=%08X DATA=%08X SIZE=%d\n",pc,wpaddr,wpdata,wpsize; quit}
temp0=0
rp {++temp0 >= #8192},{printf "OASIS_WRITE_TIMEOUT\n"; quit}
g
