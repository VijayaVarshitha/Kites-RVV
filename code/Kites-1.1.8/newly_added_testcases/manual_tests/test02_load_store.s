# Test 2: vle32.v / vse32.v (vector load and store)
.data
src: .word 10, 20, 30, 40
dst: .zero 16
.text
la t0, src
la t1, dst
li t2, 4
vsetvli x0, t2, e32, m1, tu, mu
vle32.v v1, (t0)
vse32.v v1, (t1)
lw t3, 0(t1)
lw t4, 12(t1)
