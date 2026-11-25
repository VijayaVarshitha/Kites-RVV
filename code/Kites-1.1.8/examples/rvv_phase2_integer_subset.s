# Two-tap integer FIR fragment using only the selected RVV instruction set.
# Four lanes produce four adjacent output samples.
.data
input:        .word 1, 2, 3, 4, 5
coefficients: .word 2, 3
output:       .zero 16

.text
main:
    addi a0, x0, 4
    vsetvli t0, a0, e32, m1, tu, mu
    la a1, input
    la a2, coefficients
    la a3, output

    # v1 = [0, 0, 0, 0]
    vmul.vx v1, v1, x0

    # Tap 0: add input[n] * coefficients[0]
    lw t1, 0(a2)
    vle32.v v2, (a1)
    vmul.vx v2, v2, t1
    vadd.vv v1, v1, v2

    # Tap 1: advance the input window and add input[n+1] * coefficients[1]
    addi a1, a1, 4
    addi a2, a2, 4
    lw t1, 0(a2)
    vle32.v v2, (a1)
    vmul.vx v2, v2, t1
    vadd.vv v1, v1, v2

    vse32.v v1, (a3)
    ebreak
