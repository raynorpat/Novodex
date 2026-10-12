# Fluid collision mesh-array appender (`phys_fn_003541`)

`phys_fn_003541` at RVA `0x876d0` appends one 96-byte mesh collision record. The private candidate implementation is `nxFluidCollisionAppendMesh` in `Physics/src/FluidCollision.cpp`; public Physics headers remain unchanged.

IDA confirms the cdecl `(descriptor, shape)` signature. It reads the mesh-data pointer from `shape+0xe0`; the `-8` sentinel produces a null return and leaves the descriptor untouched. Otherwise the first four words come from mesh-data offsets `+12,+20,+8,+16`, in that order, followed by the common shape record from `phys_fn_003540`. The appender writes one 96-byte stride, increments the count, and returns the descriptor. The common packer's last dword is undefined in the oracle, so the differential compares the first 92 bytes and verifies the untouched tail after the stride.

`NxPhysicsFluidEmitterAbiTests` covers a selected mesh and the `-8` sentinel. Distinct geometry words verify their order; the test checks returns, counts, stack balance, and the output tail. The oracle-only run passed with `oracle_contract=1` while red because the candidate symbol was absent. The completed direct differential reports `candidate_present=1`, `oracle_contract=1`, `stack=1/1`, and `mismatches=0`.

A row-specific mutation sourced the first geometry word from mesh-data `+8` instead of `+12`. The oracle contract remained valid and the test rejected the candidate (`mismatches=1`, exit 1); restoring `+12` returned the mesh row and fluid-emitter aggregate to zero mismatches. Passing candidate SHA-256 `f2dddf20d890c6ea621e8e2598b2f7d024da82a0074937b0d965537b68f2111e`; pinned oracle SHA-256 `4b7db3e126735c576f79fe5666e6fa661de9724b2a78808bb0924325ac79602c`.

Raw oracle-only, passing differential, and mutation transcripts are in `evidence/raw/phase7-fluid-collision-mesh-append-003541/`. This closes only the mesh appender. Router `phys_fn_003553`, scene collector `phys_fn_003555`, and dirty static-collision builder `phys_fn_003505` remain open.

Fresh Release gates passed on `main`: Phase 5 at 2,658/2,658 and Phase 7 at 1,685/1,685 assertions, both exit 0. All 810 reconstruction-tooling tests passed; inventory validation reports 6,338 functions / 5,138 data objects / zero unexplained entries, and the public Physics header manifest verifies all 80 files. The rebuilt candidate DLL SHA-256 remains `f2dddf20d890c6ea621e8e2598b2f7d024da82a0074937b0d965537b68f2111e`.
