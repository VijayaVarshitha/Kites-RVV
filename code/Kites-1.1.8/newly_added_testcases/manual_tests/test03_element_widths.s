# Test 3: other element widths (e8, e16, e64)
.data
bytes: .byte 1, 2, 3, 250, 251, 252, 253, 254, 255, 0, 10, 20, 30, 40, 50, 60
out8:  .zero 16
.text
la t0, bytes
la t1, out8
li t2, 16
vsetvli x0, t2, e8, m1, tu, mu
vle8.v v1, (t0)
vadd.vx v2, v1, t2        # +16 to each byte (wraps past 255)
vse8.v v2, (t1)
li t2, 8
vsetvli x0, t2, e16, m1, tu, mu
vle16.v v3, (t0)          # same 16 bytes seen as 8 halfwords
li t2, 2
vsetvli x0, t2, e64, m1, tu, mu
vle64.v v4, (t0)          # same 16 bytes seen as 2 doublewords
