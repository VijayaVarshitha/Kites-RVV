**VECTOR EXTENSION - MANUAL TEST RESULTS**



**Simulator:** Our Kites build (code\\Kites-1.1.8\\KitesApp\\Kites.exe)



**How each test was run:** 

File -> Open the .s file -> Assemble -> pick processor-> Run -> read registers / memory / Profiler.



Screenshots are in screenshots\\ named testNN\_<processor>.png



**Processors (Processor button in Kites):**

&#x20; SC   = single cycle

&#x20; HF   = 5-stage, hazard detection + forwarding

&#x20; HNF  = 5-stage, hazard detection, no forwarding

&#x20; NHF  = 5-stage, no hazard detection, forwarding

&#x20; NHNF = 5-stage, no hazard detection, no forwarding



**Note on NHNF:** 

This processor has no hazard detection and no forwarding, so a

program only works if the programmer puts nops between dependent instructions.

Our test programs have no nops, so on NHNF tests 2, 3, 9-15 are EXPECTED to give

wrong values (this is how that processor is designed, not a bug in our vector

code).

Tests 1 and 4-8 give the correct values on all five processors.

Values are in hex unless noted.

Floats are shown as bit patterns

(0x3f800000 = 1.0, 0x40000000 = 2.0).



For each test write what you saw after "Actual", and PASS or FAIL.





**TEST 1 - test01\_vsetvli.s - how vsetvli sets VL**

&#x20; Expected: t1=4  t2=4  t3=2  t4=10  t5=16  t6=8 (decimal); at the end VL=8, SEW=16

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : as expected

&#x20; Result      : PASS   



**TEST 2 - test02\_load\_store.s - vle32.v / vse32.v**

&#x20; Expected: v1 = 10, 20, 30, 40 (decimal); t3 = 10, t4 = 40

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : different values, as expected (no nops on this processor)

&#x20; Result      : PASS



**TEST 3 - test03\_element\_widths.s - e8 / e16 / e64, 8-bit wraparound**

&#x20; Expected (SEW is 64 at the end, 2 values per register):

&#x20;   v1 = v3 = v4 = fefdfcfbfa030201, 3c32281e140a00ff

&#x20;   v2 = 0e0d0c0b0a131211, 4c42382e241a100f

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : different values, as expected (no nops on this processor)

&#x20; Result      : PASS



**TEST 4 - test04\_add\_sub.s - vadd / vsub (.vv and .vx)**

&#x20; Expected: v3 = 8, v4 = fffffffe (-2), v5 = 69 (105), v6 = ffffffa1 (-95)

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : as expected

&#x20; Result      : PASS



**TEST 5 - test05\_mul.s - vmul keeps the low 16 bits**

&#x20; Expected: v1 = 12c (300), v2 = 5f90, v3 = fda8 (-600)

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : as expected

&#x20; Result      : PASS



**TEST 6 - test06\_macc.s - vmacc.vv / vmacc.vx**

&#x20; Expected: v3 = 55 (85) in all 4 elements

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : as expected

&#x20; Result      : PASS



**TEST 7 - test07\_moves.s - vmv.v.x / vmv.v.v / vmv.v.i**

&#x20; Expected: v1 = v2 = 2a (42), v3 = ffffffff, v4 = f, v5 = fffffff0

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : as expected

&#x20; Result      : PASS



**TEST 8 - test08\_tail\_undisturbed.s - elements past VL stay the same**

&#x20; Expected: v1 = 2, 2, 9, 9; t1 = 2

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : as expected

&#x20; Result      : PASS



**TEST 9 - test09\_pipeline\_hazard.s - lw/addi used by the next vector instruction**

&#x20; Expected: v2 = 8 (all), v3 = 60 (96) (all); same on SC, HF and HNF

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : different values, as expected (no nops on this processor)

&#x20; Result      : PASS



**TEST 10 - test10\_fp\_add\_sub\_mul.s - vfadd / vfsub / vfmul**

&#x20; Expected: v3 = v6 = 40600000 (3.5), v4 = v7 = bf000000 (-0.5),

&#x20;           v5 = 40400000 (3.0), v8 = 40100000 (2.25)

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : different values, as expected (no nops on this processor)

&#x20; Result      : PASS



**TEST 11 - test11\_fp\_macc.s - vfmacc.vv / .vf, vfmv.v.f**

&#x20; Expected: v1 = 40400000 (3.0), v2 = 41980000 (19.0)

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : different values, as expected (no nops on this processor)

&#x20; Result      : PASS



**TEST 12 - test12\_fp\_special\_values.s - NaN, infinity, overflow**

&#x20; Expected: v2 = v3 = 7fc00000 (NaN), v5 = 7f800000 (infinity)

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : different values, as expected (no nops on this processor)

&#x20; Result      : PASS



**TEST 13 - test13\_fp\_double.s - e64 doubles**

&#x20; Expected: v2 = 4010000000000000 (4.0), v3 = 4024000000000000 (10.0)

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : different values, as expected (no nops on this processor)

&#x20; Result      : PASS



**TEST 14 - test14a\_fir\_scalar\_int.s vs test14b\_fir\_vector\_int.s - integer FIR**

&#x20; Expected: output 51, 68, 85 ... 578 for both (Memory tab, "output")

&#x20;           Profiler instructions: 1319 (scalar) vs 319 (vector)

&#x20;           HF cycles: 1704 (scalar) vs 416 (vector)

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : different values, as expected (no nops on this processor)

&#x20; Result      : PASS



**TEST 15 - test15a\_fir\_scalar\_fp.s vs test15b\_fir\_vector\_fp.s - float FIR**

&#x20; Expected: output 13.0, 17.25, 21.5 ... 144.75 for both (13.0 = 41500000)

&#x20;           Profiler instructions: 1191 (scalar) vs 320 (vector)

&#x20;           HF cycles: 1576 (scalar) vs 417 (vector)

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected

&#x20; Actual HNF  : as expected

&#x20; Actual NHF  : as expected

&#x20; Actual NHNF : different values, as expected (no nops on this processor)

&#x20; Result      : PASS



**UNDO - any test**

&#x20; Expected: step a few times, press undo, the vector registers go back

&#x20; Actual SC   : as expected

&#x20; Actual HF   : as expected





**Screenshots are in screenshots\\ named testNN\_<processor>.png**



**SUMMARY:** 16 of 16 checks passed on SC, HF, HNF and NHF.

&#x20;        On NHNF, tests 1 and 4-8 passed; the others need nops (expected).



**Automatic tests (Google Test) run on our PC:**

&#x20; 1024 / 1024 vector tests passed      (..\\google\_tests\\google\_tests\_passed\_screenshot.jpeg)

&#x20; 158 / 158 original Kites tests passed (..\\old\_tests\_passed\_screenshot.jpeg)



