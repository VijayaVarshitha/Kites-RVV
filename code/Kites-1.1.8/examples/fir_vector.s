# FIR filter, VECTOR version (RVV subset: vsetvli, vle32, vmul.vx, vadd.vv, vse32)
# out[n] = c0*in[n] + c1*in[n+1] + c2*in[n+2] + c3*in[n+3],  n = 0..31
# Expected output: [51, 68, 85, 102] ... 578
# Computes VLMAX = 4 outputs (e32, m1, VLEN = 128) per strip-mined iteration.
.data
input:  .word 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34, 35
coeffs: .word 2, 3, 5, 7
output: .zero 128

.text
    la   a0, input          # a0 = &in[n]
    la   a1, coeffs         # a1 = &c[0]
    la   a2, output         # a2 = &out[n]
    li   t0, 32             # outputs remaining

strip:
    vsetvli t1, t0, e32, m1, tu, mu   # t1 = vl = min(remaining, 4)
    vmul.vx v1, v1, x0      # acc = 0 (active lanes)
    mv   t2, a0             # input pointer for this strip
    mv   t3, a1             # coefficient pointer
    li   t4, 4              # taps remaining
tap:
    lw   t5, 0(t3)          # scalar coefficient c[k]
    vle32.v v2, (t2)        # in[n+k .. n+k+vl-1]
    vmul.vx v2, v2, t5      # * c[k]
    vadd.vv v1, v1, v2      # acc += ...
    addi t2, t2, 4
    addi t3, t3, 4
    addi t4, t4, -1
    bnez t4, tap

    vse32.v v1, (a2)        # out[n .. n+vl-1] = acc
    slli t6, t1, 2          # bytes processed = vl * 4
    add  a0, a0, t6
    add  a2, a2, t6
    sub  t0, t0, t1
    bnez t0, strip
