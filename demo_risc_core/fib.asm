movw r5, #9
movw r0, #0
movw r1, #0
movw r2, #1
more:
add r3, r1, r2
bkpt
and r7, r0, #1
bne d0
mov r1, r3
b d1
d0:
mov r2, r3
d1:
add r0, r0, #1
sub r7, r5, r0
bne more
halt
