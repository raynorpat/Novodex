# Phase 6 constant-argument virtual thunks 004417–004433

The nine 8-byte oracle thunks load the object's vtable, push a row-specific constant, call the virtual method at vtable offset `0x4c`, and return. IDA confirmed this exact instruction shape and constants: `004417=1`, `004419=5`, `004421=4`, `004423=0`, `004425=2`, `004427=3`, `004429=8`, `004431=6`, and `004433=7`.

`Physics/src/ObjectModel.cpp` now contains nine distinct noinline x86 naked functions. They preserve the `thiscall` entry ABI and reproduce the oracle's instruction sequence and argument for each thunk. The object-layout fixture invokes every candidate function against the pinned oracle and emits a required per-row marker. The fixture itself remains pinned by `oracle-only-baselines.json`; its oracle digest is unchanged (`3acf0dc7`). No public SDK headers changed.

Each of the nine production immediates was independently changed in source to `argument XOR 1`, rebuilt, and run against the oracle. Every mutant reported one mismatch for its row and exited nonzero. Restoring the original source returned all nine row mismatch counts to zero and the fixture exited successfully. The full Phase 5, 6, and 7 gates then passed at 2,908/2,908, 1,765/1,765, and 3,047/3,047 assertions.

Candidate symbols and RVAs are recorded in `inventory.json`; raw mutation, restored differential, and complete gate logs with hashes are in [raw evidence](raw/phase6-constant-argument-thunks-004417-004433-20261018/).
