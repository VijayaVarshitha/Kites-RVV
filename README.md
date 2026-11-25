PIPELINED VECTOR PROCESSING IN KITES - FINAL SUBMISSION
=======================================================

We added a subset of the RISC-V Vector extension (RVV 1.0) to the Kites RISC-V
simulator (version 1.1.8, Qt 6 / C++20). The vector instructions run on the
single-cycle processor and on all four 5-stage pipelines, with undo/redo and a
performance report (cycles, CPI, stalls).


1. WHAT IS IN THIS FOLDER
-------------------------
  README.txt                    this file
  HOW_TO_RUN.txt                step-by-step commands to build, run and test
  run_and_verify.bat            menu that runs those steps for you
  Kites.bat                     builds the simulator (first time) and opens it
  report\                       project report (also has the presentation
                                feedback and the LLM disclosure)
  presentation\                 our slides
  code\Kites-1.1.8\             full simulator source code with our changes
      src\                      source code
      scripts\                  build and test scripts
      examples\                 example programs (FIR scalar/vector, int/float)
      test\                     original Kites tests (unchanged)
      newly_added_testcases\    OUR tests
          manual_tests\         17 .s programs (15 tests) to run in the Kites window,
                                expected answers (TEST_RESULTS.md), screenshots
          google_tests\         1024 automatic Google Tests (one .cpp file)

As asked, only source code and build scripts are included (no compiled files).


2. VECTOR INSTRUCTIONS WE ADDED
-------------------------------
  Configuration : vsetvli (SEW e8/e16/e32/e64, LMUL m1)
  Memory        : vle8.v vle16.v vle32.v vle64.v, vse8.v vse16.v vse32.v vse64.v
  Integer       : vadd.vv/.vx, vsub.vv/.vx, vmul.vv/.vx, vmacc.vv/.vx,
                  vmv.v.v, vmv.v.x, vmv.v.i
  Floating point: vfadd.vv/.vf, vfsub.vv/.vf, vfmul.vv/.vf, vfmacc.vv/.vf,
  (e32 and e64)   vfmv.v.f

VLEN = 128 bits, 32 vector registers v0-v31, unmasked instructions only.
Floating-point instructions use the rounding mode in frm, set the flags in
fflags, return the standard NaN, and vfmacc rounds only once (fused).


3. WHAT YOU NEED (WINDOWS)
--------------------------
  Qt 6 with the MinGW compiler, CMake and Ninja (all from the Qt installer).
  The paths used are:
      C:\Qt\6.12.0\mingw_64
      C:\Qt\Tools\mingw1310_64
      C:\Qt\Tools\Ninja
      C:\Qt\Tools\CMake_64
  If your Qt is somewhere else, change the three C:/Qt/... paths in the "base"
  section of code\Kites-1.1.8\CMakePresets.json, and the QT= line at the top of
  code\Kites-1.1.8\scripts\build_app.bat.


4. HOW TO BUILD AND OPEN THE SIMULATOR
--------------------------------------
Easiest: double-click Kites.bat in this folder.
  The first time it builds the simulator (a few minutes) and puts a ready-to-run
  copy in code\Kites-1.1.8\KitesApp\ together with a "Kites" shortcut.
  After that, Kites.bat (or the Kites shortcut in code\Kites-1.1.8) opens it at once.

By hand (Command Prompt):
  cd code\Kites-1.1.8
  scripts\build_app.bat


5. HOW TO RUN THE TESTS
-----------------------
Open Command Prompt and run:

  cd <this folder>\code\Kites-1.1.8
  set PATH=C:\Qt\Tools\CMake_64\bin;C:\Qt\Tools\Ninja;C:\Qt\Tools\mingw1310_64\bin;C:\Qt\6.12.0\mingw_64\bin;%PATH%
  cmake --preset test
  cmake --build --preset test-only --parallel
  cmake --build build\test --target kites_vector_tests --parallel

Our 1024 vector tests:
  ctest --test-dir build\test -R Rvv -j 8
  Expected: 100% tests passed, 0 tests failed out of 1024   (about 20 minutes)

Original Kites tests + ours together:
  ctest --test-dir build\test -j 8
  Expected: 100% tests passed (158 original + 1024 ours)

One-click alternative for our tests:
  double-click code\Kites-1.1.8\newly_added_testcases\google_tests\run_google_tests.bat

Note: the .cpp test file cannot be run by itself. It has to be compiled together
with the simulator and Google Test, which is what the commands above do.

Manual tests:
  Open the simulator, File -> Open -> newly_added_testcases\manual_tests\testNN_....s,
  pick a processor, assemble and run, and compare with manual_tests\TEST_RESULTS.md.


6. FIR BENCHMARK RESULTS (32 outputs, 4 taps)
---------------------------------------------
                                  Instructions   Cycles (hazard+fwd)   Cycles (hazard, no fwd)
  fir_scalar.s        (integer)       1319            1704                  2446
  fir_vector_macc.s   (vmacc.vx)       319             416                   560
  fir_scalar_fp.s     (fmadd.s)       1191            1576                  2446
  fir_vector_fp.s     (vfmacc.vf)      320             417                   561

The vector versions give the same output with about 4x fewer instructions and
3.8x - 4.4x fewer cycles.


7. WHERE OUR CODE IS  (paths inside code\Kites-1.1.8)
--------------------
  src\processor\registers.*                    vector registers, VL, VTYPE, vsetvli
  src\processor\vector_alu.h                   integer vector instructions
  src\processor\vector_fpu.h                   floating-point vector instructions
  src\processor\rvss\rvss_processor.cpp        single-cycle execution
  src\processor\rv5s\rv5s_processor_base.cpp   pipeline execution and bypass
  src\assembler\parse_formats\vector_formats.cpp, src\assembler\code_generator.cpp
                                               assembler for vector instructions
  src\common\instructions.*                    instruction tables, disassembler
  src\processor\processor_base.*               performance counters and report
