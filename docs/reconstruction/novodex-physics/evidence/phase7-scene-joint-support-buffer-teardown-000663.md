# Scene joint-support body array teardown (`phys_fn_000663`)

A simulated dynamic actor with a fixed joint grows three Scene-owned arrays.
The `JointSupportBody` solver array at `+0x5ac` is 100 bytes, including the
four-byte count prefix before the stored pointer. The solver-record array at
`+0x5b8` is 640 bytes, and the joint-reference array at `+0x58c` is 8 bytes.
The fixture captures each allocation before Scene release and checks that the
allocator no longer tracks its base. The pinned oracle and candidate report
all three freed; their focused staged-pair transcripts match exactly.

Each release has a row-specific negative control. Omitting the `pointer - 4`
release from `nxSceneDelete` reports `joint_support_body_buffer bytes=100
freed=0`; omitting the `+0x5b8` release reports `records_freed=0`; and omitting
the `+0x58c` release reports `refs_freed=0`. Each mutant rebuilds the DLL,
leaves the oracle at exit 0, and exits 1 on the candidate (`stdout_delta=23`).
Restoring the source and rebuilding returns the differential to exact output.

The support-array simulation also creates a lazy process-global allocation on
the oracle that the candidate does not make. The static-first teardown check
therefore reports and gates its per-Scene release delta (`-39`) rather than
including absolute process allocation totals. The Phase 3 gate passes
558/558, the Phase 7 gate passes 1,647/1,647, and all 810 tooling tests pass.
No public Physics headers or production source changed. This closes only the
joint-support allocation path; `phys_fn_000663` remains intermediate and the
full DLL reconstruction remains open.
