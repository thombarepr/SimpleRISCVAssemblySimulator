bkpt
movw r0, #0
bkpt
movw r0, #16
bkpt
add r0, r0, #20
bkpt
sub r0, r0, #10
bkpt
and r0, r0, #24
halt
 add r2, r1, r0
d:
  sub r0, r1, #18
	ldr r0, [r1]
	bne d
