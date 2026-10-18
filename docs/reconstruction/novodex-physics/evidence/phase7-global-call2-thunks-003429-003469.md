# Two-argument global-call thunk family

The candidate reconstruction in `Physics/src/ObjectModel.cpp` implements the common thunk contract in `nxGlobalCall2`: load the object and second dword at `+0x7c/+0x80`, optionally follow the pointer at `this+4`, load the first callback argument at `object+0x30`, then call the bound global function with both arguments. The registered `NxPhysicsObjectLayoutTests` fixture exercises 11 pinned oracle RVAs: `003429`, `003433`, `003437`, `003441`, `003445`, `003457`, `003459`, `003463`, `003465`, `003469`, and `003679`. It binds each oracle function-pointer global to the recorder and checks hit count and both callback arguments, including the `this+4` case.

A candidate mutation changed the second-argument read from `+0x80` to `+0x84`. The fixture reported `cases=11 failures=11`, and the harness exited 1. Restoring the read produced `cases=11 failures=0` and an exact oracle differential. Raw build and differential runs are recorded in `evidence/raw/phase7-global-call2-thunks-003429-003469-20261017/`.

Fresh registered Phase 5 and Phase 7 gates pass at 2,887/2,887 and 3,026/3,026 coverage assertions. The final 817-test tooling run is in progress.

Fresh Phase 5, Phase 6, and Phase 7 gates pass at 2,887/2,887, 1,744/1,744, and 3,026/3,026 coverage assertions. The final tooling suite rerun is in progress.
