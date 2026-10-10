# Scene joint-support body array teardown (`phys_fn_000663`)

A simulated dynamic actor with a fixed joint grows the `JointSupportBody`
allocation used by the solver at Scene `+0x5ac`. The Scene stores the usable
array pointer four bytes after the allocation base because the block begins
with a count word. The fixture keeps the allocation live through Scene release
and verifies that the allocator no longer tracks `pointer - 4`. The pinned
oracle and candidate both report `bytes=100 freed=1`; their focused staged-pair
transcripts match exactly.

The row-specific negative control omits the `pointer - 4` release from
`nxSceneDelete`, rebuilds `NxPhysics.dll`, and reruns the same fixture. The
oracle exits 0; the candidate reports `joint_support_body_buffer bytes=100
freed=0` and exits 1 (`stdout_delta=23`). Restoring the source and rebuilding
returns the differential to exact output.

The support-array simulation also creates a lazy process-global allocation on
the oracle that the candidate does not make. The static-first teardown check
therefore reports and gates its per-Scene release delta (`-39`) rather than
including absolute process allocation totals. The Phase 3 gate passes
557/557, the Phase 7 gate passes 1,646/1,646, and all 810 tooling tests pass.
No public Physics headers or production source changed. This closes only the
joint-support allocation path; `phys_fn_000663` remains intermediate and the
full DLL reconstruction remains open.
