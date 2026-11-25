NEWLY ADDED TEST CASES (VECTOR EXTENSION)
=========================================

Everything in this folder tests what we added to Kites. The original Kites tests
are untouched in ..\test\.

  manual_tests\    17 .s programs (15 tests) to run by hand in the Kites window
      test01 ... test15b         the programs
      TEST_RESULTS.md            expected answers + columns for what you saw
      screenshots\               screenshots of our runs
  google_tests\    1024 automatic Google Tests
      rvv_vector_tests.cpp       all 1024 tests in one file
      CMakeLists.txt             tells CMake how to build the tests
      run_google_tests.bat       one click: build + run + save results


THE 15 MANUAL TESTS
-------------------
  1   test01_vsetvli.s             vsetvli: VL from AVL and SEW
  2   test02_load_store.s          vle32.v / vse32.v
  3   test03_element_widths.s      e8 / e16 / e64, 8-bit wraparound
  4   test04_add_sub.s             vadd, vsub (.vv and .vx)
  5   test05_mul.s                 vmul (.vv and .vx), low SEW bits kept
  6   test06_macc.s                vmacc (.vv and .vx)
  7   test07_moves.s               vmv.v.x, vmv.v.v, vmv.v.i
  8   test08_tail_undisturbed.s    elements past VL stay unchanged
  9   test09_pipeline_hazard.s     scalar made just before a vector instruction
  10  test10_fp_add_sub_mul.s      vfadd, vfsub, vfmul (.vv and .vf)
  11  test11_fp_macc.s             vfmacc (.vv and .vf), vfmv.v.f
  12  test12_fp_special_values.s   inf-inf and inf*0 give NaN, overflow gives inf
  13  test13_fp_double.s           e64 double precision
  14  test14a / test14b            integer FIR: scalar vs vector
  15  test15a / test15b            float FIR: scalar vs vector
  Undo: step a few times in any test, press undo, registers go back.

How to run: open the simulator, File -> Open -> manual_tests\testNN_....s,
pick a processor (single cycle, then 5-stage hazard + forwarding), assemble, run,
and compare with TEST_RESULTS.md.


THE 1024 GOOGLE TESTS (google_tests\rvv_vector_tests.cpp)
--------------------------------------------------------
  16 hand-written tests (RvvManual.*): the 15 manual programs above plus an
     undo test, each run on the single-cycle processor and two pipelines.
  1008 generated tests (Generated/RvvRandom...ProgramMatchesReferenceOnAllProcessors/0..1007):
     each builds a random vector program from its number (same every run) with
     random SEW, random VL (including 0), data loaded with vle, 10 random
     instructions and results stored with vse. Even numbers use the integer
     instructions, odd numbers the floating-point ones with a random rounding
     mode and special values (NaN, inf, +-0). A small reference model in the
     same file works out the right answer, and the program is run on all five
     processors; registers v1-v7, stored memory, VL and FP flags must match.

We checked that the tests catch mistakes: breaking vmacc on purpose makes 275
of the 1024 tests fail.


HOW TO RUN THE GOOGLE TESTS
---------------------------
Easiest: double-click google_tests\run_google_tests.bat

Or in Command Prompt:
  cd <submission>\code\Kites-1.1.8
  set PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;%PATH%
  cmake --preset test
  cmake --build --preset test-only --parallel
  cmake --build build\test --target kites_vector_tests --parallel
  ctest --test-dir build\test -R Rvv -j 8

Expected: 100% tests passed, 0 tests failed out of 1024 (about 20 minutes).

Our result: 1024 / 1024 passed (google_tests\google_tests_passed_screenshot.jpeg)
            158 / 158 original Kites tests passed (old_tests_passed_screenshot.jpeg)
