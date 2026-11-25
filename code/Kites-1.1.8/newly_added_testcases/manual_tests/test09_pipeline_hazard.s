# Test 9: scalar value made just before a vector instruction (pipeline hazard)
.data
val: .word 7
.text
li t0, 4
vsetvli x0, t0, e32, m1, tu, mu
vmv.v.i v1, 1
la t1, val
lw t2, 0(t1)              # load
vadd.vx v2, v1, t2        # uses t2 right away -> 8
addi t3, t2, 5            # 12
vmul.vx v3, v2, t3        # uses t3 right away -> 96
